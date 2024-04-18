#pragma once

#include "MatrixOS.h"
#include "../Application.h"
#include "system/USB/MIDI.h"

class BootAnimation : public Application {
 public:
  uint8_t stage = 0;
  bool idle_hold = false;

  // MatrixOS::USB::MIDI usbMidi = MatrixOS::USB::MIDI();
  void Loop() override;

  virtual bool Idle(bool ready) {
    return true;
  };  // return true will signal the Boot Animation to enter Boot Animation
  virtual void Boot(){};

  virtual void End(){};
  virtual void KeyEvent(uint16_t KeyID, KeyInfo* keyInfo);
};