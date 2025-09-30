#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include "Application.h"
#include "PythonAppDiscovery.h"

// Shell-specific application management structures
enum class ApplicationType : uint8_t {
    Native = 0,
    Python = 1
};

struct ApplicationEntry {
  ApplicationType type;

  union {
    struct {
      Application_Info* info;  // Points to Application_Info regardless of type
    } native;
    struct {
      PythonAppDiscovery::PythonAppInfo* info;  // Points to PythonAppInfo for Python apps
    } python;
  };

  ApplicationEntry(Application_Info* native_app_info)
    : type(ApplicationType::Native) {
      native.info = native_app_info;
    }

  ApplicationEntry(PythonAppDiscovery::PythonAppInfo* py_app_info)
    : type(ApplicationType::Python) {
      python.info = py_app_info;
    }
};

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

  // Storage for Python app infos (to keep Application_Info objects alive)
  std::vector<PythonAppDiscovery::PythonAppInfo> python_app_infos;

  // Shell's view of all applications (native + Python)
  std::unordered_map<uint32_t, ApplicationEntry> all_applications;

  uint8_t current_folder = 0; // Start with folder 0
  uint32_t selected_app_id = 0; // Currently selected app in edit mode (0 = none)

  void Setup(const vector<string>& args) override;
  void Loop() override;

  // Folder system functions
  void InitializeFolderSystem();
  void TestFileSystem();
  uint8_t GetAppFolder(uint32_t app_id, const ApplicationEntry& app_entry);
  bool EnableFolder(uint8_t folder_idx, Color color);
  void DisableFolder(uint8_t folder_id);
  void DeleteFolder(uint8_t folder_id);
  void MoveAppToFolder(uint32_t app_id, uint8_t folder_id);

  // Python application discovery
  void DiscoverPythonApps();
  
  // Helper functions for NVS
  void SaveFolderVector(uint8_t folder_id);
  void LoadFolderVector(uint8_t folder_id);
  void SaveAllFolderVectors();
  void CleanupInvalidApps();
  
  // Application launcher functions
  void ApplicationLauncher();
  void ApplicationLauncherEditing();
  void HiddenApplicationLauncher();
  void LaunchAnimation(Point origin, Color color);
};


