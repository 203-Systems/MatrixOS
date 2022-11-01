#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"

class WirelessRepeater : public Application {
 public:
  string name = "WirelessRepeater";
  string author = "203 Electronics";
  uint32_t version = 0;

  // void Setup() override;
  // void Loop() override;

  // void KeyEvent(uint16_t keyID, KeyInfo keyInfo) override;
  void MidiEvent(MidiPacket midiPacket) override;
};