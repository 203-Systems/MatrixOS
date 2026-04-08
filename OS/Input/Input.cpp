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

// Cluster registry
static vector<InputCluster> clusters;

// Rotation callback: device layer sets this to re-register clusters on rotation change
static void (*rotationCallback)() = nullptr;

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

void RegisterCluster(const InputCluster& cluster) {
  // Replace if a cluster with the same ID already exists
  for (auto& existing : clusters)
  {
    if (existing.clusterId == cluster.clusterId)
    {
      existing = cluster;
      MLOGI(TAG, "Cluster %d (%s) updated", cluster.clusterId, cluster.name.c_str());
      return;
    }
  }
  clusters.push_back(cluster);
  MLOGI(TAG, "Cluster %d (%s) registered: %s %dx%d count=%d",
        cluster.clusterId, cluster.name.c_str(),
        cluster.shape == InputClusterShape::Grid2D ? "Grid2D" :
        cluster.shape == InputClusterShape::Linear1D ? "Linear1D" : "Scalar",
        cluster.dimension.x, cluster.dimension.y, cluster.inputCount);
}

void ClearClusters() {
  clusters.clear();
}

bool NewEvent(const InputEvent& event) {
  // Update state cache
  uint32_t key = PackId(event.id);
  InputSnapshot& snap = stateCache[key];
  snap.id = event.id;
  snap.inputClass = event.inputClass;
  // Copy the union payload by raw copy (all XXXInfo are trivially copyable)
  static_assert(sizeof(snap.keypad) == sizeof(event.keypad));
  memcpy(&snap.keypad, &event.keypad, sizeof(event.keypad));

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
  return clusters;
}

const InputCluster* GetCluster(uint8_t clusterId) {
  for (const auto& cluster : clusters)
  {
    if (cluster.clusterId == clusterId)
    {
      return &cluster;
    }
  }
  return nullptr;
}

const InputCluster* GetPrimaryGridCluster() {
  for (const auto& cluster : clusters)
  {
    if (cluster.inputClass == InputClass::Keypad && cluster.shape == InputClusterShape::Grid2D)
    {
      return &cluster;
    }
  }
  return nullptr;
}

Dimension GetPrimaryGridSize() {
  const InputCluster* grid = GetPrimaryGridCluster();
  if (grid)
  {
    return grid->dimension;
  }
  return Dimension(0, 0);
}

void GetInputsAt(Point xy, vector<InputId>* ids) {
  ids->clear();
  for (const auto& cluster : clusters)
  {
    if (!cluster.HasCoordinates())
    {
      continue;
    }

    // Reverse-rotate from visual space to hardware space
    Point hwPoint = xy;
    if (cluster.rotation != TOP)
    {
      hwPoint = xy.Rotate(cluster.rotation, Point(cluster.rotationDimension.x, cluster.rotationDimension.y), true);
    }

    if (!cluster.Contains(hwPoint))
    {
      continue;
    }

    Point local = hwPoint - cluster.rootPoint;
    uint16_t memberId;
    if (cluster.shape == InputClusterShape::Grid2D)
    {
      memberId = static_cast<uint16_t>(local.y) * cluster.dimension.x + local.x;
    }
    else if (cluster.shape == InputClusterShape::Linear1D)
    {
      memberId = (cluster.dimension.x > 1) ? local.x : local.y;
    }
    else
    {
      continue;
    }

    if (memberId < cluster.inputCount)
    {
      ids->push_back(InputId{cluster.clusterId, memberId});
    }
  }
}

bool GetInputAt(uint8_t clusterId, Point xy, InputId* id) {
  const InputCluster* cluster = GetCluster(clusterId);
  if (!cluster || !cluster->HasCoordinates())
  {
    return false;
  }

  // Reverse-rotate from visual space to hardware space
  Point hwPoint = xy;
  if (cluster->rotation != TOP)
  {
    hwPoint = xy.Rotate(cluster->rotation, Point(cluster->rotationDimension.x, cluster->rotationDimension.y), true);
  }

  if (!cluster->Contains(hwPoint))
  {
    return false;
  }

  Point local = hwPoint - cluster->rootPoint;
  uint16_t memberId;
  if (cluster->shape == InputClusterShape::Grid2D)
  {
    memberId = static_cast<uint16_t>(local.y) * cluster->dimension.x + local.x;
  }
  else if (cluster->shape == InputClusterShape::Linear1D)
  {
    memberId = (cluster->dimension.x > 1) ? local.x : local.y;
  }
  else
  {
    return false;
  }

  if (memberId >= cluster->inputCount)
  {
    return false;
  }

  *id = InputId{clusterId, memberId};
  return true;
}

bool TryGetPoint(InputId id, Point* xy) {
  const InputCluster* cluster = GetCluster(id.clusterId);
  if (!cluster || !cluster->HasCoordinates())
  {
    return false;
  }

  if (id.memberId >= cluster->inputCount)
  {
    return false;
  }

  Point hwPoint;
  if (cluster->shape == InputClusterShape::Grid2D)
  {
    int16_t localX = id.memberId % cluster->dimension.x;
    int16_t localY = id.memberId / cluster->dimension.x;
    hwPoint = Point(cluster->rootPoint.x + localX, cluster->rootPoint.y + localY);
  }
  else if (cluster->shape == InputClusterShape::Linear1D)
  {
    if (cluster->dimension.x > 1)
    {
      hwPoint = Point(cluster->rootPoint.x + id.memberId, cluster->rootPoint.y);
    }
    else
    {
      hwPoint = Point(cluster->rootPoint.x, cluster->rootPoint.y + id.memberId);
    }
  }
  else
  {
    return false;
  }

  // Forward-rotate from hardware space to visual space
  if (cluster->rotation != TOP)
  {
    hwPoint = hwPoint.Rotate(cluster->rotation, Point(cluster->rotationDimension.x, cluster->rotationDimension.y));
  }

  *xy = hwPoint;
  return true;
}

void ClearQueue() {
  if (inputEventQueue)
  {
    xQueueReset(inputEventQueue);
  }
}

void ClearState() {
  stateCache.clear();
  ClearQueue();
}

KeypadInfo GetKeypadState(Point xy) {
  const InputCluster* grid = GetPrimaryGridCluster();
  if (grid) {
    InputId id;
    if (GetInputAt(grid->clusterId, xy, &id)) {
      InputSnapshot snap;
      if (GetState(id, &snap) && snap.inputClass == InputClass::Keypad) {
        return snap.keypad;
      }
    }
  }
  KeypadInfo empty;
  memset(&empty, 0, sizeof(empty));
  return empty;
}

void SetRotationCallback(void (*callback)()) {
  rotationCallback = callback;
}

void NotifyRotationChanged() {
  if (rotationCallback)
  {
    rotationCallback();
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

bool HasVelocitySensitivity() {
  const InputCluster* cluster = GetPrimaryGridCluster();
  if (!cluster) return false;
  KeypadCapabilities caps;
  if (!GetKeypadCapabilities(cluster->clusterId, &caps)) return false;
  return caps.hasVelocity;
}

InputId GetFunctionKeyId() {
  return Device::GetFunctionKeyId();
}

bool IsFunctionKey(InputId id) {
  return Device::IsFunctionKey(id);
}

} // namespace MatrixOS::Input
