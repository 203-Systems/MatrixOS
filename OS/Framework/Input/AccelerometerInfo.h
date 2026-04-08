#pragma once

#include <type_traits>

#include "InputConstants.h"

enum class AccelerometerState : uint8_t {
  Idle = 0,
  Changed,
  Invalid = 255u,
};

struct AccelerometerInfo {
  uint32_t lastEventTime;
  int16_t xAcceleration;
  int16_t yAcceleration;
  int16_t zAcceleration;
  AccelerometerState state;
};

static_assert(sizeof(AccelerometerInfo) <= inputInfoMaxSize, "AccelerometerInfo must be <= 16 bytes");
static_assert(std::is_trivially_copyable_v<AccelerometerInfo>, "AccelerometerInfo must be trivially copyable");
