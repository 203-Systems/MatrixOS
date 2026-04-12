// MystrixSIL Device Implementation
// Software-In-the-Loop emulator for Mystrix, targeting WASM/Emscripten.
// Rebuilt for OS4.0 Input architecture (InputCluster / InputId / InputEvent).

#include "Device.h"
#include "MatrixOS.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static const char* TAG = "MystrixSIL";

#ifndef MATRIXOS_GIT_HASH
#define MATRIXOS_GIT_HASH ""
#endif

#ifndef MATRIXOS_GIT_DIRTY
#define MATRIXOS_GIT_DIRTY 0
#endif

// ---------------------------------------------------------------------------
// Internal keypad state
// ---------------------------------------------------------------------------
namespace
{
struct KeyState {
  KeypadInfo info = {};
  bool suppressed = false;
};

KeyState fnState;
KeyState gridState[X_SIZE * Y_SIZE];
KeyState touchbarLeftState[8];
KeyState touchbarRightState[8];

Direction deviceRotation = TOP;

// USB connection state (controlled by web UI). Must be atomic since it is set
// from the JS main thread and read from the OS pthread.
// Defaults to true so the OS boots with USB available (simulated environment).
static std::atomic<bool> wasmUsbConnected{true};

// Weak hook called by USBStub.cpp to read USB state across translation units.
bool getWasmUsbState() {
  return wasmUsbConnected.load(std::memory_order_acquire);
}

// LED framebuffer visible to WASM host
Color ledFrameBuffer[64 + 32];
std::mutex ledMutex;

// Rotation-aware LED index mapping (rebuilt on rotation change)
uint16_t ledIndexMap[X_SIZE * Y_SIZE]; // gridIndex -> framebuffer index

string BuildVersionLabel() {
  return "Matrix OS " XSTR(MATRIXOS_MAJOR_VER) "." XSTR(MATRIXOS_MINOR_VER) MATRIXOS_MINOR_VERSION_STRING;
}

string BuildIdentityLabel() {
  string version = BuildVersionLabel();
#if defined(MATRIXOS_BUILD_RELEASE)
  string channel = "Release";
#elif defined(MATRIXOS_BUILD_RELEASE_CANDIDATE)
  string channel = "RC " XSTR(MATRIXOS_RELEASE_VER);
#elif defined(MATRIXOS_BUILD_BETA)
  string channel = "Beta " XSTR(MATRIXOS_RELEASE_VER);
#elif defined(MATRIXOS_BUILD_NIGHTY)
  string channel = "Nightly";
#elif defined(MATRIXOS_BUILD_INDEV)
  string channel = "InDev";
#else
  string channel = "";
#endif

  string identity = version;
  if (!channel.empty())
  {
    identity += " • " + channel;
  }

#if defined(MATRIXOS_BUILD_NIGHTY) || defined(MATRIXOS_BUILD_INDEV)
  if (strlen(MATRIXOS_GIT_HASH) > 0)
  {
    identity += " ";
    identity += MATRIXOS_GIT_HASH;
  }
#endif

#if MATRIXOS_GIT_DIRTY
  identity += " • Dirty";
#endif

  return identity;
}

string wasmVersionLabel = BuildVersionLabel();
string wasmBuildIdentityLabel = BuildIdentityLabel();

void BuildLEDIndexMap(Direction rotation) {
  for (uint16_t y = 0; y < Y_SIZE; y++)
  {
    for (uint16_t x = 0; x < X_SIZE; x++)
    {
      Point physical(x, y);
      Point logical = physical.Rotate(rotation, Point(X_SIZE, Y_SIZE), true);
      uint16_t gridIndex = logical.y * X_SIZE + logical.x;
      uint16_t fbIndex = y * X_SIZE + x;
      if (gridIndex < X_SIZE * Y_SIZE)
      {
        ledIndexMap[gridIndex] = fbIndex;
      }
    }
  }
}

// NVS in-memory store
std::map<uint32_t, vector<char>> nvsStore;
std::mutex nvsMutex;

// Emit input event to OS, respecting suppression
bool EmitKeyEvent(InputId id, KeyState* ks) {
  if (ks->suppressed)
  {
    if (!ks->info.Active())
    {
      ks->suppressed = false;
    }
    return false;
  }

  InputEvent event;
  event.id = id;
  event.inputClass = InputClass::Keypad;
  event.keypad = ks->info;
  return MatrixOS::Input::NewEvent(event);
}

bool TickHoldEvent(InputId id, KeyState* ks, uint32_t now) {
  if (!ks->info.Active() || ks->info.hold)
  {
    return false;
  }

  if ((now - ks->info.lastEventTime) <= KeypadInfo::kHoldThreshold)
  {
    return false;
  }

  ks->info.state = KeypadState::Hold;
  ks->info.hold = true;
  return EmitKeyEvent(id, ks);
}
} // anonymous namespace

