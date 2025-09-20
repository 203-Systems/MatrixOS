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
  string GetAppSandboxPath() {
    if (MatrixOS::SYS::active_app_info) {
      return "/MatrixOS/AppData/" + MatrixOS::SYS::active_app_info->author + "-" + MatrixOS::SYS::active_app_info->name;
    }
    return "";
  }

  // Path translation for sandboxing
  string TranslatePath(const string& path) {
    // Check for privilege escalation attempts
    if (path.substr(0, 2) == "//" || path.substr(0, 8) == "rootfs:/") {
      // Only allow system/privileged tasks to access these paths
      if (!MatrixOS::SYS::IsTaskPrivileged()) {
        return ""; // Deny access
      }
      // Remove prefix for system access
      if (path.substr(0, 2) == "//") {
        string result = path.substr(2);
        return result;
      }
      if (path.substr(0, 8) == "rootfs:/") {
        string result = path.substr(8);
        return result;
      }
    }

    // Sandbox application paths
    if (MatrixOS::SYS::active_app_info && xTaskGetCurrentTaskHandle() == MatrixOS::SYS::active_app_task) {
      string sandbox_base = GetAppSandboxPath();
      string result;
      if (path[0] == '/') {
        result = sandbox_base + path;
      } else {
        result = sandbox_base + "/" + path;
      }
      return result;
    }

    return "";
  }

  // Ensure app directory exists
  void EnsureAppDirectory() {
    if (MatrixOS::SYS::active_app_info) {
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
      // Log storage status for debugging
      const Device::Storage::StorageStatus* storage_status = Device::Storage::Status();
      MLOGI("FileSystem", "Storage available - sectors: %lu, size: %u bytes",
            storage_status->sector_count, storage_status->sector_size);

      FRESULT result = f_mount(&fs, "", 1);

      if (result == FR_NO_FILESYSTEM) {
        // Mount failed, try to format the filesystem
        MLOGW("FileSystem", "No File System, formatting storage...", result);

        // Try a simple format first (let FatFS choose the best format)
        BYTE work[FF_MAX_SS]; // Work area for f_mkfs

        MKFS_PARM mkfs_opt = {0}; // Initialize to zero
        mkfs_opt.fmt = FS_FAT32;    // Let FatFS choose format (FAT12/16/32)
        mkfs_opt.n_fat = 0;       // Number of FAT copies
        mkfs_opt.align = 0;       // Data area alignment (0 = auto)
        mkfs_opt.n_root = 0;      // Number of root directory entries (0 = auto)
        mkfs_opt.au_size = 0;     // Allocation unit size (0 = auto)

        FRESULT mkfs_result = f_mkfs("", &mkfs_opt, work, sizeof(work));

        if (mkfs_result == FR_OK) {
          MLOGI("FileSystem", "Format successful, mounting filesystem...");
          // Format successful, try to mount again
          result = f_mount(&fs, "", 1);
          if (result == FR_OK) {
            MLOGI("FileSystem", "Filesystem mounted successfully after format");
          } else {
            MLOGE("FileSystem", "Mount failed after format (code %d)", result);
          }
        } else {
          MLOGE("FileSystem", "Format failed (code %d)", mkfs_result);
        }
      } else {
        MLOGI("FileSystem", "Filesystem mounted successfully");
      }

      filesystem_mounted = (result == FR_OK);
    } else if (!Device::Storage::Available()) {
      MLOGW("FileSystem", "Storage not available for filesystem");
    }
  }

  bool Available(void)
  {
    return Device::Storage::Available();
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

  File Open(const string& path, const string& mode) {
    string translated_path = TranslatePath(path);
    if (translated_path.empty()) return File(); // Return invalid file

    EnsureAppDirectory();

    File file;
    file._Open(translated_path, mode);
    return file;
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