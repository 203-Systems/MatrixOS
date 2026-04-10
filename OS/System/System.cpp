#include "MatrixOS.h"
#include "../../Applications/Setting/Setting.h"
#include "System.h"
#include "Applications.h" // This is from device layer

#include "../USB/USB.h"
#include "../LED/LED.h"
#include "../Input/Input.h"
#include "../FileSystem/File.h"
#include "../FileSystem/FileSystem.h"
#include "../MIDI/MIDI.h"
#include "task.h"

extern std::unordered_map<uint32_t, Application_Info*> applications;

namespace MatrixOS::SYS
{
// Application argument storage
static vector<string> nextAppArgs;

// Thread Local Storage indices
enum TLSIndex {
  TLS_PERMISSIONS_INDEX = 0, // Stores TaskPermissions bitmap
  TLS_MAX_INDEX = 1          // Reserve for future use
};

void ApplicationFactory(void* param) {
  MLOGD("Application Factory", "App ID %X", nextAppId);

  activeApp = NULL;

  if (nextAppId != 0)
  {
    auto& applications = GetApplications();
    auto application = applications.find(nextAppId);
    if (application != applications.end())
    {
      MLOGD("Application Factory", "Launching %s-%s", application->second->author.c_str(), application->second->name.c_str());
      activeAppId = nextAppId;
      activeApp = application->second->factory();
      if (activeApp == NULL)
      {
        MLOGE("Application Factory", "Factory returned NULL - allocation failed!");
        ErrorHandler("App allocation failed");
      }
      activeAppInfo = application->second;
    }
  }

  if (activeApp == NULL) // Default to launch shell
  {
    if (nextAppId != 0)
    {
      MLOGD("Application Factory", "Can't find target app.");
    }
    MLOGD("Application Factory", "Launching Shell");
    MLOGD("Application Factory", "Free heap before Shell factory: %d bytes", xPortGetFreeHeapSize());
    nextAppId = OS_SHELL;
    auto& applications = GetApplications();
    auto application = applications.find(nextAppId);
    if (application != applications.end())
    {
      MLOGD("Application Factory", "Launching %s-%s", application->second->author.c_str(), application->second->name.c_str());
      activeApp = application->second->factory();
      if (activeApp == NULL)
      {
        MLOGE("Application Factory", "Shell factory returned NULL - allocation failed!");
        ErrorHandler("Shell allocation failed");
      }
      activeAppId = nextAppId;
      activeAppInfo = application->second;
    }
  }

  nextAppId = 0; // Reset activeAppId so when active app exits it will default to shell again.

  // Update task permissions based on app info
  if (activeAppInfo != nullptr)
  {
    TaskPermissions perms;
    perms.privileged = activeAppInfo->isSystem;
    SetTaskPermissions(perms); // Uses current task (which IS the app task)
    MLOGD("Application Factory", "Set app permissions: %s", perms.privileged ? "Privileged" : "Not Privileged");
  }

  InitSysModules();
  MatrixOS::LED::Fade();

  // Pass arguments to application
  activeApp->Start(nextAppArgs);
}

void Supervisor(void* param) {

  MLOGD("Supervisor", "%d Apps registered", GetApplications().size());

  activeAppTask =
      xTaskCreateStatic(ApplicationFactory, "application", APPLICATION_STACK_SIZE, NULL, 1, applicationStack, &applicationTaskdef);

  bool exited = false;
  InputId fnKeyId = InputId::FunctionKey();
  while (true)
  {
    // Check if function key is held for more than 3 seconds via new Input API
    InputSnapshot fnSnap;
    bool hasFnState = MatrixOS::Input::GetState(fnKeyId, &fnSnap);
    bool fnActive = hasFnState && fnSnap.keypad.Active();
    uint32_t fnHoldTime = fnActive ? ((uint32_t)MatrixOS::SYS::Millis() - fnSnap.keypad.lastEventTime) : 0;

    if (exited == false && (fnHoldTime > 3000))
    {
      MLOGD("Supervisor", "Function key held for 3s, force exiting app");
      exited = true;
      ExitAPP();
    }
    else if (!fnActive)
    {
      exited = false;
    }

    if (activeAppTask == NULL || eTaskGetState(activeAppTask) == eTaskState::eDeleted)
    {
      activeAppTask =
          xTaskCreateStatic(ApplicationFactory, "application", APPLICATION_STACK_SIZE, NULL, 1, applicationStack, &applicationTaskdef);
    }
    DelayMs(100);
  }
}

void Begin(void) {
  MLOGI("System", "Begin: DeviceInit start");
  Device::DeviceInit();
  MLOGI("System", "Begin: DeviceInit done");

  // Initialize MIDI system before USB to ensure osPort exists
  MLOGI("System", "Begin: MIDI init start");
  MatrixOS::MIDI::Init();
  MLOGI("System", "Begin: MIDI init done");

  MLOGI("System", "Begin: USB init start");
  MatrixOS::USB::Init();
  MLOGI("System", "Begin: USB init done");

  MLOGI("System", "Begin: InitSysModules start");
  InitSysModules();
  MLOGI("System", "Begin: InitSysModules done");

  MLOGI("System", "Begin: UpdateSystemNVS start");
  UpdateSystemNVS();
  MLOGI("System", "Begin: UpdateSystemNVS done");

  inited = true;

  MLOGI("System", "Matrix OS initialization complete");

  MLOGE("Logging", "This is an error log");
  MLOGW("Logging", "This is a warning log");
  MLOGI("Logging", "This is an info log");
  MLOGD("Logging", "This is a debug log");
  MLOGV("Logging", "This is a verbose log");

  ExecuteAPP(DEFAULT_BOOTANIMATION);

  Device::DeviceStart(); // App won't run till supervisor is running

  (void)xTaskCreateStatic(Supervisor, "supervisor", configMINIMAL_STACK_SIZE * 4, NULL, 1, supervisorStack, &supervisorTaskdef);

  // nextAppId = GenerateAPPID("203 Systems", "Performance Mode");  // Launch Performance mode by default for now
}

void InitSysModules(void) {
  MatrixOS::Input::Init();
  MatrixOS::LED::Init();
#if DEVICE_STORAGE
  MatrixOS::FileSystem::Init();
#endif
  MatrixOS::USB::SetMode(USB_MODE_NORMAL);
  MatrixOS::MIDI::Init();
  MatrixOS::HID::Init();
}

uint64_t Millis(void) {
  return ((((uint64_t)xTaskGetTickCount()) * 1000) / configTICK_RATE_HZ);
}

uint64_t Micros(void) {
  return Device::Micros();
}

void DelayMs(uint32_t ms) {
  vTaskDelay(pdMS_TO_TICKS(ms));
}

void Reboot(void) {
  Device::Reboot();
}

void Bootloader() {
  MatrixOS::LED::Fill(0);
  MatrixOS::LED::Update();
  DelayMs(20); // Wait for led data to be updated first.
  Device::Bootloader();
}

void OpenSetting(void) {
  Setting setting;
  setting.SystemSetting();
}

uint32_t GenerateAPPID(string author, string appName) {
  // MLOG("System", "APP ID: %u", appId);
  return StringHash(author + "-" + appName);
}

void ExecuteAPP(uint32_t appId, const vector<string>& args) {
  nextAppArgs = args;
  nextAppId = appId;

  if (activeAppTask != NULL)
  {
    ExitAPP();
  }
}

void ExecuteAPP(string author, string appName, const vector<string>& args) {
  nextAppArgs = args;
  MLOGD("System", "Launching APP\t%s - %s", author.c_str(), appName.c_str());
  nextAppId = GenerateAPPID(author, appName);

  if (activeAppTask != NULL)
  {
    ExitAPP();
  }
}

void ExitAPP() {
  if (MatrixOS::UserVar::uiAnimation)
  {
    MatrixOS::LED::Fade();
    MatrixOS::LED::Fill(0, 0);
    // Add a delay so it fade to black
    // This give the user a better sense that they just exited an APP
    MatrixOS::SYS::DelayMs(crossfadeDuration);
  }

  if (xTaskGetCurrentTaskHandle() != activeAppTask)
  {
    vTaskSuspend(activeAppTask);
  }

  // Safeguard against nullptr before calling End()
  if (activeApp != nullptr)
  {
    activeApp->End();
  }

  // Safeguard destructor call
  if (activeAppInfo != nullptr && activeAppInfo->destructor != nullptr && activeApp != nullptr)
  {
    activeAppInfo->destructor(activeApp);
  }

  if (activeAppTask != NULL)
  {
    UI::ExitAllUIs();
    activeApp = NULL;
    vTaskDelete(activeAppTask);
  }
  activeAppTask = NULL;
}

void ErrorHandler(string error) {
  if (error.empty())
  {
    error = "Undefined Error";
  }

  MLOGE("System", "Matrix OS Error: %s", error.c_str());

  // Show Blue Screen
  MatrixOS::LED::Fill(0x00adef);
  if (Device::xSize >= 5 && Device::ySize >= 5)
  {
    MatrixOS::LED::SetColor(Point(1, 1), 0xFFFFFF);
    MatrixOS::LED::SetColor(Point(1, 3), 0xFFFFFF);

    MatrixOS::LED::SetColor(Point(3, 1), 0xFFFFFF);
    MatrixOS::LED::SetColor(Point(3, 2), 0xFFFFFF);
    MatrixOS::LED::SetColor(Point(3, 3), 0xFFFFFF);
  }

  MatrixOS::LED::Update();

  Device::ErrorHandler(); // Low level indicator in case LED and USB failed
}

uint16_t GetApplicationCount() // Used by shell, for some reason shell can not access app_count
{
  return GetApplications().size();
}

#define SYSTEM_VERSION_ID(major, minor, patch) ((major << 16) | (minor << 8) | patch)
void UpdateSystemNVS() {
  if (!prevSystemVersion.Load() ||
      (prevSystemVersion & 0xFFFFFF00) > (MATRIXOS_VERSION_ID & 0xFFFFFF00)) // System version is not set or is newer than current
  {
    // Wipe NVS
    Device::NVS::Clear();
    prevSystemVersion.Set(MATRIXOS_VERSION_ID);
    return;
  }

  if (prevSystemVersion == MATRIXOS_VERSION_ID) // System version is up to date
  {
    // We are all good here
    return;
  }

  // System version is older, update here
  uint32_t prevVer = prevSystemVersion >> 8;
  prevSystemVersion.Set(MATRIXOS_VERSION_ID);

  // Code for demo purposes, pre_system_version var is introduced in 2.5.0
  // if(prevVer == SYSTEM_VERSION_ID(2, 5, 0))
  // {
  // Update stuffs here
  // }

  (void)prevVer;
}

TaskPermissions GetTaskPermissions(TaskHandle_t task) {
  // If null, get current task
  if (task == nullptr)
  {
    task = xTaskGetCurrentTaskHandle();
  }

  // Get permissions from Thread Local Storage
  uint32_t raw = (uintptr_t)pvTaskGetThreadLocalStoragePointer(task, TLS_PERMISSIONS_INDEX);
  return TaskPermissions(raw);
}

bool IsTaskPrivileged(TaskHandle_t task) {
  TaskPermissions perms = GetTaskPermissions(task);
  return perms.privileged;
}

void SetTaskPermissions(TaskPermissions permissions, TaskHandle_t task) {
  // If null, get current task
  if (task == nullptr)
  {
    task = xTaskGetCurrentTaskHandle();
  }

  // Set permissions in Thread Local Storage
  vTaskSetThreadLocalStoragePointer(task, TLS_PERMISSIONS_INDEX, (void*)(uintptr_t)permissions.raw);
}
} // namespace MatrixOS::SYS
