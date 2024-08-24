#include "MatrixOS.h"
#include <string>

namespace MatrixOS::KEYPAD
{
  QueueHandle_t keyevent_queue;

  void Init() {
    if (keyevent_queue)
    {
      vQueueDelete(keyevent_queue);
    }
    keyevent_queue = xQueueCreate(KEYEVENT_QUEUE_SIZE, sizeof(KeyEvent));
  }

  bool NewEvent(KeyEvent* keyevent) {

    if (uxQueueSpacesAvailable(keyevent_queue) == 0)
    {
      // TODO: Drop first element
    }
    xQueueSend(keyevent_queue, keyevent, 0);
    return uxQueueSpacesAvailable(keyevent_queue) == 0;
  }

  bool Get(KeyEvent* keyevent_dest, uint32_t timeout_ms) {
    return xQueueReceive(keyevent_queue, (void*)keyevent_dest, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
  }

  KeyInfo* GetKey(Point keyXY) {
    return GetKey(XY2ID(keyXY));
  }

  KeyInfo* GetKey(uint16_t keyID) {
    return Device::KeyPad::GetKey(keyID);
  }

  void Clear() {
    Device::KeyPad::Clear();
    ClearList();
  }

  void ClearList() {
    xQueueReset(keyevent_queue);
  }

  uint16_t XY2ID(Point xy)  // Not sure if this is required by Matrix OS, added in for now. return UINT16_MAX if no ID
                            // is assigned to given XY //TODO Compensate for rotation
  {
    if (!xy)
      return UINT16_MAX;
    xy = xy.Rotate(UserVar::rotation, Point(Device::x_size, Device::y_size));
    return Device::KeyPad::XY2ID(xy);
  }

  Point ID2XY(uint16_t keyID)  // Locate XY for given key ID, return Point(INT16_MIN, INT16_MIN) if no XY found for
                               // given ID;
  {
    Point point = Device::KeyPad::ID2XY(keyID);
    if (point)
      return point.Rotate(UserVar::rotation, Point(Device::x_size, Device::y_size), true);
    return point;
  }
}