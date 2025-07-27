#pragma once

#include "MatrixOS.h"
#include "../Application.h"

class BootAnimation : public Application {
 public:
  void Loop() override;

  uint8_t stage = 0;
  bool idle_hold = false;

  virtual bool Idle(bool ready) {
    return true;
  };  // return true will signal the Boot Animation to enter Boot Animation
  virtual void Boot(){};

  virtual void End(){};
  virtual void KeyEvent(uint16_t KeyID, KeyInfo* keyInfo);
};