#pragma once

#include <type_traits>

#include "InputConstants.h"

enum class TemperatureState : uint8_t {
  Idle = 0,
  Changed,
  Invalid = 255u,
};

struct TemperatureInfo {
  uint32_t lastEventTime;
  int16_t centiDegreesC;
  TemperatureState state;
};

static_assert(sizeof(TemperatureInfo) <= inputInfoMaxSize, "TemperatureInfo must be <= 16 bytes");
static_assert(std::is_trivially_copyable_v<TemperatureInfo>, "TemperatureInfo must be trivially copyable");
