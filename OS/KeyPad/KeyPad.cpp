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
// Class 1 = Grid (XXXXXX YYYYYY) → clusterId 1, localIndex = y * xSize + x
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

uint16_t XY2ID(Point xy) // Not sure if this is required by Matrix OS, added in for now. return UINT16_MAX if no ID
                         // is assigned to given XY //TODO Compensate for rotation
{
  if (!xy)
    return UINT16_MAX;
  xy = xy.Rotate(UserVar::rotation, Point(Device::xSize, Device::ySize));
  return Device::KeyPad::XY2ID(xy);
}

Point ID2XY(uint16_t keyID) // Locate XY for given key ID, return Point(INT16_MIN, INT16_MIN) if no XY found for
                            // given ID;
{
  Point point = Device::KeyPad::ID2XY(keyID);
  if (point)
    return point.Rotate(UserVar::rotation, Point(Device::xSize, Device::ySize), true);
  return point;
}
} // namespace MatrixOS::KeyPad