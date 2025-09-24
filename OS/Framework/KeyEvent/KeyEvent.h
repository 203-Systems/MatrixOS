#pragma once

#include "KeyInfo.h"

struct KeyEvent {
  uint16_t id;
  KeyInfo info;

  uint16_t ID() {return id;}
  // Pass-through functions to KeyInfo
  KeyState State() { return info.State(); }
  bool Hold() { return info.Hold(); }
  uint32_t HoldTime(void) { return info.HoldTime(); }
  bool Active() { return info.Active(); }
  operator bool() { return info.operator bool(); }
  Fract16 Force() const { return info.Force(); }
  Fract16 Value(uint8_t index = 0) const { return info.Value(index); }
};