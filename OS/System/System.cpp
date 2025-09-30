#include "MatrixOS.h"
#include "../../Applications/Setting/Setting.h"
#include "System.h"
#include "Applications.h" // This is from device layer

#include "../USB/USB.h"
#include "../LED/LED.h"
#include "../KeyPad/KeyPad.h"
#include "../FileSystem/File.h"
#include "../MIDI/MIDI.h"

extern std::unordered_map<uint32_t, Application_Info*> applications;

namespace MatrixOS::SYS
{
  // Application argument storage
  static vector<string> next_app_args;

  // Thread Local Storage indices
  enum TLS_Index {
    TLS_PERMISSIONS_INDEX = 0,  // Stores TaskPermissions bitmap
    TLS_MAX_INDEX = 1           // Reserve for future use
  };

  void ApplicationFactory(void* param) {
    MLOGD("Application Factory", "App ID %X", next_app_id);

    active_app = NULL; 

    if (next_app_id != 0)
    {
      auto application = applications.find(next_app_id);
      if (application != applications.end())
      {
        MLOGD("Application Factory", "Launching %s-%s", application->second->author.c_str(),
                                    application->second->name.c_str());
        active_app_id = next_app_id;
        active_app = application->second->factory();
        active_app_info = application->second;
      }
    } 

    if (active_app == NULL)  // Default to launch shell
    {
      if (next_app_id != 0)
      {
        MLOGD("Application Factory", "Can't find target app.");
      }
      MLOGD("Application Factory", "Launching Shell");
      next_app_id = OS_SHELL;
      auto application = applications.find(next_app_id);
      if (application != applications.end())
      {
        MLOGD("Application Factory", "Launching %s-%s", application->second->author.c_str(),
                                    application->second->name.c_str());
        active_app = application->second->factory();
        active_app_id = next_app_id;
        active_app_info = application->second;
      }
    }

    next_app_id = 0;  // Reset active_app_id so when active app exits it will default to shell again.

    // Update task permissions based on app info
    if (active_app_info != nullptr) {
      TaskPermissions perms;
      perms.privileged = active_app_info->is_system;
      SetTaskPermissions(perms);  // Uses current task (which IS the app task)
      MLOGD("Application Factory", "Set app permissions: %s", perms.privileged ? "Privileged" : "Not Privileged");
    }

    InitSysModules();
    MatrixOS::LED::Fade();

    // Pass arguments to application
    active_app->Start(next_app_args);
  }

  void Supervisor(void* param) {

    MLOGD("Supervisor", "%d Apps registered", applications.size());

    active_app_task = xTaskCreateStatic(ApplicationFactory, "application", APPLICATION_STACK_SIZE, NULL, 1,
                                        application_stack, &application_taskdef);

    bool exited = false;
    while (true)
    {
      // Check if function key is held for more than 3 seconds
      KeyInfo* fnKeyInfo = MatrixOS::KeyPad::GetKey(FUNCTION_KEY);
      if (exited == false && (fnKeyInfo->HoldTime() > 3000))
      {
          MLOGD("Supervisor", "Function key held for 3s, force exiting app");
          exited = true;
          ExitAPP();
      }
      else if (fnKeyInfo->Active() == false)
      {
        exited = false;
      }
      
      if (active_app_task == NULL || eTaskGetState(active_app_task) == eTaskState::eDeleted)
      {
        active_app_task = xTaskCreateStatic(ApplicationFactory, "application", APPLICATION_STACK_SIZE, NULL, 1,
                                            application_stack, &application_taskdef);
      }
      DelayMs(100);
    }
  }

  void Begin(void) {
    Device::DeviceInit();

    MatrixOS::USB::Init();

    InitSysModules();

    UpdateSystemNVS();

    inited = true;

    MLOGI("System", "Matrix OS initialization complete");

    MLOGE("Logging", "This is an error log");
    MLOGW("Logging", "This is a warning log");
    MLOGI("Logging", "This is an info log");
    MLOGD("Logging", "This is a debug log");
    MLOGV("Logging", "This is a verbose log");

    ExecuteAPP(DEFAULT_BOOTANIMATION);

    Device::DeviceStart();  // App won't run till supervisor is running

    (void)xTaskCreateStatic(Supervisor, "supervisor", configMINIMAL_STACK_SIZE * 4, NULL, 1, supervisor_stack,
                            &supervisor_taskdef);

    // next_app_id = GenerateAPPID("203 Systems", "Performance Mode");  // Launch Performance mode by default for now
  }

  void InitSysModules(void)
  {
    MatrixOS::KeyPad::Init();
    MatrixOS::LED::Init();
    MatrixOS::FileSystem::Init();
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
    DelayMs(20);  // Wait for led data to be updated first.
    Device::Bootloader();
  }

