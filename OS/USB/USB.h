#pragma once

#include "tusb.h"

#define USB_MIDI_COUNT 2

namespace MatrixOS::USB::MIDI
{
  void Init();
}

void tud_midi_rx_cb(uint8_t itf);