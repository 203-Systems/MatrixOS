#include "MatrixOS.h"
#include "Input.h"
#include <unordered_map>

static const char* TAG = "Input";

namespace MatrixOS::Input
{

QueueHandle_t inputEventQueue = nullptr;

// State cache: maps packed InputId -> latest snapshot
// Packed key = (clusterId << 16) | localIndex
static std::unordered_map<uint32_t, InputSnapshot> stateCache;

// Cluster registry
static vector<InputCluster> clusters;

static uint32_t PackId(InputId id) {
  return (static_cast<uint32_t>(id.clusterId) << 16) | id.localIndex;
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

} // namespace MatrixOS::Input
