#pragma once

#include "MatrixOS.h"
#include "ui/UI.h"
#include "Application.h"

class Shell : public Application {
  public:
  inline static Application_Info info = {
      .name = "Shell",
      .author = "203 Systems",
      .color = Color(0x00FFAA),
      .version = 1,
      .visibility = false,
  };

  // Folder system constants
  static constexpr uint8_t FOLDER_COUNT = 6;  // Folders 0-5
  static constexpr uint8_t FOLDER_HIDDEN = 254;  // User hidden apps folder
  static constexpr uint8_t FOLDER_INVISIBLE = 255;  // System invisible apps folder
  
  struct Folder
  {
    std::vector<uint32_t> app_ids;
  };

  Color folder_colors[FOLDER_COUNT] = { // If color is not set, that means folder is not created
      Color(0x00FFFF), Color(0x000000), Color(0x000000),
      Color(0x000000), Color(0x000000), Color(0x000000)
  };

  // Folder definitions (0-5)
  std::unordered_map<uint8_t, Folder> folders;

  uint8_t current_folder = 0; // Start with folder 0
  uint32_t selected_app_id = 0; // Currently selected app in edit mode (0 = none)

  void Setup() override;
  void Loop() override;

  // Folder system functions
  void InitializeFolderSystem();
  uint8_t GetAppFolder(uint32_t app_id, Application_Info* app_info);
  bool EnableFolder(uint8_t folder_idx, Color color);
  void DisableFolder(uint8_t folder_id);
  void DeleteFolder(uint8_t folder_id);
  void MoveAppToFolder(uint32_t app_id, uint8_t folder_id);
  
  // Helper functions for NVS
  void SaveFolderVector(uint8_t folder_id);
  void LoadFolderVector(uint8_t folder_id);
  void SaveAllFolderVectors();
  
  // Application launcher functions
  void ApplicationLauncher();
  void ApplicationLauncherEditing();
  void HiddenApplicationLauncher();
  void LaunchAnimation(Point origin, Color color);
};


