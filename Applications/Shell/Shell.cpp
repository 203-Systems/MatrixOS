#include "Shell.h"
#include <algorithm>
#include <cctype>
#include <string>

#include "AppLauncherBar.h"
#include "AppLauncherPicker.h"
#include "AppLauncherBarEditMode.h"
#include "AppLauncherPickerEditMode.h"

#define APP_FOLDER_HASH_KEY(app_id_val)  Hash("203 Systems-Shell-Folder-Of-" + std::to_string(app_id_val))
#define APP_FOLDER_COLOR_HASH_KEY  Hash("203 Systems-Shell-Folder-Colors")

namespace MatrixOS::SYS
{
  void ExecuteAPP(uint32_t app_id);
  uint16_t GetApplicationCount();
}  // Use non exposed Matrix OS API

void Shell::Setup()
{
  #ifdef configUSE_FREERTOS_PROVIDED_HEAP
    MLOGD("Shell", "Matrix OS Free Heap Size: %.2fkb (%d%%)", xPortGetFreeHeapSize() / 1024.0f, xPortGetFreeHeapSize() * 100 / configTOTAL_HEAP_SIZE);
    MLOGD("Shell", "Matrix OS Minimum Free Heap Size: %.2fkb (%d%%)", xPortGetMinimumEverFreeHeapSize() / 1024.0f, xPortGetMinimumEverFreeHeapSize() * 100 / configTOTAL_HEAP_SIZE);
  #endif
  #ifdef ESP_PLATFORM
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    uint32_t total_heap_size = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);  
    MLOGD("Shell", "Matrix OS Free Heap Size: %d (%.2fkb) (%d%%)", info.total_free_bytes, info.total_free_bytes / 1024.0f, info.total_free_bytes * 100 / total_heap_size); 
    MLOGD("Shell", "Matrix OS Lifetime Minimum Free Heap Size: %d (%.2fkb) (%d%%)", info.minimum_free_bytes, info.minimum_free_bytes / 1024.0f, info.minimum_free_bytes * 100 / total_heap_size);
    MLOGD("Shell", "Matrix OS Total Heap Size: %d (%.2fkb)", total_heap_size, total_heap_size / 1024.0f);
    MLOGD("Shell", "Matrix OS Free Blocks: %d", info.free_blocks);
    MLOGD("Shell", "Matrix OS Largest Free Block: %.2fkb", info.largest_free_block / 1024.0f);
    MLOGD("Shell", "Matrix OS Total Blocks: %d", info.total_blocks);
  #endif
  
  // Initialize the folder system
  InitializeFolderSystem();
}

void Shell::Loop() {
    ApplicationLauncher();
}

void Shell::InitializeFolderSystem() {
  // Clear and initialize folders with default names and colors
  folders.clear();

  MatrixOS::NVS::GetVariable(APP_FOLDER_COLOR_HASH_KEY, folder_colors, sizeof(folder_colors));

  folders[FOLDER_HIDDEN] = {{}};
  folders[FOLDER_INVISIBLE] = {{}};

  for (uint8_t i = 0; i < FOLDER_COUNT; i++) {
    folders[i] = {{}};
  }
  
  // Iterate through all applications in registration order
  for (const auto& [order_id, app_id] : application_ids) {
    auto application_it = applications.find(app_id);
    if(application_it == applications.end()) {
      continue;
    }
    Application_Info* application = application_it->second;
    
    uint8_t folder_id = GetAppFolder(app_id, application);
    
    // Make sure the folder exists in the map
    if (folders.find(folder_id) == folders.end()) {
      folders[folder_id] = {{}}; // Create a new folder if it doesn't exist
    }
    
    // Add the app to the folder
    folders[folder_id].app_ids.push_back(app_id);
    MLOGD("Shell", "App [%d] %s assigned to folder %d", order_id, application->name.c_str(), folder_id);
  }
  
  // Log folder statistics
  for (uint8_t i = 0; i < FOLDER_COUNT; i++) {
    MLOGD("Shell", "Folder %d: %d apps", i, folders[i].app_ids.size());
  }
  MLOGD("Shell", "Folder [Hidden]: %d apps", folders[FOLDER_HIDDEN].app_ids.size());
  MLOGD("Shell", "Folder [Invisible]: %d apps", folders[FOLDER_INVISIBLE].app_ids.size());
}

