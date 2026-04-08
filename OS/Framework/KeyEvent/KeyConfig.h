#pragma once

#include "Types.h"

struct KeyConfig {
  bool applyCurve;
  Fract16 lowThreshold;
  Fract16 highThreshold;
  Fract16 activationOffset;
  uint16_t debounce;
};