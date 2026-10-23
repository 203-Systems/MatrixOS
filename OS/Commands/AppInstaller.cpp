#ifdef MATRIXOS_INSTALLER_TEST
#include "firmware_test_support.h"
#else
#include "AppInstaller.h"
#include "System/System.h"
#include "USB/USB.h"
#endif
#include "AppInstallFormat.h"
#include <cstring>
#include <atomic>
#if DEVICE_STORAGE && !defined(MATRIXOS_INSTALLER_TEST)
#include "FileSystem/FatFS/ff.h"
#include "ArduinoJson.h"
#include "../../Applications/Shell/PythonAppDiscovery.h"
#endif

namespace MatrixOS::AppInstaller
{
namespace {
using namespace AppInstallFormat;
enum Sub : uint8_t { Caps, Begin, Data, Finish, Status, Abort };
enum State : uint8_t { Idle, Receiving, Validating, Committing, Committed, Aborted, Failed };
enum Error : uint8_t { OK, Unsupported, Busy, BadPayload, BadSession, BadOffset, IO, Checksum, Exists, BadApp, Capacity, Running };
void Put32(vector<uint8_t>& v, uint32_t n) { for (int shift = 24; shift >= 0; shift -= 8) v.push_back(n >> shift); }
void Reply(const uint8_t* r, uint32_t session, uint8_t error, const vector<uint8_t>& data, Command::ReplyCallback cb) {
  vector<uint8_t> reply{0xF0, r[1], r[2], r[3]};
  Put32(reply, session); reply.push_back(error); reply.push_back(data.size());
  reply.insert(reply.end(), data.begin(), data.end());
  // Endpoint readiness is transient. Never wait here while holding the filesystem lock.
  for (unsigned i = 0; i < 100; ++i) {
    if (cb(reply, true, nullptr)) return;
    SYS::DelayMs(2);
  }
}
#if DEVICE_STORAGE
struct Request { uint8_t bytes[63]; Command::ReplyCallback callback; };
StaticSemaphore_t mutexDef;
SemaphoreHandle_t mutex = xSemaphoreCreateRecursiveMutexStatic(&mutexDef);
struct Guard { Guard() { xSemaphoreTakeRecursive(mutex, portMAX_DELAY); } ~Guard() { xSemaphoreGiveRecursive(mutex); } };
struct Snapshot { uint32_t session = 0, size = 0, checksum = 0, offset = 0, touched = 0; uint8_t state = Idle, error = OK; } snapshot;
Snapshot Get() { Guard g; return snapshot; }
void SetState(uint8_t state, uint8_t error = OK) { Guard g; snapshot.state = state; snapshot.error = error; }
const string Root = "/MatrixOS/.installer";
const string Stream = Root + "/package.bin", Stage = Root + "/stage", Backup = Root + "/backup", Journal = Root + "/commit.json";
FIL stream;
bool streamOpen = false, replace = false;
constexpr uint32_t DeveloperId = APPID("203 Systems", "Developer");
uint8_t token[16]{};
uint8_t beginArgs[25]{};
uint32_t serial = 0;
vector<uint8_t> lastRequest, lastReply;
std::atomic<bool> contextClosing{false};
struct Entry { string path; uint32_t size, checksum; };

bool Stat(const string& p, FILINFO& info) { return f_stat(p.c_str(), &info) == FR_OK; }
bool IsDir(const string& p) { FILINFO info; return Stat(p, info) && (info.fattrib & AM_DIR); }
bool Has(const string& p) { FILINFO info; return Stat(p, info); }
bool Mkdir(const string& p) { FRESULT r = f_mkdir(p.c_str()); return r == FR_OK || (r == FR_EXIST && IsDir(p)); }
bool RemoveTree(const string& path) {
  // Keep only one FatFS DIR/FILINFO on the task stack, even for nested resources.
  struct Pending { string path; unsigned depth; bool childrenDone; };
  vector<Pending> pending{{path, 0, false}};
  while (!pending.empty()) {
    Pending item = pending.back(); pending.pop_back();
    FILINFO info;
    FRESULT result = f_stat(item.path.c_str(), &info);
    if (result == FR_NO_FILE || result == FR_NO_PATH) continue;
    if (result != FR_OK || item.depth > 20) return false;
    if ((info.fattrib & AM_DIR) && !item.childrenDone) {
      pending.push_back({item.path, item.depth, true});
      DIR dir;
      if (f_opendir(&dir, item.path.c_str()) != FR_OK) return false;
      bool good = true;
      while (true) {
        if (f_readdir(&dir, &info) != FR_OK) { good = false; break; }
        if (!info.fname[0]) break;
        if (pending.size() >= 512) { good = false; break; }
        pending.push_back({item.path + "/" + info.fname, item.depth + 1, false});
      }
      if (f_closedir(&dir) != FR_OK) good = false;
      if (!good) return false;
    } else if (f_unlink(item.path.c_str()) != FR_OK) return false;
  }
  return true;
}

bool ReadExact(FIL& f, void* data, UINT size) { UINT count; return f_read(&f, data, size, &count) == FR_OK && count == size; }
bool WriteExact(FIL& f, const void* data, UINT size) { UINT count; return f_write(&f, data, size, &count) == FR_OK && count == size; }
bool CloseStream() { if (!streamOpen) return true; streamOpen = false; return f_close(&stream) == FR_OK; }
bool ReadText(const string& path, string& out, size_t limit) {
  FIL f;
  if (f_open(&f, path.c_str(), FA_READ) != FR_OK) return false;
  bool good = f_size(&f) <= limit;
  if (good) { out.resize(f_size(&f)); good = ReadExact(f, out.data(), out.size()); }
  return f_close(&f) == FR_OK && good;
}
// Called only with filesystem access held; fail closed if the journal is invalid.
bool Recover() {
  if (Has(Journal)) {
    string text;
    JsonDocument doc;
    if (!ReadText(Journal, text, 256) || deserializeJson(doc, text) || !doc["app"].is<string>() || !doc["hadOld"].is<bool>()) return false;
    string app = doc["app"].as<string>();
    if (!AppName(app)) return false;
    string dest = "/MatrixOS/Applications/" + app;
    if (Has(Stage)) { // Promotion did not complete. Keep/restore the old application.
      if (!Has(dest) && Has(Backup) && f_rename(Backup.c_str(), dest.c_str()) != FR_OK) return false;
      if (doc["hadOld"].as<bool>() && !Has(dest)) return false;
    } else if (!Has(dest)) {
      if (!Has(Backup) || f_rename(Backup.c_str(), dest.c_str()) != FR_OK) return false;
    }
    if (!RemoveTree(Stage) || !RemoveTree(Backup) || f_unlink(Journal.c_str()) != FR_OK) return false;
  }
  return RemoveTree(Stage) && RemoveTree(Backup) && RemoveTree(Stream);
}
void Release(uint8_t state, uint8_t error) {
  CloseStream();
  SetState(state, error);
}
uint8_t Start(const uint8_t* args) {
  auto previous = Get();
  if (previous.session && !memcmp(token, args, 16)) return !memcmp(beginArgs, args, 25) ? OK : BadPayload;
  if (previous.state == Receiving || previous.state == Validating || previous.state == Committing) return Busy;
  if (args[16] & ~1 || U32(args + 17) < 24 || U32(args + 17) > MaxPackage) return BadPayload;

  if (USB::GetMode() != USB_MODE_NORMAL || !FileSystem::Available()) return Busy;
  // Developer APP already provides a host-controlled context with no filesystem I/O.
  // Enter through the existing lifecycle before touching storage; End() cancels a
  // session before the next app starts. This needs no global filesystem/task hooks.
  if (SYS::activeAppId == DeveloperId && contextClosing.load()) return Busy;
  if (SYS::activeAppId != DeveloperId) {
    SYS::ExecuteAPP(DeveloperId);
    uint32_t started = SYS::Millis();
    while (SYS::activeAppId != DeveloperId && uint32_t(SYS::Millis() - started) < 2000) SYS::DelayMs(1);
    if (SYS::activeAppId != DeveloperId) return Running;
    contextClosing.store(false);
  }
  if (!Mkdir("/MatrixOS") || !Mkdir(Root) || !Recover() || !Mkdir("/MatrixOS/Applications")) return IO;
  FATFS* fs; DWORD freeClusters;
  if (f_getfree("", &freeClusters, &fs) != FR_OK) { return IO; }
  uint64_t clusterBytes = uint64_t(fs->csize) * 512;
  if (uint64_t(freeClusters) * clusterBytes < uint64_t(U32(args + 17)) * 2 + (MaxFiles + 4) * clusterBytes) { return Capacity; }
  if (f_open(&stream, Stream.c_str(), FA_CREATE_ALWAYS | FA_READ | FA_WRITE) != FR_OK) { return IO; }
  streamOpen = true;
  memcpy(token, args, 16); memcpy(beginArgs, args, 25); replace = args[16] & 1;
  { Guard g; snapshot = {}; snapshot.session = ++serial; if (!snapshot.session) snapshot.session = ++serial;
    snapshot.size = U32(args + 17); snapshot.checksum = U32(args + 21);
    snapshot.state = Receiving; snapshot.touched = SYS::Millis(); }
  return OK;
}
uint8_t Receive(const uint8_t* args, size_t size, vector<uint8_t>& reply) {
  auto s = Get();
  if (size <= 4 || s.state != Receiving) return BadPayload;
  uint32_t offset = U32(args); size_t count = size - 4;
  if (offset > s.size || count > s.size - offset) return BadOffset;

  if ((USB::GetMode() != USB_MODE_NORMAL || !FileSystem::Available() || SYS::activeAppId != DeveloperId || contextClosing.load()) || !streamOpen) return IO;
  if (offset < s.offset) {
    if (count > s.offset - offset) return BadOffset;
    uint8_t stored[50];
    if (f_lseek(&stream, offset) != FR_OK || !ReadExact(stream, stored, count)) return IO;
    if (memcmp(stored, args + 4, count)) return BadOffset;
    Put32(reply, offset + count); return OK;
  }
  if (offset != s.offset) return BadOffset;
  if (f_lseek(&stream, offset) != FR_OK || !WriteExact(stream, args + 4, count)) return IO;
  { Guard g; snapshot.offset += count; }
  Put32(reply, offset + count); return OK;
}
uint8_t ValidateAndStage(string& app) {
  // Bound buffers independently of the size of the package.
  uint8_t header[24], buffer[1024];
  uint32_t fullCRC = ~0U;
  auto s = Get();
  {

    if (f_sync(&stream) != FR_OK || f_size(&stream) != s.size || f_lseek(&stream, 0) != FR_OK) return IO;
  }
  for (uint32_t offset = 0; offset < s.size;) {
    UINT count = std::min<uint32_t>(sizeof(buffer), s.size - offset);
    { if (!ReadExact(stream, buffer, count)) return IO; }
    fullCRC = CRC(fullCRC, buffer, count); offset += count; SYS::DelayMs(1);
  }
  if (~fullCRC != s.checksum) return Checksum;
  string manifest;
  {

    if (f_lseek(&stream, 0) != FR_OK || !ReadExact(stream, header, sizeof(header))) return IO;
    if (memcmp(header, "MPKG", 4) || header[4] != 1 || header[5] || header[6] || header[7]) return BadApp;
    uint32_t msize = U32(header + 8), psize = U32(header + 12);
    if (!msize || msize > MaxManifest || uint64_t(24) + msize + psize != s.size) return BadApp;
    manifest.resize(msize);
    if (!ReadExact(stream, manifest.data(), msize)) return IO;
  }
  if (~CRC(~0U, reinterpret_cast<const uint8_t*>(manifest.data()), manifest.size()) != U32(header + 16)) return Checksum;
  JsonDocument doc;
  if (deserializeJson(doc, manifest) || !doc.is<JsonObject>() || doc.size() != 2 || !doc["app"].is<string>() || !doc["files"].is<JsonArray>()) return BadApp;
  // v1's host emits compact canonical JSON. Re-serialization also detects duplicate
  // keys and alternate escape encodings before any destination paths are used.
  string canonical; serializeJson(doc, canonical);
  if (canonical != manifest) return BadApp;
  app = doc["app"].as<string>();
  auto files = doc["files"].as<JsonArray>();
  if (!AppName(app) || !files.size() || files.size() > MaxFiles) return BadApp;
  vector<Entry> entries;
  uint64_t sum = 0;
  for (JsonVariant v : files) {
    if (!v.is<JsonObject>() || v.size() != 3 || !v["path"].is<string>() || !v["size"].is<uint32_t>() || !v["crc32"].is<uint32_t>()) return BadApp;
    Entry entry{v["path"].as<string>(), v["size"].as<uint32_t>(), v["crc32"].as<uint32_t>()};
    if (!Path(entry.path)) return BadApp;
    for (const auto& prior : entries) {
      string a = Fold(entry.path), b = Fold(prior.path);
      if (a == b || a.compare(0, b.size() + 1, b + "/") == 0 || b.compare(0, a.size() + 1, a + "/") == 0) return BadApp;
    }
    sum += entry.size; entries.push_back(entry);
  }
  if (sum != U32(header + 12)) return BadApp;
  {

    if (!Mkdir(Stage)) return IO;
    string dest = "/MatrixOS/Applications/" + app;
    if (Has(dest) && (!replace || !IsDir(dest))) return Exists;
  }
  uint32_t payloadCRC = ~0U;
  for (const auto& entry : entries) {
    FIL output;
    string path = Stage + "/" + entry.path;
    {

      for (size_t slash = path.find('/', Stage.size() + 1); slash != string::npos; slash = path.find('/', slash + 1))
        if (!Mkdir(path.substr(0, slash))) return IO;
      if (f_open(&output, path.c_str(), FA_WRITE | FA_CREATE_NEW) != FR_OK) return IO;
    }
    uint32_t fileCRC = ~0U;
    bool good = true;
    for (uint32_t offset = 0; offset < entry.size;) {
      UINT count = std::min<uint32_t>(sizeof(buffer), entry.size - offset);
      {
        if (!ReadExact(stream, buffer, count) || !WriteExact(output, buffer, count)) { good = false; break; } }
      fileCRC = CRC(fileCRC, buffer, count); payloadCRC = CRC(payloadCRC, buffer, count);
      offset += count; SYS::DelayMs(1);
    }
    { if (f_close(&output) != FR_OK) good = false; }
    if (!good) return IO;
    if (~fileCRC != entry.checksum) return Checksum;
    // Check the staged bytes from storage, not just the source stream.
    { if (f_open(&output, path.c_str(), FA_READ) != FR_OK) return IO; }
    uint32_t storedCRC = ~0U;
    for (uint32_t offset = 0; offset < entry.size;) {
      UINT count = std::min<uint32_t>(sizeof(buffer), entry.size - offset);
      { if (!ReadExact(output, buffer, count)) { good = false; break; } }
      storedCRC = CRC(storedCRC, buffer, count); offset += count; SYS::DelayMs(1);
    }
    { if (f_close(&output) != FR_OK) good = false; }
    if (!good) return IO;
    if (~storedCRC != entry.checksum) return Checksum;
  }
  if (~payloadCRC != U32(header + 20)) return Checksum;

  string infoText;
  if (!ReadText(Stage + "/AppInfo.json", infoText, MaxManifest)) return BadApp;
  JsonDocument metadata;
  if (deserializeJson(metadata, infoText) || !metadata["name"].is<string>() || !metadata["author"].is<string>() ||
      !metadata["version"].is<uint32_t>() || !metadata["color"].is<JsonArray>() || !metadata["osMinimalVer"].is<JsonArray>()) return BadApp;
  for (const char* field : {"name", "author"}) {
    string value = metadata[field].as<string>();
    if (value.empty() || value == "." || value == ".." || value.find('/') != string::npos || value.find('\\') != string::npos || value.find('\0') != string::npos) return BadApp;
  }
  for (const char* field : {"color", "osMinimalVer"}) {
    JsonArray values = metadata[field].as<JsonArray>();
    if (values.size() != 3) return BadApp;
    for (JsonVariant value : values) if (!value.is<uint8_t>()) return BadApp;
  }
  PythonAppDiscovery::AppInfoJson info;
  if (!PythonAppDiscovery::ParseAppInfo(infoText, info) || !PythonAppDiscovery::ValidateVersionCompatibility(info.osMinimalVer) || !Path(info.appMainFile) ||
      info.appMainFile.size() < 3 || info.appMainFile.substr(info.appMainFile.size() - 3) != ".py") return BadApp;
  bool found = false;
  for (const auto& entry : entries) if (entry.path == info.appMainFile) found = true;
  return found ? OK : BadApp;
}
uint8_t Commit(const string& app) {

  string dest = "/MatrixOS/Applications/" + app;
  bool old = Has(dest);
  if (SYS::activeAppId != DeveloperId) return Running;
  if (old && !replace) return Exists;
  JsonDocument journal; journal["app"] = app; journal["hadOld"] = old;
  string text; serializeJson(journal, text);
  FIL file;
  if (f_open(&file, Journal.c_str(), FA_CREATE_NEW | FA_WRITE) != FR_OK) return IO;
  bool good = WriteExact(file, text.data(), text.size());
  if (f_close(&file) != FR_OK) good = false;
  if (!good) return IO; // Preserve a questionable journal for inspection/recovery.
  if (old && f_rename(dest.c_str(), Backup.c_str()) != FR_OK) return IO;
  if (f_rename(Stage.c_str(), dest.c_str()) != FR_OK) return IO;
  // Recovery sees no stage + existing destination as a completed promotion.
  if (!RemoveTree(Backup) || f_unlink(Journal.c_str()) != FR_OK) return IO;
  return OK;
}
void FinishWork() {
  string app;
  uint8_t error = ValidateAndStage(app);
  if (!error) { SetState(Committing); error = Commit(app); }
  {

    CloseStream();
    if (error && Has(Journal)) Recover();
    else if (!Has(Journal)) { RemoveTree(Stage); RemoveTree(Stream); }
  }
  Release(error ? Failed : Committed, error);
}
void Process(const Request& request) {
  const uint8_t* r = request.bytes;
  uint32_t session = U32(r + 4);
  size_t length = r[8]; uint8_t sub = r[1], error = OK;
  vector<uint8_t> data;
  vector<uint8_t> key(r, r + 9 + length);
  if (key == lastRequest) { Reply(r, U32(lastReply.data()), lastReply[4], vector<uint8_t>(lastReply.begin() + 5, lastReply.end()), request.callback); return; }
  auto s = Get();
  if (sub == Begin) {
    error = length == 25 && session == 0 ? Start(r + 9) : BadPayload;
    session = error ? 0 : Get().session;
  } else if (!session || session != s.session) error = BadSession;
  else if (sub == Data) error = Receive(r + 9, length, data);
  else if (sub == Finish) {
    if (length || s.state != Receiving || s.offset != s.size || contextClosing.load()) error = BadPayload;
    else { SetState(Validating); FinishWork(); error = Get().error; }
  } else if (sub == Abort) {
    if (length) error = BadPayload;
    else if (s.state == Committed || s.state == Committing || s.state == Validating) error = Busy;
    else if (s.state == Aborted) error = OK;
    else if (SYS::activeAppId != DeveloperId || contextClosing.load()) error = Running;
    else { CloseStream(); bool recovered = Recover();
      SetState(recovered ? Aborted : Failed, recovered ? OK : IO); error = recovered ? OK : IO; }
  } else error = Unsupported;
  { Guard g; snapshot.touched = SYS::Millis(); }
  lastRequest = key; lastReply.clear(); Put32(lastReply, session); lastReply.push_back(error); lastReply.insert(lastReply.end(), data.begin(), data.end());
  Reply(r, session, error, data, request.callback);
  if (sub == Data && error == IO) Release(Failed, IO);
}
void AbortSession() {
  auto s = Get();
  if (s.state != Receiving && s.state != Validating && s.state != Committing) return;
  CloseStream();
  bool good = Recover();
  SetState(good ? Aborted : Failed, good ? OK : IO);
  lastRequest.clear();
}
#endif
} // namespace

void Cancel() {
#if DEVICE_STORAGE
  // End() runs before the next app starts. Its task can wait for this command's
  // bounded storage operation; the command never suspends or locks an app task.
  contextClosing.store(true);
  Guard guard;
  AbortSession();
#endif
}
void Poll() {
#if DEVICE_STORAGE
  Guard guard;
  auto s = Get();
  if (s.state == Receiving && uint32_t(SYS::Millis() - s.touched) >= 600000) AbortSession();
#endif
}
bool Handle(const uint8_t* r, size_t size, Command::ReplyCallback cb) {
  if (!r || size < 9 || size > 63 || !cb) return false;
  uint32_t session = U32(r + 4);
  if (r[8] > 54 || size < size_t(9 + r[8])) { Reply(r, session, BadPayload, {}, cb); return true; }
#if DEVICE_STORAGE
  Guard guard;
  if (USB::GetMode() != USB_MODE_NORMAL) { Reply(r, session, Busy, {}, cb); return true; }
  if (r[1] == Caps) {
    vector<uint8_t> data{'M','I','A','P',1,50,uint8_t(MaxFiles >> 8),uint8_t(MaxFiles)};
    Put32(data, MaxPackage); Put32(data, MaxManifest);
    uint8_t error = r[8] || session ? BadPayload : OK;
    Reply(r, session, error, error ? vector<uint8_t>{} : data, cb); return true;
  }
  if (r[1] == Status) {
    auto s = Get(); vector<uint8_t> data{s.state, s.error}; Put32(data, s.offset); Put32(data, s.checksum);
    uint8_t error = r[8] ? BadPayload : session && session == s.session ? OK : BadSession;
    Reply(r, session, error, error ? vector<uint8_t>{} : data, cb); return true;
  }
  Request request{}; memcpy(request.bytes, r, size); request.callback = cb;
  Process(request);
  return true;
#endif
  Reply(r, session, Unsupported, {}, cb); return true;
}
}
