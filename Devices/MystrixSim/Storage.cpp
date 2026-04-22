#include "Device.h"

#if DEVICE_STORAGE == 1

#include "FileSystem/FatFS/ff.h"
#include "MatrixOS.h"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/em_asm.h>
#endif

namespace
{
constexpr uint32_t kStorageSectorSize = 512;
constexpr uint32_t kStorageSectorCount = 4096;
constexpr uint32_t kStorageImageSize = kStorageSectorSize * kStorageSectorCount;
constexpr const char* kStorageTag = "MystrixSimStorage";

std::vector<uint8_t> storageImage(kStorageImageSize, 0);
std::mutex storageMutex;
bool storageInitialized = false;
Device::Storage::StorageStatus storageStatus = {
    false,
    false,
    kStorageSectorCount,
    kStorageSectorSize,
    1,
};

std::string wasmFsListJson;
std::vector<uint8_t> wasmFsReadBuffer;

bool HasParentTraversal(const std::string& path) {
  size_t segmentStart = 0;
  while (segmentStart <= path.size())
  {
    size_t segmentEnd = path.find('/', segmentStart);
    if (segmentEnd == std::string::npos)
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

std::string JsonEscape(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size() + 8);
  for (char character : value)
  {
    switch (character)
    {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += character;
        break;
    }
  }
  return escaped;
}

std::string NormalizeFsPath(const char* rawPath) {
  std::string path = rawPath ? std::string(rawPath) : "/";
  if (path.empty())
  {
    path = "/";
  }

  if (path.rfind("rootfs:/", 0) == 0)
  {
    path = path.substr(7);
  }
  else if (path.rfind("//", 0) == 0)
  {
    path = path.substr(1);
  }

  if (path.empty())
  {
    path = "/";
  }

  if (path[0] != '/')
  {
    path.insert(path.begin(), '/');
  }

  if (HasParentTraversal(path))
  {
    return "";
  }

  std::vector<std::string> segments;
  size_t segmentStart = 0;
  while (segmentStart <= path.size())
  {
    size_t segmentEnd = path.find('/', segmentStart);
    if (segmentEnd == std::string::npos)
    {
      segmentEnd = path.size();
    }

    if (segmentEnd > segmentStart)
    {
      std::string segment = path.substr(segmentStart, segmentEnd - segmentStart);
      if (segment != ".")
      {
        segments.push_back(segment);
      }
    }

    if (segmentEnd == path.size())
    {
      break;
    }
    segmentStart = segmentEnd + 1;
  }

  if (segments.empty())
  {
    return "/";
  }

  std::string normalized = "/";
  for (size_t i = 0; i < segments.size(); ++i)
  {
    normalized += segments[i];
    if (i + 1 < segments.size())
    {
      normalized += "/";
    }
  }
  return normalized;
}

bool EnsureDirectoryTree(const std::string& fullPath) {
  size_t offset = 1;
  while (true)
  {
    offset = fullPath.find('/', offset);
    if (offset == std::string::npos)
    {
      break;
    }

    std::string directory = fullPath.substr(0, offset);
    FILINFO info = {};
    FRESULT status = f_stat(directory.c_str(), &info);
    if (status == FR_OK)
    {
      if ((info.fattrib & AM_DIR) == 0)
      {
        return false;
      }
    }
    else if (f_mkdir(directory.c_str()) != FR_OK)
    {
      return false;
    }

    offset += 1;
  }

  return true;
}

bool EnsureStorageLoaded() {
  if (storageInitialized)
  {
    return true;
  }

#ifdef __EMSCRIPTEN__
  int loadedBytes = MAIN_THREAD_EM_ASM_INT({
    try {
      if (!window._matrixos_fs_bridge || typeof window._matrixos_fs_bridge.loadImage !== 'function') return -1;
      return window._matrixos_fs_bridge.loadImage($0, $1) | 0;
    } catch (error) {
      console.error('[MystrixSimStorage] Failed to load browser storage image:', error);
      return -1;
    }
  }, storageImage.data(), static_cast<int>(storageImage.size()));
  if (loadedBytes < 0)
  {
    MatrixOS::Logging::LogWarning(kStorageTag, "Browser storage load failed, using blank image");
  }
  else
  {
    MatrixOS::Logging::LogInfo(kStorageTag, "Loaded %d bytes from browser storage image", loadedBytes);
  }
#endif

  storageStatus.available = true;
  storageStatus.writeProtected = false;
  storageInitialized = true;
  return true;
}

void ScheduleStoragePersist() {
#ifdef __EMSCRIPTEN__
  MAIN_THREAD_ASYNC_EM_ASM({
    try {
      if (window._matrixos_fs_bridge && typeof window._matrixos_fs_bridge.scheduleSave === 'function') {
        window._matrixos_fs_bridge.scheduleSave($0, $1);
      }
    } catch (error) {
      console.error('[MystrixSimStorage] Failed to schedule browser storage save:', error);
    }
  }, storageImage.data(), static_cast<int>(storageImage.size()));
#endif
}

std::string BuildFsListJson(const std::string& normalizedPath) {
  DIR directory = {};
  FILINFO entryInfo = {};
  FRESULT result = f_opendir(&directory, normalizedPath.c_str());
  if (result != FR_OK)
  {
    return "{\"ok\":false,\"error\":\"Unable to open directory\",\"path\":\"" + JsonEscape(normalizedPath) + "\"}";
  }

  struct Entry {
    std::string name;
    std::string path;
    bool isDir;
    uint32_t size;
  };

  std::vector<Entry> entries;
  while (true)
  {
    result = f_readdir(&directory, &entryInfo);
    if (result != FR_OK || entryInfo.fname[0] == 0)
    {
      break;
    }

    std::string name = entryInfo.fname;
    if (name == "." || name == "..")
    {
      continue;
    }

    bool isDir = (entryInfo.fattrib & AM_DIR) != 0;
    std::string path = normalizedPath == "/" ? "/" + name : normalizedPath + "/" + name;
    entries.push_back({name, path, isDir, static_cast<uint32_t>(entryInfo.fsize)});
  }

  f_closedir(&directory);

  std::sort(entries.begin(), entries.end(), [](const Entry& lhs, const Entry& rhs) {
    if (lhs.isDir != rhs.isDir)
    {
      return lhs.isDir > rhs.isDir;
    }
    return lhs.name < rhs.name;
  });

  std::string json = "{\"ok\":true,\"path\":\"" + JsonEscape(normalizedPath) + "\",\"entries\":[";
  for (size_t i = 0; i < entries.size(); ++i)
  {
    const Entry& entry = entries[i];
    json += "{\"name\":\"" + JsonEscape(entry.name) + "\",";
    json += "\"path\":\"" + JsonEscape(entry.path) + "\",";
    json += "\"isDir\":" + std::string(entry.isDir ? "true" : "false") + ",";
    json += "\"size\":" + std::to_string(entry.size) + "}";
    if (i + 1 < entries.size())
    {
      json += ",";
    }
  }
  json += "]}";
  return json;
}
} // namespace

namespace Device::Storage
{
void Init() {
  std::lock_guard<std::mutex> lock(storageMutex);
  (void)EnsureStorageLoaded();
}

bool Available() {
  std::lock_guard<std::mutex> lock(storageMutex);
  return EnsureStorageLoaded() && storageStatus.available;
}

const StorageStatus* Status() {
  std::lock_guard<std::mutex> lock(storageMutex);
  (void)EnsureStorageLoaded();
  return &storageStatus;
}

bool ReadSectors(uint32_t lba, uint32_t sectorCount, void* dest) {
  if (dest == nullptr || sectorCount == 0)
  {
    return false;
  }

  std::lock_guard<std::mutex> lock(storageMutex);
  if (!EnsureStorageLoaded())
  {
    return false;
  }

  const uint64_t offset = static_cast<uint64_t>(lba) * kStorageSectorSize;
  const uint64_t byteCount = static_cast<uint64_t>(sectorCount) * kStorageSectorSize;
  if (offset + byteCount > storageImage.size())
  {
    return false;
  }

  memcpy(dest, storageImage.data() + offset, static_cast<size_t>(byteCount));
  return true;
}

bool WriteSectors(uint32_t lba, uint32_t sectorCount, const void* src) {
  if (src == nullptr || sectorCount == 0)
  {
    return false;
  }

  {
    std::lock_guard<std::mutex> lock(storageMutex);
    if (!EnsureStorageLoaded())
    {
      return false;
    }

    const uint64_t offset = static_cast<uint64_t>(lba) * kStorageSectorSize;
    const uint64_t byteCount = static_cast<uint64_t>(sectorCount) * kStorageSectorSize;
    if (offset + byteCount > storageImage.size())
    {
      return false;
    }

    memcpy(storageImage.data() + offset, src, static_cast<size_t>(byteCount));
  }

  ScheduleStoragePersist();
  return true;
}
} // namespace Device::Storage

extern "C"
{
uint8_t MatrixOS_Wasm_FsMounted(void) {
  if (!Device::Storage::Available())
  {
    return 0;
  }

  DIR directory = {};
  FRESULT result = f_opendir(&directory, "/");
  if (result == FR_OK)
  {
    f_closedir(&directory);
    return 1;
  }

  return 0;
}

const char* MatrixOS_Wasm_FsListJson(const char* path) {
  const std::string normalizedPath = NormalizeFsPath(path);
  if (normalizedPath.empty())
  {
    wasmFsListJson = "{\"ok\":false,\"error\":\"Invalid path\"}";
    return wasmFsListJson.c_str();
  }

  wasmFsListJson = BuildFsListJson(normalizedPath);
  return wasmFsListJson.c_str();
}

uint8_t* MatrixOS_Wasm_FsReadFile(const char* path) {
  const std::string normalizedPath = NormalizeFsPath(path);
  if (normalizedPath.empty())
  {
    wasmFsReadBuffer.clear();
    return nullptr;
  }

  FILINFO info = {};
  if (f_stat(normalizedPath.c_str(), &info) != FR_OK || (info.fattrib & AM_DIR) != 0)
  {
    wasmFsReadBuffer.clear();
    return nullptr;
  }

  FIL file = {};
  if (f_open(&file, normalizedPath.c_str(), FA_READ) != FR_OK)
  {
    wasmFsReadBuffer.clear();
    return nullptr;
  }

  wasmFsReadBuffer.assign(info.fsize, 0);
  UINT bytesRead = 0;
  FRESULT result = FR_OK;
  if (!wasmFsReadBuffer.empty())
  {
    result = f_read(&file, wasmFsReadBuffer.data(), static_cast<UINT>(wasmFsReadBuffer.size()), &bytesRead);
  }
  f_close(&file);

  if (result != FR_OK)
  {
    wasmFsReadBuffer.clear();
    return nullptr;
  }

  wasmFsReadBuffer.resize(bytesRead);
  return wasmFsReadBuffer.empty() ? nullptr : wasmFsReadBuffer.data();
}

uint32_t MatrixOS_Wasm_FsReadFileSize(void) {
  return static_cast<uint32_t>(wasmFsReadBuffer.size());
}

uint8_t MatrixOS_Wasm_FsWriteFile(const char* path, const uint8_t* data, uint32_t length) {
  const std::string normalizedPath = NormalizeFsPath(path);
  if (normalizedPath.empty() || (data == nullptr && length > 0))
  {
    return 0;
  }

  if (!EnsureDirectoryTree(normalizedPath))
  {
    return 0;
  }

  FIL file = {};
  if (f_open(&file, normalizedPath.c_str(), FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
  {
    return 0;
  }

  UINT bytesWritten = 0;
  FRESULT result = FR_OK;
  if (length > 0)
  {
    result = f_write(&file, data, length, &bytesWritten);
  }

  if (result == FR_OK)
  {
    result = f_sync(&file);
  }
  f_close(&file);

  return (result == FR_OK && bytesWritten == length) ? 1 : 0;
}

uint8_t MatrixOS_Wasm_FsDelete(const char* path) {
  const std::string normalizedPath = NormalizeFsPath(path);
  if (normalizedPath.empty() || normalizedPath == "/")
  {
    return 0;
  }

  return f_unlink(normalizedPath.c_str()) == FR_OK ? 1 : 0;
}

uint8_t MatrixOS_Wasm_FsMakeDir(const char* path) {
  const std::string normalizedPath = NormalizeFsPath(path);
  if (normalizedPath.empty() || normalizedPath == "/")
  {
    return 0;
  }

  if (!EnsureDirectoryTree(normalizedPath + "/placeholder"))
  {
    return 0;
  }

  FILINFO info = {};
  if (f_stat(normalizedPath.c_str(), &info) == FR_OK)
  {
    return (info.fattrib & AM_DIR) != 0 ? 1 : 0;
  }

  return f_mkdir(normalizedPath.c_str()) == FR_OK ? 1 : 0;
}
}

#endif
