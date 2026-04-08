#pragma once

#include <type_traits>

#include "InputConstants.h"

enum class BatteryState : uint8_t {
  Idle = 0,
  Changed,
  Invalid = 255u,
};

struct BatteryInfo {
  uint32_t lastEventTime;
  uint16_t milliVolts;
  uint8_t percentage;
  BatteryState state;
  struct {
    uint8_t charging : 1;
    uint8_t full : 1;
    uint8_t reserved : 6;
  };
};

static_assert(sizeof(BatteryInfo) <= inputInfoMaxSize, "BatteryInfo must be <= 16 bytes");
static_assert(std::is_trivially_copyable_v<BatteryInfo>, "BatteryInfo must be trivially copyable");
