#include "FileSystem.h"
#include "Device.h"
#include "MatrixOS.h"
#include "Application.h"
#include "System/System.h"

namespace MatrixOS::FileSystem
{
  static FATFS fs;
  static bool filesystem_mounted = false;

  // Helper function to get app sandbox path
  static string GetAppSandboxPath() {
    if (MatrixOS::SYS::active_app_info) {
      return "/MatrixOS/AppData/" + MatrixOS::SYS::active_app_info->author + "-" + MatrixOS::SYS::active_app_info->name;
    }
    return "";
  }

  // Path translation for sandboxing
  static string TranslatePath(const string& path) {
    // Check for privilege escalation attempts
    if (path.substr(0, 2) == "//" || path.substr(0, 8) == "rootfs:/") {
      // Only allow system/privileged tasks to access these paths
      if (!MatrixOS::SYS::IsTaskPrivileged()) {
        return ""; // Deny access
      }
      // Remove prefix for system access
      if (path.substr(0, 2) == "//") {
        return path.substr(2);
      }
      if (path.substr(0, 8) == "rootfs:/") {
        return path.substr(8);
      }
    }

    // Sandbox application paths
    if (MatrixOS::SYS::active_app_info && !MatrixOS::SYS::IsTaskPrivileged()) {
      string sandbox_base = GetAppSandboxPath();
      if (path[0] == '/') {
        return sandbox_base + path;
      } else {
        return sandbox_base + "/" + path;
      }
    }

    return path;
  }

  // Ensure app directory exists
  static void EnsureAppDirectory() {
    if (MatrixOS::SYS::active_app_info && !MatrixOS::SYS::IsTaskPrivileged()) {
      string sandbox_path = GetAppSandboxPath();

      FILINFO fno;
      FRESULT res;

      // Create directory structure
      res = f_stat("/MatrixOS", &fno);
      if (res != FR_OK) {
        f_mkdir("/MatrixOS");
      }

      res = f_stat("/MatrixOS/AppData", &fno);
      if (res != FR_OK) {
        f_mkdir("/MatrixOS/AppData");
      }

      res = f_stat(sandbox_path.c_str(), &fno);
      if (res != FR_OK) {
        f_mkdir(sandbox_path.c_str());
      }
    }
  }

  void Init() {
    if (Device::Storage::Available() && !filesystem_mounted) {
      FRESULT result = f_mount(&fs, "", 1);
      filesystem_mounted = (result == FR_OK);
    }
  }

  bool Exists(const string& path) {
    string translated_path = TranslatePath(path);
    if (translated_path.empty()) return false;

    FILINFO fno;
    FRESULT result = f_stat(translated_path.c_str(), &fno);
    return (result == FR_OK);
  }

  bool MakeDir(const string& path) {
    string translated_path = TranslatePath(path);
    if (translated_path.empty()) return false;

    EnsureAppDirectory();
    FRESULT result = f_mkdir(translated_path.c_str());
    return (result == FR_OK);
  }

  File* Open(const string& path, const string& mode) {
    string translated_path = TranslatePath(path);
    if (translated_path.empty()) return nullptr;

    EnsureAppDirectory();

    File* file = new File();
    if (file->_Open(translated_path, mode)) {
      return file;
    }

    delete file;
    return nullptr;
  }

  bool Remove(const string& path) {
    string translated_path = TranslatePath(path);
    if (translated_path.empty()) return false;

    FRESULT result = f_unlink(translated_path.c_str());
    return (result == FR_OK);
  }

  bool RemoveDir(const string& path) {
    string translated_path = TranslatePath(path);
    if (translated_path.empty()) return false;

    FRESULT result = f_unlink(translated_path.c_str()); // f_unlink works for empty dirs too
    return (result == FR_OK);
  }

  vector<string> ListDir(const string& path) {
    vector<string> files;
    string translated_path = TranslatePath(path);
    if (translated_path.empty()) return files;

    DIR dir;
    FILINFO fno;

    FRESULT result = f_opendir(&dir, translated_path.c_str());
    if (result != FR_OK) return files;

    while (true) {
      result = f_readdir(&dir, &fno);
      if (result != FR_OK || fno.fname[0] == 0) break; // End of directory

      files.push_back(string(fno.fname));
    }

    f_closedir(&dir);
    return files;
  }
}