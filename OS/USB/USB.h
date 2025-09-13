#pragma once

#include <stdint.h>

#define USB_MIDI_COUNT 2

// USB mode definitions
enum USB_MODE {
  USB_MODE_DEFAULT = 0,  // MIDI + CDC + HID
  USB_MODE_MSC = 1       // MSC only
};

namespace MatrixOS::USB
{
  void Init(USB_MODE initial_mode = USB_MODE_DEFAULT);
  void NormalMode();
  uint8_t GetMode();

  namespace MIDI
  {
    void Init();
  }

  namespace MSC
  {
    void Enable();
  }
}