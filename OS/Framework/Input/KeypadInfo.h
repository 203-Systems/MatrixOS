#pragma once

#include <type_traits>

#include "InputConstants.h"
#include "Fract16.h"

enum class KeypadState : uint8_t {
  Idle = 0,
  Activated,
  Pressed,
  Hold,
  Aftertouch,
  Released,
  Debouncing = 240u,
  ReleaseDebouncing,
  Invalid = 255u,
};

// Forward-declare Millis() so the .cpp file can call it.
namespace MatrixOS::SYS { uint64_t Millis(void); }

#define KEY_SCAN_THRESHOLD 512

// Device-layer key-scan configuration (thresholds, curves, debounce).
struct KeyScanConfig {
  bool applyCurve;
  Fract16 lowThreshold;
  Fract16 highThreshold;
  Fract16 activationOffset;
  uint16_t debounce;
};

// Portable per-key state.
// Holds the snapshot fields forwarded to the OS via InputEvent *and* the
// device-layer debounce / state-machine logic that drivers need.
struct KeypadInfo {
  static constexpr uint16_t kHoldThreshold = 400;

  uint32_t lastEventTime;
  Fract16 pressure;
  Fract16 velocity;
  KeypadState state;
  struct {
    uint8_t hold : 1;
    uint8_t cleared : 1;
    uint8_t reserved : 6;
  };

  bool Hold() const {
    return hold;
  }

  bool Cleared() const {
    return cleared;
  }

  bool Active() const {
    return (state >= KeypadState::Activated && state <= KeypadState::Aftertouch) || state == KeypadState::ReleaseDebouncing;
  }

  // Device-layer scan methods (implementations in KeypadInfo.cpp).
  Fract16 ApplyForceCurve(KeyScanConfig& config, Fract16 value);
  bool Update(KeyScanConfig& config, Fract16 newValue);
  void Clear();
  uint32_t HoldTime();
};

struct KeypadCapabilities {
  bool hasPressure;
  bool hasAftertouch;
  bool hasVelocity;
  bool hasPosition;      // Supports sub-key position tracking
};

static_assert(sizeof(KeypadInfo) <= inputInfoMaxSize, "KeypadInfo must be <= 16 bytes");
