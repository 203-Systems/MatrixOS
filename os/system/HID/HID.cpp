#include "MatrixOS.h"

#define NKRO_COUNT 6 // Only 6 is supported by the current USB stack

namespace MatrixOS::HID
{
  bool Ready()
  {
    return tud_hid_ready();
  }
}