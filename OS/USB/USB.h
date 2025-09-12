#pragma once

#define USB_MIDI_COUNT 2

namespace MatrixOS::USB
{
  void Init();

  namespace MIDI
  {
    void Init();
  }
}