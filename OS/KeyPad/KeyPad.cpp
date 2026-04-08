#include "MatrixOS.h"
#include "KeyPad.h"
#include "../Input/Input.h"
#include <string>

namespace MatrixOS::KeyPad
{
QueueHandle_t keyeventQueue;

// Bridge: forward KeyEvent as InputEvent to the new input system
IRAM_ATTR static void BridgeToInput(KeyEvent* keyevent) {
  InputEvent inputEvent;
  inputEvent.id = Device::KeyPad::BridgeKeyId(keyevent->id);
  inputEvent.inputClass = InputClass::Keypad;
  inputEvent.keypad = KeyInfoToKeypadInfo(keyevent->info);
  MatrixOS::Input::NewEvent(inputEvent);
}

void Init() {
  if (!keyeventQueue)
  {
    keyeventQueue = xQueueCreate(KEYEVENT_QUEUE_SIZE, sizeof(KeyEvent));
  }
  else
  {
    xQueueReset(keyeventQueue);
  }
}

IRAM_ATTR bool NewEvent(KeyEvent* keyevent) {
  // Bridge to new input system
  BridgeToInput(keyevent);

  if (uxQueueSpacesAvailable(keyeventQueue) == 0)
  {
    // TODO: Drop first element
  }
  xQueueSend(keyeventQueue, keyevent, 0);
  return uxQueueSpacesAvailable(keyeventQueue) == 0;
}

bool Get(KeyEvent* keyeventDest, uint32_t timeoutMs) {
  return xQueueReceive(keyeventQueue, (void*)keyeventDest, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

KeyInfo* GetKey(Point keyXY) {
  return GetKey(XY2ID(keyXY));
}

KeyInfo* GetKey(uint16_t keyID) {
  return Device::KeyPad::GetKey(keyID);
}

void ClearList() {
  xQueueReset(keyeventQueue);
}

void Clear() {
  // Delegate to the new input system — ClearState clears device KeyInfo,
  // input state cache, input queue, and legacy KeyPad queue.
  MatrixOS::Input::ClearState();
}

uint16_t XY2ID(Point xy) // Delegates to device handlers for coordinate mapping (rotation handled by device layer)
{
  if (!xy)
    return UINT16_MAX;
  // Try all coordinate-capable clusters, preferring cluster function pointers
  for (const auto& cluster : Device::Input::clusters)
  {
    if (!cluster.HasCoordinates())
      continue;
    uint16_t memberId;
    bool found = cluster.tryGetMemberId
      ? cluster.tryGetMemberId(cluster, xy, &memberId)
      : Device::Input::TryGetMemberId(cluster.clusterId, xy, &memberId);
    if (found)
    {
      return Device::KeyPad::InputIdToLegacyKeyId(InputId{cluster.clusterId, memberId});
    }
  }
  return UINT16_MAX;
}

Point ID2XY(uint16_t keyID) // Delegates to device handlers for coordinate mapping (rotation handled by device layer)
{
  InputId id = Device::KeyPad::BridgeKeyId(keyID);
  // Prefer cluster function pointer, fall back to device-level free function
  const auto& clusters = Device::Input::clusters;
  for (const auto& cluster : clusters)
  {
    if (cluster.clusterId == id.clusterId && cluster.tryGetPoint)
    {
      Point point;
      if (cluster.tryGetPoint(cluster, id.memberId, &point))
      {
        return point;
      }
      return Point(INT16_MIN, INT16_MIN);
    }
  }
  Point point;
  if (Device::Input::TryGetPoint(id.clusterId, id.memberId, &point))
  {
    return point;
  }
  return Point(INT16_MIN, INT16_MIN);
}
} // namespace MatrixOS::KeyPad