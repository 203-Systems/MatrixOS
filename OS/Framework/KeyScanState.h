#pragma once

#include "Input/KeypadInfo.h"  // KeypadInfo, KeypadState, Fract16

// Forward-declare Millis() so the .cpp file can call it.
namespace MatrixOS::SYS
{
uint64_t Millis(void);
}

#define KEY_SCAN_THRESHOLD 512

// Device-layer key-scan configuration (thresholds, curves, debounce).
struct KeyScanConfig {
  bool applyCurve;
  Fract16 lowThreshold;
  Fract16 highThreshold;
  Fract16 activationOffset;
  uint16_t debounce;
};

// Device-layer per-key state tracker.
// Extends the portable KeypadInfo snapshot with the debouncing / state-machine
// logic that was formerly in KeyInfo.  Device drivers store one of these for
// every physical key; only the base KeypadInfo portion is forwarded to the OS
// via InputEvent.
struct KeyScanState : KeypadInfo {
  static constexpr uint16_t kHoldThreshold = 400;

  KeyScanState() {
    lastEventTime = 0;
    pressure = 0;
    velocity = 0;
    state = KeypadState::Idle;
    hold = 0;
    cleared = 0;
    reserved = 0;
  }

  Fract16 ApplyForceCurve(KeyScanConfig& config, Fract16 value);
  bool Update(KeyScanConfig& config, Fract16 newValue);
  void Clear();
  uint32_t HoldTime();
};
