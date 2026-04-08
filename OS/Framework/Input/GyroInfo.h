#pragma once

#include <type_traits>

#include "InputConstants.h"

enum class GyroState : uint8_t {
  Idle = 0,
  Changed,
  Invalid = 255u,
};

struct GyroInfo {
  uint32_t lastEventTime;
  int16_t xRate;
  int16_t yRate;
  int16_t zRate;
  GyroState state;
  uint8_t reserved = 0;
};

static_assert(sizeof(GyroInfo) <= inputInfoMaxSize, "GyroInfo must be <= 16 bytes");
static_assert(std::is_trivially_copyable_v<GyroInfo>, "GyroInfo must be trivially copyable");
