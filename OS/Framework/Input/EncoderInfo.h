#pragma once

#include <type_traits>

#include "InputConstants.h"
#include "Fract16.h"

enum class EncoderState : uint8_t {
  Idle = 0,
  Decrement,
  Increment,
  Invalid = 255u,
};

struct EncoderInfo {
  uint32_t lastEventTime;
  EncoderState state;
};

static_assert(sizeof(EncoderInfo) <= inputInfoMaxSize, "EncoderInfo must be <= 16 bytes");
static_assert(std::is_trivially_copyable_v<EncoderInfo>, "EncoderInfo must be trivially copyable");
