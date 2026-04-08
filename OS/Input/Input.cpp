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
