#pragma once

#include <type_traits>

#include "InputConstants.h"
#include "Fract16.h"

enum class FaderState : uint8_t {
  Idle = 0,
  Changed,
  Invalid = 255u,
};

struct FaderInfo {
  uint32_t lastEventTime;
  Fract16 position;
  FaderState state;
};

static_assert(sizeof(FaderInfo) <= inputInfoMaxSize, "FaderInfo must be <= 16 bytes");
static_assert(std::is_trivially_copyable_v<FaderInfo>, "FaderInfo must be trivially copyable");
