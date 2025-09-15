#include "Shell.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>

#include "AppLauncherBar.h"
#include "AppLauncherPicker.h"
#include "AppLauncherBarEditMode.h"
#include "AppLauncherPickerEditMode.h"

#define APP_FOLDER_COLOR_HASH_KEY  StringHash("203 Systems-Shell-Folder-Colors")

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

  // Test filesystem functionality - DISABLED to prevent conflicts with USB MSC
  // TestFileSystem();
}

void Shell::Loop() {
    ApplicationLauncher();
}

void Shell::TestFileSystem() {
#if DEVICE_STORAGE == 1
    MLOGD("Shell", "Testing filesystem functionality...");

    // Check storage status
    const Device::Storage::StorageStatus* status = Device::Storage::Status();
    if (!status->available) {
        MLOGW("Shell", "Storage not available - skipping filesystem tests");
        return;
    }

    MLOGD("Shell", "Storage Status:");
    MLOGD("Shell", "  Available: %s", status->available ? "true" : "false");
    MLOGD("Shell", "  Write Protected: %s", status->write_protected ? "true" : "false");
    MLOGD("Shell", "  Sector Count: %lu", status->sector_count);
    MLOGD("Shell", "  Sector Size: %u bytes", status->sector_size);
    MLOGD("Shell", "  Block Size: %lu sectors", status->block_size);

    // Test basic file operations
    string test_file = "/test.txt";
    string test_content = "MatrixOS Filesystem Test\nTimestamp: ";
    test_content += std::to_string(Device::Micros());

    // Test file write using low-level API
    MatrixOS::File::File* file = MatrixOS::File::Open(test_file, FA_WRITE | FA_CREATE_ALWAYS);
    if (file) {
        size_t written = MatrixOS::File::Write(file, test_content.c_str(), test_content.length());
        MatrixOS::File::Close(file);

        if (written == test_content.length()) {
            MLOGD("Shell", "File write test: PASS");

            // Test file read
            file = MatrixOS::File::Open(test_file, FA_READ);
            if (file) {
                char read_buffer[256];
                size_t read_bytes = MatrixOS::File::Read(file, read_buffer, sizeof(read_buffer));
                MatrixOS::File::Close(file);

                if (read_bytes == test_content.length()) {
                    string read_content(read_buffer, read_bytes);
                    if (read_content == test_content) {
                        MLOGD("Shell", "File read test: PASS");
                    } else {
                        MLOGE("Shell", "File read test: FAIL - content mismatch");
                    }
                } else {
                    MLOGE("Shell", "File read test: FAIL - read %zu bytes, expected %zu", read_bytes, test_content.length());
                }
            } else {
                MLOGE("Shell", "File read test: FAIL - could not open file for reading");
            }

            // Test file deletion
            if (MatrixOS::File::Delete(test_file)) {
                MLOGD("Shell", "File delete test: PASS");
            } else {
                MLOGE("Shell", "File delete test: FAIL");
            }
        } else {
            MLOGE("Shell", "File write test: FAIL - wrote %zu bytes, expected %zu", written, test_content.length());
        }
    } else {
        MLOGE("Shell", "File write test: FAIL - could not open file for writing");
    }

    // Test directory operations
    string test_dir = "/test_dir";
    if (MatrixOS::File::CreateDir(test_dir)) {
        MLOGD("Shell", "Directory create test: PASS");

        // Test directory deletion
        if (MatrixOS::File::Delete(test_dir)) {
            MLOGD("Shell", "Directory delete test: PASS");
        } else {
            MLOGE("Shell", "Directory delete test: FAIL");
        }
    } else {
        MLOGE("Shell", "Directory create test: FAIL");
    }

    MLOGD("Shell", "Filesystem tests completed");
#else
    MLOGW("Shell", "Filesystem not enabled - skipping tests");
#endif
}

void Shell::InitializeFolderSystem() {
  // Clear and initialize folders
  folders.clear();

  // Load folder colors
  MatrixOS::NVS::GetVariable(APP_FOLDER_COLOR_HASH_KEY, folder_colors, sizeof(folder_colors));

  // Initialize folder structures
  folders[FOLDER_HIDDEN] = {{}};
  folders[FOLDER_INVISIBLE] = {{}};
  for (uint8_t i = 0; i < FOLDER_COUNT; i++) {
    folders[i] = {{}};
  }

  // Try to load existing folder vectors from NVS
  bool folders_loaded = false;
  for (uint8_t i = 0; i < FOLDER_COUNT; i++) {
    LoadFolderVector(i);
    if (!folders[i].app_ids.empty()) {
      folders_loaded = true;
    }
  }
  LoadFolderVector(FOLDER_HIDDEN);
  LoadFolderVector(FOLDER_INVISIBLE);
  
  // Build a set of all apps that are already in folders
  std::unordered_set<uint32_t> apps_in_folders;
  for (auto& [folder_id, folder] : folders) {
    for (uint32_t app_id : folder.app_ids) {
      apps_in_folders.insert(app_id);
    }
  }
  
  // Check if any apps are missing from the folders and add them to appropriate folders
  bool missing_apps_found = false;
  for (const auto& [order_id, app_id] : application_ids) {
    auto application_it = applications.find(app_id);
    if(application_it == applications.end()) {
      continue;
    }
    
    // If app is not in any folder, add it
    if (apps_in_folders.find(app_id) == apps_in_folders.end()) {
      Application_Info* application = application_it->second;
      uint8_t folder_id = GetAppFolder(app_id, application);
      
      // Add to the appropriate folder
      folders[folder_id].app_ids.push_back(app_id);
      missing_apps_found = true;
    }
  }
  
  // Save vectors if we loaded existing ones but found missing apps, or if no folders were loaded
  if (missing_apps_found || !folders_loaded) {
    SaveAllFolderVectors();
  }
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

}

