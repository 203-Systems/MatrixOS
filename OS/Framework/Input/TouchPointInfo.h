#pragma once

#include <type_traits>

#include "InputConstants.h"
#include "Fract16.h"
#include "PointFloat.h"

enum class TouchPointState : uint8_t {
  Idle = 0,
  Pressed,
  Hold,
  Changed,
  Released,
  Invalid = 255u,
};

struct TouchPointInfo {
  uint32_t lastEventTime;
  PointFloat point;
  Fract16 pressure;
  TouchPointState state;
};

struct TouchAreaCapabilities {
  bool hasPressure;
  bool supportsMultiTouch;
};

static_assert(sizeof(TouchPointInfo) <= inputInfoMaxSize, "TouchPointInfo must be <= 16 bytes");
static_assert(std::is_trivially_copyable_v<TouchPointInfo>, "TouchPointInfo must be trivially copyable");
