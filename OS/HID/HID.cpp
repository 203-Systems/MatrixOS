#include "MatrixOS.h"
#include "USB.h"
#include "HID.h"
#include "tusb.h"

#define NKRO_COUNT 6 // Only 6 is supported by the current USB stack

namespace MatrixOS::HID
{
bool Ready() {
  return tud_hid_ready();
}

void Reset() {
  RawHID::Init();

  if (Ready())
  {
    Gamepad::ReleaseAll();
    Keyboard::ReleaseAll();
  }
}

void Init() {
  Reset();
}
} // namespace MatrixOS::HID
