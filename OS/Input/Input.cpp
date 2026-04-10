#include "MatrixOS.h"

static const char* TAG = "Input";

namespace MatrixOS::Input
{

QueueHandle_t inputEventQueue = nullptr;

void Init() {
  if (!inputEventQueue)
  {
    inputEventQueue = xQueueCreate(INPUT_EVENT_QUEUE_SIZE, sizeof(InputEvent));
  }
  else
  {
    xQueueReset(inputEventQueue);
  }
  MLOGI(TAG, "Input system initialized");
}

bool NewEvent(const InputEvent& event) {
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
  return Device::Input::GetState(id, snapshot);
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

bool GetKeypadCapabilities(uint8_t clusterId, KeypadCapabilities* caps) {
  return Device::Input::GetKeypadCapabilities(clusterId, caps);
}

} // namespace MatrixOS::Input
