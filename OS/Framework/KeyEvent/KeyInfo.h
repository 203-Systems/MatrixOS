#pragma once

#include <stdint.h>
#include <vector>
#include "SavedVar.h"
#include "Types.h"
#include "system/Parameters.h"
#include "KeyConfig.h"

#define KEY_INFO_THRESHOLD 512
#define KEY_INFO_VALUE_COUNT 3

enum KeyState : uint8_t {
  /*Placeholder Keys*/
  INVALID = 0,
  /*Status Keys*/
  IDLE,
  ACTIVATED,
  /*Event Keys*/
  PRESSED,
  RELEASED,
  HOLD,
  AFTERTOUCH
};

struct KeyInfo {
  // Bit-packed structure for state and flags
  uint32_t lastEventTime = 0;
  KeyState state;
  struct {
    bool debouncing : 1;
    bool hold : 1;
    bool cleared : 1;
  };
  Fract16 values[KEY_INFO_VALUE_COUNT];

  // Constructor
  KeyInfo() : state(IDLE), debouncing(0), hold(0), cleared(0), values{0, 0, 0} {}
  Fract16 ApplyVelocityCurve(KeyConfig& config, Fract16 value);
  bool Update(KeyConfig& config, Fract16 new_value);    // Convenience method for single value
  bool UpdateRaw(uint8_t index, Fract16 new_value);    // Update raw value
  void Clear();

  // User access methods
  KeyState State();
  bool Hold();
  uint32_t HoldTime(void); // Note that this is inaccurate if release debounce rejected.
  bool Active();
  operator bool();
  Fract16 Force() const;
  Fract16 Value(uint8_t index = 0) const;
};