#pragma once

#include <type_traits>

#include "InputConstants.h"

enum class GenericState : uint8_t {
  Idle = 0,
  Changed,
  Invalid = 255u,
};

struct GenericInfo {
  uint64_t value;
  uint32_t lastEventTime;
  GenericState state;
};

static_assert(sizeof(GenericInfo) <= inputInfoMaxSize, "GenericInfo must be <= 16 bytes");
static_assert(std::is_trivially_copyable_v<GenericInfo>, "GenericInfo must be trivially copyable");