bool Shell::EnableFolder(uint8_t folder_idx, Color color) {
  // Check if folder index is valid
  if (folder_idx >= FOLDER_COUNT) {
    MLOGW("Shell", "Invalid folder index: %d", folder_idx);
    return false;
  }
  
  // Enable the folder at the specified index
  folder_colors[folder_idx] = color;
  
  // Ensure folder exists in the map
  if (folders.find(folder_idx) == folders.end()) {
    folders[folder_idx] = {{}};
  }
  
  // Save to NVS
  MatrixOS::NVS::SetVariable(APP_FOLDER_COLOR_HASH_KEY, folder_colors, sizeof(folder_colors));
  
  MLOGD("Shell", "Enabled folder %d with color 0x%06X", folder_idx, color.RGB());
  return true;
}

void Shell::DisableFolder(uint8_t folder_id) {
  if (folder_id >= FOLDER_COUNT) {
    MLOGW("Shell", "Invalid folder ID: %d", folder_id);
    return;
  }

  // Move apps to folder 0 before disabling
  for (uint32_t app_id : folders[folder_id].app_ids) {
    MoveAppToFolder(app_id, 0);
  }

  // Disable folder by setting color to black
  folder_colors[folder_id] = Color(0x000000);
  folders[folder_id].app_ids.clear();

  // Save to NVS
  MatrixOS::NVS::SetVariable(APP_FOLDER_COLOR_HASH_KEY, folder_colors, sizeof(folder_colors));

  // Reset current folder to 0 if we just disabled the current one
  if (current_folder == folder_id) {
    current_folder = 0;
  }

  MLOGD("Shell", "Disabled folder %d and moved its apps to folder 0.", folder_id);
}

void Shell::DeleteFolder(uint8_t folder_id) {
  if (folder_id >= FOLDER_COUNT) {
    MLOGW("Shell", "Invalid folder ID: %d", folder_id);
    return;
  }

  // Move apps to folder 0 before deleting
  for (uint32_t app_id : folders[folder_id].app_ids) {
    MoveAppToFolder(app_id, 0);
  }

  // Clear folder data
  folders[folder_id].app_ids.clear();

  // Remove from NVS
  folder_colors[folder_id] = Color(0x000000);
  MatrixOS::NVS::SetVariable(APP_FOLDER_COLOR_HASH_KEY, folder_colors, sizeof(folder_colors));

  MLOGD("Shell", "Deleted folder %d and moved its apps to folder 0.", folder_id);
}

void Shell::MoveAppToFolder(uint32_t app_id, uint8_t folder_id) {
  // Check if folder exists
  if (folders.find(folder_id) == folders.end()) {
    MLOGW("Shell", "Folder %d does not exist. Cannot move app %d.", folder_id, app_id);
    return;
  }
  else if(folder_colors[folder_id] == Color(0x000000)) {
    MLOGW("Shell", "Folder %d is not enabled. Cannot move app %d.", folder_id, app_id);
    return;
  }
  else
  {
    // Remove app from current folder
    for (auto& [fid, folder] : folders) {
      auto it = std::find(folder.app_ids.begin(), folder.app_ids.end(), app_id);
      if (it != folder.app_ids.end()) {
        folder.app_ids.erase(it);
        break;
      }
    }

    // Add app to new folder
    folders[folder_id].app_ids.push_back(app_id);

    // Save to NVS
    uint32_t nvs_key_hash = APP_FOLDER_HASH_KEY(app_id);
    MatrixOS::NVS::SetVariable(nvs_key_hash, &folder_id, sizeof(folder_id));

    MLOGD("Shell", "Moved app %d to folder %d.", app_id, folder_id);
  }
}

uint8_t Shell::GetAppFolder(uint32_t app_id, Application_Info* app_info) {
  // Check if app is invisible first
  if (!app_info->visibility) {
    return FOLDER_INVISIBLE;
  }

  uint8_t folder_id = 0;
  uint32_t nvs_key_hash = APP_FOLDER_HASH_KEY(app_id);
  MatrixOS::NVS::GetVariable(nvs_key_hash, &folder_id, sizeof(folder_id));

  // Only check regular folders (0-5) for validity, not special folders
  if(folder_id < FOLDER_COUNT) {
    if(folders.find(folder_id) == folders.end() || folder_colors[folder_id] == Color(0x000000))
    {
      folder_id = 0; // Default to folder 0 if folder not exist
      MoveAppToFolder(app_id, 0);
    }
  }

  return folder_id;
}

