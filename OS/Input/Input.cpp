#include "MatrixOS.h"
#include "Input.h"
#include <unordered_map>

static const char* TAG = "Input";

namespace MatrixOS::Input
{

QueueHandle_t inputEventQueue = nullptr;

// State cache: maps packed InputId -> latest snapshot
// Packed key = (clusterId << 16) | memberId
static std::unordered_map<uint32_t, InputSnapshot> stateCache;

// Capabilities storage: maps clusterId -> capabilities struct
static std::unordered_map<uint8_t, KeypadCapabilities> keypadCapsMap;

static uint32_t PackId(InputId id) {
  return (static_cast<uint32_t>(id.clusterId) << 16) | id.memberId;
}

void Init() {
  if (!inputEventQueue)
  {
    inputEventQueue = xQueueCreate(INPUT_EVENT_QUEUE_SIZE, sizeof(InputEvent));
  }
  else
  {
    xQueueReset(inputEventQueue);
  }
  stateCache.clear();
  MLOGI(TAG, "Input system initialized");
}

bool NewEvent(const InputEvent& event) {
  // Update state cache
  uint32_t key = PackId(event.id);
  InputSnapshot& snap = stateCache[key];
  snap.id = event.id;
  snap.inputClass = event.inputClass;
  // Copy the full union payload by raw copy (all XXXInfo are trivially copyable).
  // Use the actual union size, not just one member's size, since members may differ in size.
  constexpr size_t payloadSize = sizeof(InputEvent) - offsetof(InputEvent, keypad);
  static_assert(payloadSize == sizeof(InputSnapshot) - offsetof(InputSnapshot, keypad));
  memcpy(&snap.keypad, &event.keypad, payloadSize);

  // Enqueue event
  if (uxQueueSpacesAvailable(inputEventQueue) == 0)
  {
    // Drop oldest event when queue is full
    InputEvent dropped;
    xQueueReceive(inputEventQueue, &dropped, 0);
  }
  xQueueSend(inputEventQueue, &event, 0);
  return uxQueueSpacesAvailable(inputEventQueue) == 0;
}

bool Get(InputEvent* event, uint32_t timeoutMs) {
  if (!inputEventQueue)
  {
    return false;
  }
  return xQueueReceive(inputEventQueue, event, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

bool GetState(InputId id, InputSnapshot* snapshot) {
  uint32_t key = PackId(id);
  auto it = stateCache.find(key);
  if (it == stateCache.end())
  {
    return false;
  }
  *snapshot = it->second;
  return true;
}

const vector<InputCluster>& GetClusters() {
  return Device::Input::clusters;
}

const InputCluster* GetCluster(uint8_t clusterId) {
  for (const auto& cluster : Device::Input::clusters)
  {
    if (cluster.clusterId == clusterId)
    {
      return &cluster;
    }
  }
  return nullptr;
}

const InputCluster* GetPrimaryGridCluster() {
  for (const auto& cluster : Device::Input::clusters)
  {
    if (cluster.inputClass == InputClass::Keypad && cluster.shape == InputClusterShape::Grid2D)
    {
      return &cluster;
    }
  }
  return nullptr;
}



void GetInputsAt(Point xy, vector<InputId>* ids) {
  ids->clear();
  for (const auto& cluster : Device::Input::clusters)
  {
    if (!cluster.HasCoordinates())
    {
      continue;
    }

    uint16_t memberId;
    bool found = cluster.tryGetMemberId
      ? cluster.tryGetMemberId(cluster, xy, &memberId)
      : Device::Input::TryGetMemberId(cluster.clusterId, xy, &memberId);
    if (found)
    {
      ids->push_back(InputId{cluster.clusterId, memberId});
    }
  }
}

bool GetInputAt(uint8_t clusterId, Point xy, InputId* id) {
  const InputCluster* cluster = GetCluster(clusterId);
  if (!cluster || !cluster->HasCoordinates()) return false;

  uint16_t memberId;
  bool found = cluster->tryGetMemberId
    ? cluster->tryGetMemberId(*cluster, xy, &memberId)
    : Device::Input::TryGetMemberId(clusterId, xy, &memberId);
  if (!found) return false;

  *id = InputId{clusterId, memberId};
  return true;
}

bool GetPosition(InputId id, Point* xy) {
  const InputCluster* cluster = GetCluster(id.clusterId);
  if (cluster && cluster->getPosition)
  {
    return cluster->getPosition(*cluster, id.memberId, xy);
  }
  return Device::Input::GetPosition(id.clusterId, id.memberId, xy);
}

void ClearInputBuffer() {
  if (inputEventQueue)
  {
    xQueueReset(inputEventQueue);
  }
}

void RegisterKeypadCapabilities(uint8_t clusterId, const KeypadCapabilities& caps) {
  keypadCapsMap[clusterId] = caps;
}

bool GetKeypadCapabilities(uint8_t clusterId, KeypadCapabilities* caps) {
  auto it = keypadCapsMap.find(clusterId);
  if (it == keypadCapsMap.end())
  {
    return false;
  }
  *caps = it->second;
  return true;
}

} // namespace MatrixOS::Input
