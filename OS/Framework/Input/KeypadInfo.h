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

struct KeypadInfo {
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
};

struct KeypadCapabilities {
  bool hasPressure;
  bool hasAftertouch;
  bool hasVelocity;
  bool hasPosition;      // Supports sub-key position tracking
};

static_assert(sizeof(KeypadInfo) <= inputInfoMaxSize, "KeypadInfo must be <= 16 bytes");
static_assert(std::is_trivially_copyable_v<KeypadInfo>, "KeypadInfo must be trivially copyable");