// ---------------------------------------------------------------------------
// Device contract implementation
// ---------------------------------------------------------------------------
namespace Device
{

// --- Input cluster infrastructure (mirrors Mystrix1/Family.cpp) ---

void RegisterInputClusters();

namespace Input
{
vector<InputCluster> clusters;

static Point RotatePhysicalPoint(Point physicalPoint) {
  return physicalPoint.Rotate(deviceRotation, Point(X_SIZE, Y_SIZE), true);
}

static Point UnrotateLogicalPoint(Point logicalPoint) {
  return logicalPoint.Rotate(deviceRotation, Point(X_SIZE, Y_SIZE));
}

static bool GridGetPosition(const InputCluster& cluster, uint16_t memberId, Point* point) {
  if (memberId >= cluster.inputCount)
    return false;
  int16_t localX = memberId % cluster.dimension.x;
  int16_t localY = memberId / cluster.dimension.x;
  *point = RotatePhysicalPoint(Point(localX, localY));
  return true;
}

static bool GridTryGetMemberId(const InputCluster& cluster, Point point, uint16_t* memberId) {
  Point local = UnrotateLogicalPoint(point);
  if (local.x < 0 || local.y < 0 || local.x >= cluster.dimension.x || local.y >= cluster.dimension.y)
    return false;
  uint16_t id = static_cast<uint16_t>(local.y) * cluster.dimension.x + local.x;
  if (id >= cluster.inputCount)
    return false;
  *memberId = id;
  return true;
}

static Point GetTouchBarPhysicalPoint(uint8_t clusterId, uint16_t memberId) {
  return (clusterId == 2) ? Point(-1, (int16_t)memberId) : Point(X_SIZE, (int16_t)memberId);
}

static void GetTouchBarBounds(uint8_t clusterId, Point* rootPoint, Dimension* dimension) {
  const Point physicalStart = (clusterId == 2) ? Point(-1, 0) : Point(X_SIZE, 0);
  const Point physicalEnd = (clusterId == 2) ? Point(-1, 7) : Point(X_SIZE, 7);
  const Point rotatedStart = RotatePhysicalPoint(physicalStart);
  const Point rotatedEnd = RotatePhysicalPoint(physicalEnd);
  rootPoint->x = std::min(rotatedStart.x, rotatedEnd.x);
  rootPoint->y = std::min(rotatedStart.y, rotatedEnd.y);
  dimension->x = std::abs(rotatedStart.x - rotatedEnd.x) + 1;
  dimension->y = std::abs(rotatedStart.y - rotatedEnd.y) + 1;
}

static bool TouchBarGetPosition(const InputCluster& cluster, uint16_t memberId, Point* point) {
  if (memberId >= cluster.inputCount)
    return false;
  *point = RotatePhysicalPoint(GetTouchBarPhysicalPoint(cluster.clusterId, memberId));
  return true;
}

static bool TouchBarTryGetMemberId(const InputCluster& cluster, Point point, uint16_t* memberId) {
  Point physicalPoint = UnrotateLogicalPoint(point);
  const int16_t expectedX = (cluster.clusterId == 2) ? -1 : X_SIZE;
  if (physicalPoint.x != expectedX || physicalPoint.y < 0 || physicalPoint.y >= (int16_t)cluster.inputCount)
    return false;
  *memberId = (uint16_t)physicalPoint.y;
  return true;
}

bool GetPosition(uint8_t clusterId, uint16_t memberId, Point* point) {
  for (const auto& c : clusters)
  {
    if (c.clusterId == clusterId)
    {
      if (!c.HasCoordinates() || !c.getPosition)
        return false;
      return c.getPosition(c, memberId, point);
    }
  }
  return false;
}

bool TryGetMemberId(uint8_t clusterId, Point point, uint16_t* memberId) {
  for (const auto& c : clusters)
  {
    if (c.clusterId == clusterId)
    {
      if (!c.HasCoordinates() || !c.tryGetMemberId)
        return false;
      return c.tryGetMemberId(c, point, memberId);
    }
  }
  return false;
}

void SuppressActiveInputs() {
  if (fnState.info.Active())
    fnState.suppressed = true;
  for (auto& ks : gridState)
  {
    if (ks.info.Active())
      ks.suppressed = true;
  }
  for (auto& ks : touchbarLeftState)
  {
    if (ks.info.Active())
      ks.suppressed = true;
  }
  for (auto& ks : touchbarRightState)
  {
    if (ks.info.Active())
      ks.suppressed = true;
  }
}

bool GetState(InputId id, InputSnapshot* snapshot) {
  KeyState* ks = nullptr;
  if (id.clusterId == 0 && id.memberId == 0)
  {
    ks = &fnState;
  }
  else if (id.clusterId == 1 && id.memberId < X_SIZE * Y_SIZE)
  {
    ks = &gridState[id.memberId];
  }
  else if (id.clusterId == 2 && id.memberId < 8)
  {
    ks = &touchbarLeftState[id.memberId];
  }
  else if (id.clusterId == 3 && id.memberId < 8)
  {
    ks = &touchbarRightState[id.memberId];
  }
  if (!ks)
    return false;

  memset(snapshot, 0, sizeof(*snapshot));
  snapshot->id = id;
  snapshot->inputClass = InputClass::Keypad;
  snapshot->keypad = ks->info;
  return true;
}

bool GetKeypadCapabilities(uint8_t clusterId, KeypadCapabilities* caps) {
  if (clusterId > 3)
    return false;
  // All SIL clusters are binary keypads: no pressure, aftertouch, velocity, or position
  *caps = {false, false, false, false};
  return true;
}
} // namespace Input

void RegisterInputClusters() {
  Input::clusters.clear();

  // Cluster 0: System / FN — no coordinates
  InputCluster fnCluster;
  fnCluster.clusterId = 0;
  fnCluster.name = "System";
  fnCluster.inputClass = InputClass::Keypad;
  fnCluster.shape = InputClusterShape::Scalar;
  fnCluster.rootPoint = Point::Invalid();
  fnCluster.dimension = Dimension(1, 1);
  fnCluster.inputCount = 1;
  Input::clusters.push_back(fnCluster);

  // Cluster 1: 8x8 Grid
  InputCluster gridCluster;
  gridCluster.clusterId = 1;
  gridCluster.name = "Grid";
  gridCluster.inputClass = InputClass::Keypad;
  gridCluster.shape = InputClusterShape::Grid2D;
  gridCluster.rootPoint = Point(0, 0);
  gridCluster.dimension = Dimension(X_SIZE, Y_SIZE);
  gridCluster.inputCount = X_SIZE * Y_SIZE;
  gridCluster.getPosition = Input::GridGetPosition;
  gridCluster.tryGetMemberId = Input::GridTryGetMemberId;
  Input::clusters.push_back(gridCluster);

  // Cluster 2: TouchBar Left (physical x = -1, y = 0..7)
  InputCluster touchbarLeftCluster;
  touchbarLeftCluster.clusterId = 2;
  touchbarLeftCluster.name = "TouchBarLeft";
  touchbarLeftCluster.inputClass = InputClass::Keypad;
  touchbarLeftCluster.shape = InputClusterShape::Linear1D;
  touchbarLeftCluster.inputCount = 8;
  Input::GetTouchBarBounds(2, &touchbarLeftCluster.rootPoint, &touchbarLeftCluster.dimension);
  touchbarLeftCluster.getPosition = Input::TouchBarGetPosition;
  touchbarLeftCluster.tryGetMemberId = Input::TouchBarTryGetMemberId;
  Input::clusters.push_back(touchbarLeftCluster);

  // Cluster 3: TouchBar Right (physical x = X_SIZE, y = 0..7)
  InputCluster touchbarRightCluster;
  touchbarRightCluster.clusterId = 3;
  touchbarRightCluster.name = "TouchBarRight";
  touchbarRightCluster.inputClass = InputClass::Keypad;
  touchbarRightCluster.shape = InputClusterShape::Linear1D;
  touchbarRightCluster.inputCount = 8;
  Input::GetTouchBarBounds(3, &touchbarRightCluster.rootPoint, &touchbarRightCluster.dimension);
  touchbarRightCluster.getPosition = Input::TouchBarGetPosition;
  touchbarRightCluster.tryGetMemberId = Input::TouchBarTryGetMemberId;
  Input::clusters.push_back(touchbarRightCluster);
}

// --- LED ---

namespace LED
{
void Update(Color* frameBuffer, vector<uint8_t>& brightness) {
  std::lock_guard<std::mutex> lock(ledMutex);
  uint16_t totalLEDs = Device::LED::count;
  for (uint16_t i = 0; i < totalLEDs && i < (64 + 32); i++)
  {
    ledFrameBuffer[i] = frameBuffer[i];
  }
}

uint16_t XY2Index(Point xy) {
  if (xy.x < 0 || xy.x >= X_SIZE || xy.y < 0 || xy.y >= Y_SIZE)
    return UINT16_MAX;
  uint16_t gridIndex = xy.y * X_SIZE + xy.x;
  return ledIndexMap[gridIndex];
}

uint16_t ID2Index(uint16_t ledID) {
  if (ledID >= Device::LED::count)
    return UINT16_MAX;
  return ledID;
}

Point Index2XY(uint16_t index) {
  if (index >= (uint16_t)(X_SIZE * Y_SIZE))
    return Point(INT16_MIN, INT16_MIN);
  // Reverse lookup: find which logical XY maps to this framebuffer index
  for (int16_t y = 0; y < Y_SIZE; y++)
  {
    for (int16_t x = 0; x < X_SIZE; x++)
    {
      uint16_t gridIndex = y * X_SIZE + x;
      if (ledIndexMap[gridIndex] == index)
        return Point(x, y);
    }
  }
  return Point(INT16_MIN, INT16_MIN);
}
} // namespace LED

// --- Rotation ---

void Rotate(Direction newRotation, bool absolute) {
  if (newRotation != 0 && newRotation != 90 && newRotation != 180 && newRotation != 270)
    return;
  if (newRotation == 0 && !absolute)
    return;

  Direction target = (Direction)((deviceRotation * !absolute + newRotation) % 360);
  deviceRotation = target;
  BuildLEDIndexMap(deviceRotation);
  RegisterInputClusters();
  MatrixOS::LED::Update(0);
  MatrixOS::Input::ClearInputBuffer();
  Device::Input::SuppressActiveInputs();
}

Direction GetRotation() {
  return deviceRotation;
}

// --- NVS ---

namespace NVS
{
size_t Size(uint32_t hash) {
  std::lock_guard<std::mutex> lock(nvsMutex);
  auto it = nvsStore.find(hash);
  return (it != nvsStore.end()) ? it->second.size() : 0;
}

vector<char> Read(uint32_t hash) {
  std::lock_guard<std::mutex> lock(nvsMutex);
  auto it = nvsStore.find(hash);
  return (it != nvsStore.end()) ? it->second : vector<char>();
}

bool Write(uint32_t hash, void* pointer, uint16_t length) {
  std::lock_guard<std::mutex> lock(nvsMutex);
  nvsStore[hash] = vector<char>((char*)pointer, (char*)pointer + length);
  return true;
}

bool Delete(uint32_t hash) {
  std::lock_guard<std::mutex> lock(nvsMutex);
  return nvsStore.erase(hash) > 0;
}

void Clear() {
  std::lock_guard<std::mutex> lock(nvsMutex);
  nvsStore.clear();
}
} // namespace NVS

// --- Lifecycle ---

void DeviceInit() {
  BuildLEDIndexMap(deviceRotation);
  RegisterInputClusters();
}

void DeviceStart() {
  BuildLEDIndexMap(deviceRotation);
  RegisterInputClusters();
}

void DeviceSettings() {}

void Bootloader() {
#ifdef __EMSCRIPTEN__
  EM_ASM({ if (typeof MatrixOS_Bootloader === "function") MatrixOS_Bootloader(); });
#endif
}

void Reboot() {
#ifdef __EMSCRIPTEN__
  EM_ASM({ if (typeof MatrixOS_Reboot === "function") MatrixOS_Reboot(); });
#endif
}

void Delay(uint32_t interval) {
  vTaskDelay(pdMS_TO_TICKS(interval));
}

uint32_t Millis() {
  return ((((uint64_t)xTaskGetTickCount()) * 1000) / configTICK_RATE_HZ);
}

void Log(string& format, va_list& valst) {
  vprintf(format.c_str(), valst);
}

string GetSerial() {
  return "MYSTRIXSIL000000";
}

void ErrorHandler() {}

uint64_t Micros() {
  return Millis() * 1000ULL;
}

} // namespace Device