void Shell::MoveAppToFolder(uint32_t app_id, uint8_t folder_id) {
  // Check if folder exists
  if (folders.find(folder_id) == folders.end()) {
    MLOGW("Shell", "Folder %d does not exist. Cannot move app %d.", folder_id, app_id);
    return;
  }
  else if(folder_id < FOLDER_COUNT && folder_colors[folder_id] == Color(0x000000)) {
    MLOGW("Shell", "Folder %d is not enabled. Cannot move app %d.", folder_id, app_id);
    return;
  }
  else
  {
    uint8_t old_folder_id = 255; // Track which folder we removed from
    
    // Remove app from current folder
    for (auto& [fid, folder] : folders) {
      auto it = std::find(folder.app_ids.begin(), folder.app_ids.end(), app_id);
      if (it != folder.app_ids.end()) {
        folder.app_ids.erase(it);
        old_folder_id = fid;
        break;
      }
    }

    // Add app to new folder
    folders[folder_id].app_ids.push_back(app_id);

    // Save both affected folder vectors to NVS
    if (old_folder_id != 255) {
      SaveFolderVector(old_folder_id);
    }
    SaveFolderVector(folder_id);

  }
}

void Shell::SaveFolderVector(uint8_t folder_id) {
  if (folder_id >= FOLDER_COUNT && folder_id != FOLDER_HIDDEN && folder_id != FOLDER_INVISIBLE) {
    return;
  }
  
  std::string nvs_key_str = "203 Systems-Shell-Folder-" + std::to_string(folder_id) + "-Apps";
  uint32_t nvs_key = StringHash(nvs_key_str);
  std::vector<uint32_t>& app_vector = folders[folder_id].app_ids;
  
  if (!app_vector.empty()) {
    MatrixOS::NVS::SetVariable(nvs_key, app_vector.data(), app_vector.size() * sizeof(uint32_t));
  } else {
    MatrixOS::NVS::DeleteVariable(nvs_key);
  }
}

void Shell::LoadFolderVector(uint8_t folder_id) {
  if (folder_id >= FOLDER_COUNT && folder_id != FOLDER_HIDDEN && folder_id != FOLDER_INVISIBLE) {
    return;
  }
  
  std::string nvs_key_str = "203 Systems-Shell-Folder-" + std::to_string(folder_id) + "-Apps";
  uint32_t nvs_key = StringHash(nvs_key_str);
  
  int stored_size_int = MatrixOS::NVS::GetSize(nvs_key);
  
  if (stored_size_int > 0 && stored_size_int % sizeof(uint32_t) == 0) {
    size_t stored_size = stored_size_int;
    size_t num_apps = stored_size / sizeof(uint32_t);
    
    if (num_apps > 100) {
      return;
    }
    
    std::vector<uint32_t> stored_apps(num_apps);
    
    if (MatrixOS::NVS::GetVariable(nvs_key, stored_apps.data(), stored_size) >= 0) {
      folders[folder_id].app_ids = stored_apps;
    }
  }
}

void Shell::SaveAllFolderVectors() {
  for (uint8_t i = 0; i < FOLDER_COUNT; i++) {
    SaveFolderVector(i);
  }
  SaveFolderVector(FOLDER_HIDDEN);
  SaveFolderVector(FOLDER_INVISIBLE);
}

uint8_t Shell::GetAppFolder(uint32_t app_id, Application_Info* app_info) {
  // Check if app is invisible first
  if (!app_info->visibility) {
    return FOLDER_INVISIBLE;
  }

  // Check if app is already in a folder by searching vectors
  for (auto& [folder_id, folder] : folders) {
    auto it = std::find(folder.app_ids.begin(), folder.app_ids.end(), app_id);
    if (it != folder.app_ids.end()) {
      return folder_id;
    }
  }

  // App not found in any folder, default to folder 0
  return 0;
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

    for (uint16_t i = 0; i < MatrixOS::LED::GetLEDCount(); i++)
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