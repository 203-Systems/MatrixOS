#pragma once

#include <stdint.h>
#include "SavedVar.h"
#include "Types.h"
#include "system/Parameters.h"
#include "KeyConfig.h"

#define KEY_INFO_THRESHOLD 512

enum KeyState : uint8_t { 
  /*Status Keys*/ 
  IDLE,
  ACTIVATED,
  /*Event Keys*/ 
  PRESSED,
  RELEASED,
  HOLD,
  AFTERTOUCH,
  /*Special*/ 
  DEBUNCING = 240u, 
  RELEASE_DEBUNCING = 241u,
  /*Placeholder Keys*/ 
  INVALID = 255u 
};

struct KeyInfo {
  KeyState state = IDLE;
  uint32_t lastEventTime = 0;  // PRESSED and RELEASED event only
  Fract16 velocity = 0;
  bool hold = false;
  bool cleared = false;

  KeyInfo();
  bool Active();
  operator bool();
  uint32_t HoldTime(void);
  Fract16 ApplyVelocityCurve(KeyConfig& config, Fract16 velocity);
  bool Update(KeyConfig& config, Fract16 new_velocity);
  void Clear();
};