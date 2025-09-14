#include "File.h"
#include "Device.h"
#include "MatrixOS.h"

namespace MatrixOS::File
{
  static FATFS fs;
  static bool filesystem_mounted = false;

  bool Available()
  {
#if DEVICE_STORAGE == 1
    // Check if device storage is available
    const Device::Storage::Status* status = Device::Storage::GetStatus();
    return status->available;
#else
    return false;
#endif
  }

  static bool EnsureMounted()
  {
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
        return false;
      }
    }
    return true;
  }

  Handle* Open(string path, uint8_t mode)
  {
    if (!Available() || !EnsureMounted())
    {
      return nullptr;
    }

    Handle* file = new Handle;
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

  bool Close(Handle* file)
  {
    if (!file) return false;

    FRESULT res = f_close(file);
    delete file;

    return (res == FR_OK);
  }

  size_t Read(Handle* file, void* buffer, size_t length)
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

  size_t Write(Handle* file, const void* buffer, size_t length)
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
    if (!Available() || !EnsureMounted())
    {
      return false;
    }

    FILINFO fno;
    FRESULT res = f_stat(path.c_str(), &fno);
    return (res == FR_OK);
  }

  bool Delete(string path)
  {
    if (!Available() || !EnsureMounted())
    {
      return false;
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
    if (!Available() || !EnsureMounted())
    {
      return false;
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

    if (!Available() || !EnsureMounted())
    {
      return result;
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