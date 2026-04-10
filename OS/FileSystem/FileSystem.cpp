#include "FileSystem.h"
#include "Device.h"
#include "MatrixOS.h"
#include "Application.h"
#include "System/System.h"

namespace MatrixOS::FileSystem
{
static FATFS fs;
static bool filesystemMounted = false;

static bool HasParentTraversal(const string& path) {
  size_t segmentStart = 0;
  while (segmentStart <= path.size())
  {
    size_t segmentEnd = path.find('/', segmentStart);
    if (segmentEnd == string::npos)
    {
      segmentEnd = path.size();
    }

    if (segmentEnd > segmentStart && path.substr(segmentStart, segmentEnd - segmentStart) == "..")
    {
      return true;
    }

    if (segmentEnd == path.size())
    {
      break;
    }
    segmentStart = segmentEnd + 1;
  }
  return false;
}

// Helper function to get app sandbox path
string GetAppSandboxPath() {
  if (MatrixOS::SYS::activeAppInfo)
  {
    return "/MatrixOS/AppData/" + MatrixOS::SYS::activeAppInfo->author + "-" + MatrixOS::SYS::activeAppInfo->name;
  }
  return "";
}

// Path translation for sandboxing
string TranslatePath(const string& path) {
  if (path.empty())
  {
    return "";
  }

  // Check for privilege escalation attempts
  if (path.substr(0, 2) == "//" || path.substr(0, 8) == "rootfs:/")
  {
    // Only allow system/privileged tasks to access these paths
    if (!MatrixOS::SYS::IsTaskPrivileged())
    {
      return ""; // Deny access
    }
    // Remove prefix for system access
    if (path.substr(0, 2) == "//")
    {
      string result = path.substr(2);
      return result;
    }
    if (path.substr(0, 8) == "rootfs:/")
    {
      string result = path.substr(8);
      return result;
    }
  }

  // Sandbox application paths
  if (MatrixOS::SYS::activeAppInfo && xTaskGetCurrentTaskHandle() == MatrixOS::SYS::activeAppTask)
  {
    if (HasParentTraversal(path))
    {
      return "";
    }

    string sandboxBase = GetAppSandboxPath();
    string result;
    if (path[0] == '/')
    {
      result = sandboxBase + path;
    }
    else
    {
      result = sandboxBase + "/" + path;
    }
    return result;
  }

  return "";
}

// Ensure app directory exists
void EnsureAppDirectory() {
  if (MatrixOS::SYS::activeAppInfo)
  {
    string sandboxPath = GetAppSandboxPath();

    FILINFO fno;
    FRESULT res;

    // Create directory structure
    res = f_stat("/MatrixOS", &fno);
    if (res != FR_OK)
    {
      f_mkdir("/MatrixOS");
    }

    res = f_stat("/MatrixOS/AppData", &fno);
    if (res != FR_OK)
    {
      f_mkdir("/MatrixOS/AppData");
    }

    res = f_stat(sandboxPath.c_str(), &fno);
    if (res != FR_OK)
    {
      f_mkdir(sandboxPath.c_str());
    }
  }
}

void Init() {
  if (Device::Storage::Available() && !filesystemMounted)
  {
    // Log storage status for debugging
    const Device::Storage::StorageStatus* storageStatus = Device::Storage::Status();
    MLOGI("FileSystem", "Storage available - sectors: %lu, size: %u bytes", storageStatus->sectorCount, storageStatus->sectorSize);

    FRESULT result = f_mount(&fs, "", 1);

    if (result == FR_NO_FILESYSTEM)
    {
      // Mount failed, try to format the filesystem
      MLOGW("FileSystem", "No File System, formatting storage...", result);

      // Try a simple format first (let FatFS choose the best format)
      BYTE work[FF_MAX_SS]; // Work area for f_mkfs

      MKFS_PARM mkfsOpt = {0}; // Initialize to zero
      mkfsOpt.fmt = FS_FAT32;  // Let FatFS choose format (FAT12/16/32)
      mkfsOpt.n_fat = 0;       // Number of FAT copies
      mkfsOpt.align = 0;       // Data area alignment (0 = auto)
      mkfsOpt.n_root = 0;      // Number of root directory entries (0 = auto)
      mkfsOpt.au_size = 0;     // Allocation unit size (0 = auto)

      FRESULT mkfsResult = f_mkfs("", &mkfsOpt, work, sizeof(work));

      if (mkfsResult == FR_OK)
      {
        MLOGI("FileSystem", "Format successful, mounting filesystem...");
        // Format successful, try to mount again
        result = f_mount(&fs, "", 1);
        if (result == FR_OK)
        {
          MLOGI("FileSystem", "Filesystem mounted successfully after format");
        }
        else
        {
          MLOGE("FileSystem", "Mount failed after format (code %d)", result);
        }
      }
      else
      {
        MLOGE("FileSystem", "Format failed (code %d)", mkfsResult);
      }
    }
    else
    {
      MLOGI("FileSystem", "Filesystem mounted successfully");
    }

    filesystemMounted = (result == FR_OK);
  }
  else if (!Device::Storage::Available())
  {
    MLOGW("FileSystem", "Storage not available for filesystem");
  }
}

bool Available(void) {
  return Device::Storage::Available();
}

bool Exists(const string& path) {
  string translatedPath = TranslatePath(path);
  if (translatedPath.empty())
    return false;

  FILINFO fno;
  FRESULT result = f_stat(translatedPath.c_str(), &fno);
  return (result == FR_OK);
}

bool MakeDir(const string& path) {
  string translatedPath = TranslatePath(path);
  if (translatedPath.empty())
    return false;

  EnsureAppDirectory();
  FRESULT result = f_mkdir(translatedPath.c_str());
  return (result == FR_OK);
}

File Open(const string& path, const string& mode) {
  string translatedPath = TranslatePath(path);
  if (translatedPath.empty())
    return File(); // Return invalid file

  EnsureAppDirectory();

  File file;
  file._Open(translatedPath, mode);
  return file;
}

bool Remove(const string& path) {
  string translatedPath = TranslatePath(path);
  if (translatedPath.empty())
    return false;

  FRESULT result = f_unlink(translatedPath.c_str());
  return (result == FR_OK);
}

bool RemoveDir(const string& path) {
  string translatedPath = TranslatePath(path);
  if (translatedPath.empty())
    return false;

  FRESULT result = f_unlink(translatedPath.c_str()); // f_unlink works for empty dirs too
  return (result == FR_OK);
}

bool Rename(const string& from, const string& to) {
  string translatedFrom = TranslatePath(from);
  string translatedTo = TranslatePath(to);
  if (translatedFrom.empty() || translatedTo.empty())
    return false;
  EnsureAppDirectory();
  FRESULT result = f_rename(translatedFrom.c_str(), translatedTo.c_str());
  return (result == FR_OK);
}

vector<string> ListDir(const string& path) {
  vector<string> files;
  string translatedPath = TranslatePath(path);
  if (translatedPath.empty())
    return files;

  DIR dir;
  FILINFO fno;

  FRESULT result = f_opendir(&dir, translatedPath.c_str());
  if (result != FR_OK)
    return files;

  while (true)
  {
    result = f_readdir(&dir, &fno);
    if (result != FR_OK || fno.fname[0] == 0)
      break; // End of directory

    files.push_back(string(fno.fname));
  }

  f_closedir(&dir);
  return files;
}
} // namespace MatrixOS::FileSystem