// ---------------------------------------------------------------------------
// WASM-exported API (called from JavaScript)
// ---------------------------------------------------------------------------
extern "C"
{

uint8_t* MatrixOS_Wasm_GetFrameBuffer(void) {
  return reinterpret_cast<uint8_t*>(ledFrameBuffer);
}

uint32_t MatrixOS_Wasm_GetFrameBufferByteLength(void) {
  return sizeof(ledFrameBuffer);
}

uint32_t MatrixOS_Wasm_GetWidth(void) {
  return X_SIZE;
}

uint32_t MatrixOS_Wasm_GetHeight(void) {
  return Y_SIZE;
}

void MatrixOS_Wasm_KeyEvent(uint16_t x, uint16_t y, bool pressed) {
  if (x >= X_SIZE || y >= Y_SIZE)
    return;
  uint16_t keyIndex = y * X_SIZE + x;

  KeyState* ks = &gridState[keyIndex];
  if (pressed && !ks->info.Active())
  {
    ks->info.pressure = 65535;
    ks->info.velocity = 65535;
    ks->info.state = KeypadState::Pressed;
    ks->info.hold = false;
    ks->info.lastEventTime = Device::Millis();
  }
  else if (!pressed && ks->info.Active())
  {
    ks->info.pressure = 0;
    ks->info.velocity = 0;
    ks->info.state = KeypadState::Released;
    ks->info.lastEventTime = Device::Millis();
  }
  else
  {
    return;
  }

  InputId id = {1, keyIndex};
  EmitKeyEvent(id, ks);
}

void MatrixOS_Wasm_FnEvent(bool pressed) {
  if (pressed && !fnState.info.Active())
  {
    fnState.info.pressure = 65535;
    fnState.info.velocity = 65535;
    fnState.info.state = KeypadState::Pressed;
    fnState.info.hold = false;
    fnState.info.lastEventTime = Device::Millis();
  }
  else if (!pressed && fnState.info.Active())
  {
    fnState.info.pressure = 0;
    fnState.info.velocity = 0;
    fnState.info.state = KeypadState::Released;
    fnState.info.lastEventTime = Device::Millis();
  }
  else
  {
    return;
  }

  InputId id = InputId::FunctionKey();
  EmitKeyEvent(id, &fnState);
}

void MatrixOS_Wasm_KeypadTick(void) {
  uint32_t now = Device::Millis();
  TickHoldEvent(InputId::FunctionKey(), &fnState, now);

  for (uint16_t keyIndex = 0; keyIndex < X_SIZE * Y_SIZE; keyIndex++)
  {
    InputId id = {1, keyIndex};
    TickHoldEvent(id, &gridState[keyIndex], now);
  }

  for (uint16_t i = 0; i < 8; i++)
  {
    TickHoldEvent({2, i}, &touchbarLeftState[i], now);
    TickHoldEvent({3, i}, &touchbarRightState[i], now);
  }
}

// side: 0 = left (cluster 2), 1 = right (cluster 3); index: 0..7
void MatrixOS_Wasm_TouchBarEvent(uint8_t side, uint16_t index, bool pressed) {
  if (index >= 8)
    return;
  uint8_t clusterId = (side == 0) ? 2 : 3;
  KeyState* ks = (side == 0) ? &touchbarLeftState[index] : &touchbarRightState[index];

  if (pressed && !ks->info.Active())
  {
    ks->info.pressure = 65535;
    ks->info.velocity = 65535;
    ks->info.state = KeypadState::Pressed;
    ks->info.hold = false;
    ks->info.lastEventTime = Device::Millis();
  }
  else if (!pressed && ks->info.Active())
  {
    ks->info.pressure = 0;
    ks->info.velocity = 0;
    ks->info.state = KeypadState::Released;
    ks->info.lastEventTime = Device::Millis();
  }
  else
  {
    return;
  }

  InputId id = {clusterId, index};
  EmitKeyEvent(id, ks);
}

const char* MatrixOS_Wasm_GetVersionString(void) {
  return wasmVersionLabel.c_str();
}

const char* MatrixOS_Wasm_GetBuildIdentityString(void) {
  return wasmBuildIdentityLabel.c_str();
}

void MatrixOS_Wasm_SetUsbAvailable(int available) {
  wasmUsbConnected.store(available != 0, std::memory_order_release);
}

void MatrixOS_Wasm_Reboot(void) {
  Device::Reboot();
}

void MatrixOS_Wasm_Bootloader(void) {
  Device::Bootloader();
}

int MatrixOS_Wasm_GetRotation(void) {
  return (int)deviceRotation;
}

uint32_t MatrixOS_Wasm_GetUptimeMs(void) {
  return Device::Millis();
}

// Returns a pointer to an array of uint8_t[X_SIZE * Y_SIZE] where each byte
// is 1 if the key is currently active, 0 otherwise.  Polled by the dashboard
// Input panel so it reflects real runtime state, not just injection-side events.
static uint8_t wasmKeypadSnapshot[X_SIZE * Y_SIZE];

uint8_t* MatrixOS_Wasm_GetKeypadState(void) {
  for (uint16_t i = 0; i < X_SIZE * Y_SIZE; i++)
  {
    wasmKeypadSnapshot[i] = gridState[i].info.Active() ? 1 : 0;
  }
  return wasmKeypadSnapshot;
}

uint32_t MatrixOS_Wasm_GetKeypadStateLength(void) {
  return X_SIZE * Y_SIZE;
}

uint8_t MatrixOS_Wasm_GetFnState(void) {
  return fnState.info.Active() ? 1 : 0;
}

// ---- MIDI injection ----

void MatrixOS_Wasm_MidiInject(uint8_t status, uint8_t d0, uint8_t d1, uint8_t d2, uint16_t targetPort) {
  MidiPacket pkt;
  pkt.status = (EMidiStatus)status;
  pkt.data[0] = d0;
  pkt.data[1] = d1;
  pkt.data[2] = d2;
  pkt.port = MIDI_PORT_OS;
  MatrixOS::MIDI::Send(pkt, targetPort, 10);
}

// ---- RawHID injection ----

void MatrixOS_Wasm_RawHidInject(uint8_t* data, uint32_t length) {
  if (length > 32) length = 32;
  MatrixOS::HID::RawHID::NewReport(data, length);
}

// ---- Serial TX (device output) ----

void MatrixOS_Wasm_SerialWrite(const char* data, uint32_t length) {
  string str(data, length);
  MatrixOS::USB::CDC::Print(str);
}

// ---- NVS exports ----

uint32_t MatrixOS_Wasm_NvsGetCount(void) {
  std::lock_guard<std::mutex> lock(nvsMutex);
  return nvsStore.size();
}

static uint32_t nvsHashBuffer[256];

uint32_t* MatrixOS_Wasm_NvsGetHashes(void) {
  std::lock_guard<std::mutex> lock(nvsMutex);
  uint32_t i = 0;
  for (const auto& [hash, data] : nvsStore) {
    if (i >= 256) break;
    nvsHashBuffer[i++] = hash;
  }
  return nvsHashBuffer;
}

uint32_t MatrixOS_Wasm_NvsGetSize(uint32_t hash) {
  return Device::NVS::Size(hash);
}

static char nvsReadBuffer[4096];

uint8_t* MatrixOS_Wasm_NvsGetData(uint32_t hash) {
  auto data = Device::NVS::Read(hash);
  if (data.empty()) return nullptr;
  uint32_t len = data.size();
  if (len > sizeof(nvsReadBuffer)) len = sizeof(nvsReadBuffer);
  memcpy(nvsReadBuffer, data.data(), len);
  return reinterpret_cast<uint8_t*>(nvsReadBuffer);
}

bool MatrixOS_Wasm_NvsWrite(uint32_t hash, uint8_t* data, uint16_t length) {
  return Device::NVS::Write(hash, data, length);
}

bool MatrixOS_Wasm_NvsDelete(uint32_t hash) {
  return Device::NVS::Delete(hash);
}

void MatrixOS_Wasm_NvsClear(void) {
  Device::NVS::Clear();
}

static vector<uint8_t> nvsExportBuffer;

uint8_t* MatrixOS_Wasm_NvsExport(void) {
  std::lock_guard<std::mutex> lock(nvsMutex);
  nvsExportBuffer.clear();
  uint32_t count = nvsStore.size();
  nvsExportBuffer.insert(nvsExportBuffer.end(), (uint8_t*)&count, (uint8_t*)&count + 4);
  for (const auto& [hash, data] : nvsStore) {
    uint32_t h = hash;
    uint32_t len = data.size();
    nvsExportBuffer.insert(nvsExportBuffer.end(), (uint8_t*)&h, (uint8_t*)&h + 4);
    nvsExportBuffer.insert(nvsExportBuffer.end(), (uint8_t*)&len, (uint8_t*)&len + 4);
    nvsExportBuffer.insert(nvsExportBuffer.end(), data.begin(), data.end());
  }
  return nvsExportBuffer.data();
}

uint32_t MatrixOS_Wasm_NvsExportSize(void) {
  return nvsExportBuffer.size();
}

void MatrixOS_Wasm_NvsImport(uint8_t* data, uint32_t totalLength) {
  std::lock_guard<std::mutex> lock(nvsMutex);
  nvsStore.clear();
  if (totalLength < 4) return;
  uint32_t count;
  memcpy(&count, data, 4);
  uint32_t offset = 4;
  for (uint32_t i = 0; i < count && offset + 8 <= totalLength; i++) {
    uint32_t hash, len;
    memcpy(&hash, data + offset, 4); offset += 4;
    memcpy(&len, data + offset, 4); offset += 4;
    if (offset + len > totalLength) break;
    nvsStore[hash] = vector<char>((char*)(data + offset), (char*)(data + offset + len));
    offset += len;
  }
}

} // extern "C"
