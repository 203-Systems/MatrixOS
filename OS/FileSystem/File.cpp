#include "File.h"
#include "Device.h"
#include "MatrixOS.h"
#include "Application.h"
#include "../System/System.h"

// Path translation constants
#define ROOTFS_PREFIX "rootfs:/"
#define ROOTFS_SHORTHAND "//"

static bool starts_with(const string& str, const string& prefix) {
  return str.length() >= prefix.length() && str.substr(0, prefix.length()) == prefix;
}

namespace MatrixOS::File
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

  // Ensure app directory exists (lazy creation)
  static void EnsureAppDirectory() {
    if (!MatrixOS::SYS::IsTaskPrivileged() && MatrixOS::SYS::active_app_info) {
      string sandbox_path = GetAppSandboxPath();

      // Create directories if they don't exist
      // Note: We check and create each level separately to avoid issues
      FILINFO fno;

      // Check/create /MatrixOS
      if (f_stat("/MatrixOS", &fno) != FR_OK) {
        f_mkdir("/MatrixOS");
      }

      // Check/create /MatrixOS/AppData
      if (f_stat("/MatrixOS/AppData", &fno) != FR_OK) {
        f_mkdir("/MatrixOS/AppData");
      }

      // Check/create app-specific directory
      if (f_stat(sandbox_path.c_str(), &fno) != FR_OK) {
        f_mkdir(sandbox_path.c_str());
      }
    }
  }

  // Translate paths based on privilege and check access
  static string TranslatePath(const string& path) {
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    bool is_privileged = MatrixOS::SYS::IsTaskPrivileged();
    bool is_app_task = (current_task == MatrixOS::SYS::active_app_task);

    // First check if task has filesystem access
    if (!is_privileged && !is_app_task) {
      MLOGE("File", "Filesystem access denied for non-privileged non-app task");
      return "";  // Empty string indicates access denied
    }

    // Check for root filesystem access (privileged only)
    if (starts_with(path, ROOTFS_SHORTHAND)) {
      if (is_privileged) {
        return path.substr(2);  // Remove "//" prefix
      }
      // Non-privileged: deny rootfs access
      MLOGE("File", "Rootfs access denied for non-privileged task: %s", path.c_str());
      return "";
    }
    else if (starts_with(path, ROOTFS_PREFIX)) {
      if (is_privileged) {
        return path.substr(7);  // Remove "rootfs:/" prefix
      }
      // Non-privileged: deny rootfs access
      MLOGE("File", "Rootfs access denied for non-privileged task: %s", path.c_str());
      return "";
    }

    // Apply sandboxing for app tasks (both privileged and non-privileged)
    if (is_app_task) {
      // Ensure app directory exists
      EnsureAppDirectory();

      string sandbox = GetAppSandboxPath();

      if (path.empty() || path == "/") {
        return sandbox + "/";
      } else if (path[0] == '/') {
        return sandbox + path;  // /file.txt → /MatrixOS/AppData/{app}/file.txt
      } else {
        return sandbox + "/" + path;  // file.txt → /MatrixOS/AppData/{app}/file.txt
      }
    }

    // System tasks must use rootfs prefixes for filesystem access
    MLOGE("File", "System task must use rootfs prefix for filesystem access: %s", path.c_str());
    return "";  // Deny access
  }

  void Init()
  {
#if DEVICE_STORAGE == 1
    if (!filesystem_mounted)
    {
      FRESULT res = f_mount(&fs, "0:", 1);  // Mount immediately
      if (res == FR_OK)
      {
        filesystem_mounted = true;
        MLOGI("File", "Filesystem mounted successfully");
      }
      else
      {
        MLOGE("File", "Failed to mount filesystem: %d", res);
      }
    }

    if(filesystem_mounted)
    {
      // Check/create /MatrixOS
      FILINFO fno;
      if (f_stat("/MatrixOS", &fno) != FR_OK) {
        f_mkdir("/MatrixOS");
      }
    }
#endif
  }

  bool Available()
  {
#if DEVICE_STORAGE == 1
    // Check if device storage is available and filesystem is mounted
    const Device::Storage::StorageStatus* status = Device::Storage::Status();
    return status->available && filesystem_mounted;
#else
    return false;
#endif
  }

  File* Open(string path, uint8_t mode)
  {
    if (!Available())
    {
      return nullptr;
    }

    // Translate path and check access (returns empty string if denied)
    path = TranslatePath(path);
    if (path.empty()) {
      return nullptr;  // Access denied
    }

    File* file = new File;
    FRESULT res = f_open(file, path.c_str(), mode);

    if (res == FR_OK)
    {
      return file;
    }
    else
    {
      MLOGE("File", "Failed to open file '%s': %d", path.c_str(), res);
      delete file;
      return nullptr;
    }
  }

  bool Close(File* file)
  {
    if (!file) return false;

    FRESULT res = f_close(file);
    delete file;

    return (res == FR_OK);
  }

  size_t Read(File* file, void* buffer, size_t length)
  {
    if (!file || !buffer) return 0;

    UINT bytes_read = 0;
    FRESULT res = f_read(file, buffer, length, &bytes_read);

    if (res != FR_OK)
    {
      MLOGE("File", "File read error: %d", res);
      return 0;
    }

    return bytes_read;
  }

  size_t Write(File* file, const void* buffer, size_t length)
  {
    if (!file || !buffer) return 0;

    UINT bytes_written = 0;
    FRESULT res = f_write(file, buffer, length, &bytes_written);

    if (res != FR_OK)
    {
      MLOGE("File", "File write error: %d", res);
      return 0;
    }

    return bytes_written;
  }

  bool Exists(string path)
  {
    if (!Available())
    {
      return false;
    }

    // Translate path and check access
    path = TranslatePath(path);
    if (path.empty()) {
      return false;  // Access denied
    }

    FILINFO fno;
    FRESULT res = f_stat(path.c_str(), &fno);
    return (res == FR_OK);
  }

  bool Delete(string path)
  {
    if (!Available())
    {
      return false;
    }

    // Translate path and check access
    path = TranslatePath(path);
    if (path.empty()) {
      return false;  // Access denied
    }

    FRESULT res = f_unlink(path.c_str());
    if (res != FR_OK)
    {
      MLOGE("File", "Failed to delete '%s': %d", path.c_str(), res);
      return false;
    }
    return true;
  }

  bool CreateDir(string path)
  {
    if (!Available())
    {
      return false;
    }

    // Translate path and check access
    path = TranslatePath(path);
    if (path.empty()) {
      return false;  // Access denied
    }

    FRESULT res = f_mkdir(path.c_str());
    if (res != FR_OK && res != FR_EXIST)
    {
      MLOGE("File", "Failed to create directory '%s': %d", path.c_str(), res);
      return false;
    }
    return true;
  }

  vector<string> ListDir(string path)
  {
    vector<string> result;

    if (!Available())
    {
      return result;
    }

    // Translate path and check access
    path = TranslatePath(path);
    if (path.empty()) {
      return result;  // Access denied - return empty vector
    }

    DIR dir;
    FILINFO fno;

    FRESULT res = f_opendir(&dir, path.c_str());
    if (res != FR_OK)
    {
      MLOGE("File", "Failed to open directory '%s': %d", path.c_str(), res);
      return result;
    }

    while (true)
    {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) break;  // End of directory

      result.push_back(string(fno.fname));
    }

    f_closedir(&dir);
    return result;
  }
}