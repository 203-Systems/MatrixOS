#include "MatrixOS.h"
#include "KeyPad.h"
#include "../Input/Input.h"
#include <string>

namespace MatrixOS::KeyPad
{
QueueHandle_t keyeventQueue;

// Bridge: decode old keyID into InputId
// Old encoding: CCCC IIIIIIIIIIII (C=class 4 bits, I=index 12 bits)
// Class 0 = System → clusterId 0
// Class 1 = Grid (XXXXXX YYYYYY) → clusterId 1, memberId = y * xSize + x
// Class 2 = TouchBar → cluster 2 (left 0-7) or cluster 3 (right 8-15)
static InputId BridgeKeyId(uint16_t keyID) {
  uint8_t keyClass = keyID >> 12;
  switch (keyClass)
  {
  case 0: // System
    return InputId{0, static_cast<uint16_t>(keyID & 0x0FFF)};
  case 1: // Grid
  {
    uint16_t x = (keyID >> 6) & 0x3F;
    uint16_t y = keyID & 0x3F;
    return InputId{1, static_cast<uint16_t>(y * Device::xSize + x)};
  }
  case 2: // TouchBar: left side (0-7) → cluster 2, right side (8-15) → cluster 3
  {
    uint16_t index = keyID & 0x0FFF;
    if (index >= 8)
    {
      return InputId{3, static_cast<uint16_t>(index - 8)};
    }
    return InputId{2, static_cast<uint16_t>(index)};
  }
  default:
    return InputId{static_cast<uint8_t>(keyClass), static_cast<uint16_t>(keyID & 0x0FFF)};
  }
}

// Reverse bridge: convert InputId back to legacy keyID encoding
// This is the inverse of BridgeKeyId.
static uint16_t InputIdToLegacyKeyId(InputId id) {
  switch (id.clusterId)
  {
  case 0: // System
    return id.memberId;
  case 1: // Grid: memberId = y * xSize + x
  {
    uint16_t x = id.memberId % Device::xSize;
    uint16_t y = id.memberId / Device::xSize;
    return (1 << 12) | (x << 6) | y;
  }
  case 2: // TouchBar Left
    return (2 << 12) | id.memberId;
  case 3: // TouchBar Right
    return (2 << 12) | (id.memberId + 8);
  default:
    return UINT16_MAX;
  }
}

// Bridge: forward KeyEvent as InputEvent to the new input system
IRAM_ATTR static void BridgeToInput(KeyEvent* keyevent) {
  InputEvent inputEvent;
  inputEvent.id = BridgeKeyId(keyevent->id);
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
  Device::KeyPad::Clear();
  ClearList();
}

uint16_t XY2ID(Point xy) // Delegates to device handlers for coordinate mapping (rotation handled by device layer)
{
  if (!xy)
    return UINT16_MAX;
  // Try all coordinate-capable clusters via device handlers
  for (const auto& cluster : Device::Input::clusters)
  {
    if (!cluster.HasCoordinates())
      continue;
    uint16_t memberId;
    if (Device::Input::TryGetMemberId(cluster.clusterId, xy, &memberId))
    {
      return InputIdToLegacyKeyId(InputId{cluster.clusterId, memberId});
    }
  }
  return UINT16_MAX;
}

Point ID2XY(uint16_t keyID) // Delegates to device handlers for coordinate mapping (rotation handled by device layer)
{
  InputId id = BridgeKeyId(keyID);
  Point point;
  if (Device::Input::TryGetPoint(id.clusterId, id.memberId, &point))
  {
    return point;
  }
  return Point(INT16_MIN, INT16_MIN);
}
} // namespace MatrixOS::KeyPad