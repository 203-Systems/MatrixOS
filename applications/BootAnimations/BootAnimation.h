#pragma once

#include "MatrixOS.h"
#include "../Application.h"

class BootAnimation : public Application {
 public:
  void Loop() override;

  virtual bool Idle(bool ready) {
    return false;
  };  // return true will keep render idle animation even when device is ready
  virtual void Boot(){};

  virtual void End(){};
  virtual void KeyEvent(uint16_t KeyID, KeyInfo* keyInfo);
};