#pragma once

#include <stdint.h>
#include <vector>
#include "SavedVar.h"
#include "Types.h"
#include "System/Parameters.h"
#include "KeyConfig.h"

#define KEY_INFO_THRESHOLD 512
#define KEY_INFO_VALUE_COUNT 3

enum KeyState : uint8_t {
    /*Status Keys*/
    IDLE,
    ACTIVATED,
    /*Event Keys*/
    PRESSED,
    HOLD,
    AFTERTOUCH,
    RELEASED,
    /*Special*/
    DEBUNCING = 240u,
    RELEASE_DEBUNCING = 241u,
    /*Placeholder Keys*/
    INVALID = 255u
};

  
struct KeyInfo {
  // Bit-packed structure for state and flags
  uint32_t lastEventTime = 0;
  KeyState state;
  struct {
    bool hold : 1;
    bool cleared : 1;
  };
  Fract16 values[KEY_INFO_VALUE_COUNT]; // Kind wasteful, TODO revise later.

  // Constructor
  KeyInfo() : state(IDLE), hold(0), cleared(0), values{0, 0, 0} {}
  Fract16 ApplyForceCurve(KeyConfig& config, Fract16 value);
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