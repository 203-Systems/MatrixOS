#include "MatrixOS.h"
#include "KeyPad.h"
#include <string>

namespace MatrixOS::KeyPad
{
QueueHandle_t keyeventQueue;

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
  xy = xy.Rotate(UserVar::rotation, Point(Device::x_size, Device::y_size));
  return Device::KeyPad::XY2ID(xy);
}

Point ID2XY(uint16_t keyID) // Locate XY for given key ID, return Point(INT16_MIN, INT16_MIN) if no XY found for
                            // given ID;
{
  Point point = Device::KeyPad::ID2XY(keyID);
  if (point)
    return point.Rotate(UserVar::rotation, Point(Device::x_size, Device::y_size), true);
  return point;
}
} // namespace MatrixOS::KeyPad