// MystrixSIL Device Implementation
// Software-In-the-Loop emulator for Mystrix, targeting WASM/Emscripten.
// Rebuilt for OS4.0 Input architecture (InputCluster / InputId / InputEvent).

#include "Device.h"
#include "MatrixOS.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static const char* TAG = "MystrixSIL";

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

Direction deviceRotation = TOP;

// LED framebuffer visible to WASM host
Color ledFrameBuffer[64 + 32];
std::mutex ledMutex;

// Rotation-aware LED index mapping (rebuilt on rotation change)
uint16_t ledIndexMap[X_SIZE * Y_SIZE]; // gridIndex -> framebuffer index

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
  if (!ks)
    return false;

  memset(snapshot, 0, sizeof(*snapshot));
  snapshot->id = id;
  snapshot->inputClass = InputClass::Keypad;
  snapshot->keypad = ks->info;
  return true;
}

bool GetKeypadCapabilities(uint8_t clusterId, KeypadCapabilities* caps) {
  if (clusterId > 1)
    return false;
  // SIL emulates binary keypads: no pressure, aftertouch, velocity, or position
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
    ks->info.lastEventTime = Device::Millis();
  }
  else if (!pressed && ks->info.Active())
  {
    ks->info.pressure = 0;
    ks->info.velocity = 0;
    ks->info.state = KeypadState::Idle;
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
    fnState.info.lastEventTime = Device::Millis();
  }
  else if (!pressed && fnState.info.Active())
  {
    fnState.info.pressure = 0;
    fnState.info.velocity = 0;
    fnState.info.state = KeypadState::Idle;
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
  // No-op: SIL keypad is event-driven from JS, not scan-driven.
}

const char* MatrixOS_Wasm_GetVersionString(void) {
  return MATRIXOS_VERSION_STRING.c_str();
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

} // extern "C"
