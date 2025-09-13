#pragma once

#include "Types.h"

struct KeyConfig {
  bool apply_curve;
  Fract16 low_threshold;
  Fract16 high_threshold;
  Fract16 activation_offset;
  uint16_t debounce;
};