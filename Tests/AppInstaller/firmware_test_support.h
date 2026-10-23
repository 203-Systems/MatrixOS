#pragma once
// Host-only adapters. Tests execute the production installer state machine and
// commit/recovery code against an isolated directory instead of a microSD card.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#include "ArduinoJson.h"
using std::string; using std::vector;
#define DEVICE_STORAGE 1
#define OS_SHELL 123
#define APPID(author, name) 501
#define USB_MODE_NORMAL 0
#define USB_MODE_MSC 1
#define configMINIMAL_STACK_SIZE 1024
#define portMAX_DELAY 0xFFFFFFFF
#define pdTRUE 1
#define pdMS_TO_TICKS(x) (x)
using StaticTask_t = int; using StaticSemaphore_t = int; using StackType_t = uint32_t;
using QueueHandle_t = void*; using SemaphoreHandle_t = void*;
inline void* xSemaphoreCreateMutexStatic(int*) { return reinterpret_cast<void*>(1); }
inline void xSemaphoreTake(void*, uint32_t) {}
inline void xSemaphoreGive(void*) {}
inline void* xSemaphoreCreateRecursiveMutexStatic(int* p) { return xSemaphoreCreateMutexStatic(p); }
inline void xSemaphoreTakeRecursive(void*, uint32_t) {}
inline void xSemaphoreGiveRecursive(void*) {}
inline void* xQueueCreate(unsigned, size_t) { return reinterpret_cast<void*>(1); }
inline int xQueueSend(void*, const void*, unsigned) { return 1; }
inline int xQueueReceive(void*, void*, unsigned) { return 0; }
inline void* xTaskCreateStatic(void(*)(void*), const char*, size_t, void*, unsigned, void*, void*) { return reinterpret_cast<void*>(1); }
namespace fs = std::filesystem;
inline string testRoot, failRenameFrom;
using UINT = unsigned; using DWORD = uint32_t; using FRESULT = int;
constexpr int FR_OK=0, FR_NO_FILE=1, FR_NO_PATH=2, FR_EXIST=3, FR_DISK_ERR=4;
constexpr unsigned AM_DIR=16, FA_READ=1, FA_WRITE=2, FA_CREATE_ALWAYS=4, FA_CREATE_NEW=8;
struct FIL { std::FILE* file = nullptr; };
struct FILINFO { unsigned fattrib = 0; char fname[256]{}; };
struct FATFS { unsigned csize=8; };
struct DIR { vector<fs::path> entries; size_t index=0; };
inline string Physical(const string& path) { return testRoot + path; }
inline FRESULT f_stat(const char* p, FILINFO* info) {
  std::error_code e; auto st=fs::status(Physical(p), e);
  if (e || !fs::exists(st)) return FR_NO_FILE;
  info->fattrib=fs::is_directory(st)?AM_DIR:0; return FR_OK;
}
inline FRESULT f_mkdir(const char* p) { std::error_code e; bool b=fs::create_directory(Physical(p), e); return b?FR_OK:(!e?FR_EXIST:FR_DISK_ERR); }
inline FRESULT f_unlink(const char* p) { std::error_code e; bool b=fs::remove(Physical(p), e); return b && !e?FR_OK:FR_DISK_ERR; }
inline FRESULT f_rename(const char* a, const char* b) {
  if (failRenameFrom == a) { failRenameFrom.clear(); return FR_DISK_ERR; }
  if (fs::exists(Physical(b))) return FR_EXIST;
  std::error_code e; fs::rename(Physical(a), Physical(b), e); return e?FR_DISK_ERR:FR_OK;
}
inline FRESULT f_opendir(DIR* d, const char* p) {
  std::error_code e; fs::directory_iterator iterator(Physical(p),e);
  if (e) return FR_NO_PATH;
  for (const auto& entry:iterator) d->entries.push_back(entry.path());
  return FR_OK;
}
inline FRESULT f_readdir(DIR* d, FILINFO* info) {
  if (d->index==d->entries.size()) { info->fname[0]=0; return FR_OK; }
  auto path=d->entries[d->index++]; string n=path.filename().string();
  assert(n.size()<sizeof(info->fname)); std::strcpy(info->fname,n.c_str());
  info->fattrib=fs::is_directory(path)?AM_DIR:0; return FR_OK;
}
inline FRESULT f_closedir(DIR*) { return FR_OK; }
inline FRESULT f_open(FIL* f, const char* p, unsigned mode) {
  string path=Physical(p);
  if ((mode&FA_CREATE_NEW) && fs::exists(path)) return FR_EXIST;
  const char* flags=(mode&(FA_CREATE_ALWAYS|FA_CREATE_NEW))?"w+b":(mode&FA_WRITE)?"r+b":"rb";
  f->file=std::fopen(path.c_str(),flags); return f->file?FR_OK:FR_NO_FILE;
}
inline FRESULT f_read(FIL* f, void* data, UINT size, UINT* count) { *count=std::fread(data,1,size,f->file); return std::ferror(f->file)?FR_DISK_ERR:FR_OK; }
inline FRESULT f_write(FIL* f, const void* data, UINT size, UINT* count) { *count=std::fwrite(data,1,size,f->file); return *count==size?FR_OK:FR_DISK_ERR; }
inline FRESULT f_lseek(FIL* f, uint32_t offset) { return std::fseek(f->file,offset,SEEK_SET)?FR_DISK_ERR:FR_OK; }
inline FRESULT f_sync(FIL* f) { return std::fflush(f->file)?FR_DISK_ERR:FR_OK; }
inline FRESULT f_close(FIL* f) { int r=std::fclose(f->file); f->file=nullptr; return r?FR_DISK_ERR:FR_OK; }
inline uint32_t f_size(FIL* f) { long p=std::ftell(f->file); std::fseek(f->file,0,SEEK_END); long n=std::ftell(f->file); std::fseek(f->file,p,SEEK_SET); return n; }
inline FRESULT f_getfree(const char*, DWORD* clusters, FATFS** out) { static FATFS fs; *clusters=100000; *out=&fs; return FR_OK; }
namespace MatrixOS {
namespace Command { using ReplyCallback=bool(*)(const vector<uint8_t>&,bool,void*); }
namespace SYS {
inline uint32_t activeAppId=OS_SHELL,nextAppId=0,now=0,launches=0;
inline void ExecuteAPP(uint32_t id) { activeAppId=id; ++launches; }
inline uint32_t Millis() { return now; }
inline void DelayMs(unsigned n) { now+=n; }
}
namespace USB {
inline uint8_t mode=USB_MODE_NORMAL;
inline uint8_t GetMode() { return mode; }
}
namespace FileSystem {
inline bool available=true;
struct AccessGuard { AccessGuard() {} ~AccessGuard() {} };
inline bool Available() { return available; }
}
}
namespace PythonAppDiscovery {
struct AppInfoJson { string appMainFile; uint32_t osMinimalVer[3]{}; };
inline bool ParseAppInfo(const string& text, AppInfoJson& info) {
  JsonDocument doc; if (deserializeJson(doc,text) || !doc["appMainFile"].is<string>()) return false;
  info.appMainFile=doc["appMainFile"].as<string>(); return true;
}
inline bool ValidateVersionCompatibility(const uint32_t*) { return true; }
}