void Shell::ApplicationLauncher() {
  uint8_t tap_counter = 0;
  uint32_t last_tap = 0;

  UI applicationLauncher("Application Launcher", Color(0x00FFAA), false);

  #if MATRIXOS_LOG_LEVEL == LOG_LEVEL_DEBUG  // Logging Mode Indicator
  #define SHELL_SYSTEM_SETTING_COLOR Color(0xFFBF00)
  #elif MATRIXOS_LOG_LEVEL == LOG_LEVEL_VERBOSE
  #define SHELL_SYSTEM_SETTING_COLOR Color(0xFF007F)
  #else
  #define SHELL_SYSTEM_SETTING_COLOR Color(0xFFFFFF)
  #endif
  
  UIButton systemSettingBtn;
  systemSettingBtn.SetName("System Setting");
  systemSettingBtn.SetColor(SHELL_SYSTEM_SETTING_COLOR);
  systemSettingBtn.OnPress([&]() -> void { MatrixOS::SYS::OpenSetting(); });

  applicationLauncher.AddUIComponent(systemSettingBtn, Point(7, 7));

  AppLauncherPicker appLauncherPicker(this);
  applicationLauncher.AddUIComponent(&appLauncherPicker, Point(0, 0));
  
  AppLauncherBar appLauncherBar(this);
  applicationLauncher.AddUIComponent(&appLauncherBar, Point(0, 7));

  applicationLauncher.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    if (keyEvent->id == FUNCTION_KEY && keyEvent->info.state == HOLD)
    {
      ApplicationLauncherEditing();
      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
  });

  applicationLauncher.AllowExit(false);  // So nothing happens
  applicationLauncher.Start();
}

void Shell::ApplicationLauncherEditing() {
  UI applicationLauncherEdit("Edit Mode", Color(0xFF0000), false);

  uint32_t start_time = MatrixOS::SYS::Millis();
  
  // Clear any previous selection
  selected_app_id = 0;
  
  UIButton exitEditing;
  exitEditing.SetName("Exit Editing");
  exitEditing.SetColorFunc([&]() -> Color { return ColorEffects::ColorStrobe(Color(0xFF0000), 1000, start_time); });
  exitEditing.OnPress([&]() -> void { applicationLauncherEdit.Exit(); });
  applicationLauncherEdit.AddUIComponent(exitEditing, Point(7, 7));

  AppLauncherPickerEditMode appLauncherPickerEdit(this);
  applicationLauncherEdit.AddUIComponent(&appLauncherPickerEdit, Point(0, 0));
  
  AppLauncherBarEditMode appLauncherBarEdit(this);
  applicationLauncherEdit.AddUIComponent(&appLauncherBarEdit, Point(0, 7));

  applicationLauncherEdit.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    if (keyEvent->id == FUNCTION_KEY && keyEvent->info.state == RELEASED)
    {
      // Exit edit mode
      applicationLauncherEdit.Exit();
      return false;
    }
    return false;
  });

  applicationLauncherEdit.Start();

  if(current_folder >= FOLDER_COUNT || folder_colors[current_folder] == Color(0x000000))
  {
    current_folder = 0; // Reset to folder 0 if current folder is invalid
  }
}

void Shell::LaunchAnimation(Point origin, Color color)
{
  if(!MatrixOS::UserVar::ui_animation) { return; }

  uint32_t startTime = MatrixOS::SYS::Millis();

  const float speed = 30; // base mills per pixel
  const float edgeInnerWidth = -8;
  const float edgeWidth = 1;
  const float edgeOuterWidth = 1;
  const float endDistance = 20;
  Timer animTimer;
  uint16_t frameTime = 1000 / Device::LED::fps;

  while(true)
  {
    if(!animTimer.Tick(frameTime)) { continue; }
    float r = (MatrixOS::SYS::Millis() - startTime) / speed - edgeWidth;

    if(r > endDistance) { break; }

    for (uint16_t i = 0; i < MatrixOS::LED::GetLedCount(); i++)
    {
      Point xy = Device::LED::Index2XY(i);

      if(!xy) { continue; }

      float distanceDiff = sqrt(pow(xy.x - origin.x, 2) + pow(xy.y - origin.y, 2)) - r;

      Color pixelColor = Color(0);
      if(distanceDiff > (edgeWidth + edgeOuterWidth)) // Outside of render range
      {
        continue; // Do not change existing color
      }
      else if(distanceDiff < (edgeInnerWidth - edgeWidth)) // Inside of render range
      {
        pixelColor = Color(0);
      }
      else if(abs(distanceDiff) <= edgeWidth) // In the edge width
      {
        pixelColor = color;
      }
      else if(distanceDiff > edgeWidth) // In the outer edge
      {

        pixelColor = color.Scale((1 - (distanceDiff - edgeWidth) / edgeOuterWidth) * 255);
      }
      else if(distanceDiff < -edgeWidth) // In the inner edge
      {
        pixelColor = color.Scale((1 - (distanceDiff + edgeWidth) / edgeInnerWidth) * 255);
      }
      MatrixOS::LED::SetColor(xy, pixelColor);
    }
    MatrixOS::LED::Update();
  }
  MatrixOS::LED::Fill(Color(0));
}