  void OpenSetting(void) {
    Setting setting;
    setting.Start();
  }

  void Rotate(Direction new_rotation, bool absolute) {
    if (new_rotation == 0 || new_rotation == 90 || new_rotation == 180 || new_rotation == 270)
    {
      if (new_rotation == 0 && !absolute)
      { return; }
      // LED::RotateCanvas(new_rotation); //TODO Does not work if absolute is true
      for (uint8_t ledLayer = 0; ledLayer <= LED::CurrentLayer(); ledLayer++)
      { LED::Fill(0, ledLayer); }
      UserVar::rotation = (Direction)((UserVar::rotation * !absolute + new_rotation) % 360);
    }
  }

  uint32_t GenerateAPPID(string author, string app_name) {
    // MLOG("System", "APP ID: %u", app_id);
    return StringHash(author + "-" + app_name);
  }

  void ExecuteAPP(uint32_t app_id, const vector<string>& args) {
    next_app_args = args;
    next_app_id = app_id;

    if (active_app_task != NULL) {
      ExitAPP();
    }
  }

  void ExecuteAPP(string author, string app_name, const vector<string>& args) {
    next_app_args = args;
    MLOGD("System", "Launching APP\t%s - %s", author.c_str(), app_name.c_str());
    next_app_id = GenerateAPPID(author, app_name);

    if (active_app_task != NULL) {
      ExitAPP();
    }
  }

  void ExitAPP() {
    if(MatrixOS::UserVar::ui_animation)
    {
      MatrixOS::LED::Fade();
      MatrixOS::LED::Fill(0, 0);
      // Add a delay so it fade to black
      // This give the user a better sense that they just exited an APP
      MatrixOS::SYS::DelayMs(crossfade_duration);
    }

    if (xTaskGetCurrentTaskHandle() != active_app_task) {
      vTaskSuspend(active_app_task);
    }
    
    // Safeguard against nullptr before calling End()
    if (active_app != nullptr) {
      active_app->End();
    }
    
    // Safeguard destructor call
    if (active_app_info != nullptr && active_app_info->destructor != nullptr && active_app != nullptr) {
      active_app_info->destructor(active_app);
    }

    if (active_app_task != NULL)
    {
      UI::ExitAllUIs();
      active_app = NULL;
      vTaskDelete(active_app_task);
    }
    active_app_task = NULL;
  }

  void ErrorHandler(string error) {
    if (error.empty())
    {
      error = "Undefined Error";
    }

    MLOGE("System", "Matrix OS Error: %s", error.c_str());

    // Show Blue Screen
    MatrixOS::LED::Fill(0x00adef);
    if (Device::x_size >= 5 && Device::y_size >= 5)
    {
      MatrixOS::LED::SetColor(Point(1, 1), 0xFFFFFF);
      MatrixOS::LED::SetColor(Point(1, 3), 0xFFFFFF);

      MatrixOS::LED::SetColor(Point(3, 1), 0xFFFFFF);
      MatrixOS::LED::SetColor(Point(3, 2), 0xFFFFFF);
      MatrixOS::LED::SetColor(Point(3, 3), 0xFFFFFF);
    }

    MatrixOS::LED::Update();

    Device::ErrorHandler();  // Low level indicator in case LED and USB failed
  }

  uint16_t GetApplicationCount()  // Used by shell, for some reason shell can not access app_count
  {
    return applications.size();
  }

  #define SYSTEM_VERSION_ID(major, minor, patch) ((major << 16) | (minor << 8) | patch)
  void UpdateSystemNVS() {
    if(!prev_system_version.Load() || (prev_system_version & 0xFFFFFF00) > (MATRIXOS_VERSION_ID & 0xFFFFFF00))// System version is not set or is newer than current
    {
      // Wipe NVS
      Device::NVS::Clear();
      prev_system_version.Set(MATRIXOS_VERSION_ID);
      return;
    }
    
    if(prev_system_version == MATRIXOS_VERSION_ID) // System version is up to date
    {
      // We are all good here
      return;
    }

    // System version is older, update here
    uint32_t prev_ver = prev_system_version >> 8;
    prev_system_version.Set(MATRIXOS_VERSION_ID);

    // Code for demo purposes, pre_system_version var is introduced in 2.5.0
    // if(prev_ver == SYSTEM_VERSION_ID(2, 5, 0)) 
    // {
    // Update stuffs here
    // }

    (void)prev_ver;
  }

  TaskPermissions GetTaskPermissions(TaskHandle_t task) {
    // If null, get current task
    if (task == nullptr) {
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
    if (task == nullptr) {
      task = xTaskGetCurrentTaskHandle();
    }

    // Set permissions in Thread Local Storage
    vTaskSetThreadLocalStoragePointer(task, TLS_PERMISSIONS_INDEX, (void*)(uintptr_t)permissions.raw);
  }
}