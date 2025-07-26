#pragma once

// Define missing FreeRTOS trace macros to avoid compilation errors
#ifndef traceISR_EXIT_TO_SCHEDULER
#define traceISR_EXIT_TO_SCHEDULER()
#endif

#include "tusb.h"

#define USB_MIDI_COUNT 2

namespace MatrixOS::USB::MIDI
{
  void Init();
}

void tud_midi_rx_cb(uint8_t itf);