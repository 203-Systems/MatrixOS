#pragma once

#include <stdint.h>

#define USB_MIDI_COUNT 2

// USB mode definitions
enum USB_MODE {
  USB_MODE_NORMAL = 0,  // MIDI + CDC + HID
  USB_MODE_MSC = 1      // MSC only
};

namespace MatrixOS::USB
{
  void Init(USB_MODE mode = USB_MODE_NORMAL);
  void SetMode(USB_MODE mode);
  uint8_t GetMode();

  namespace MIDI
  {
    void Init();
  }
}