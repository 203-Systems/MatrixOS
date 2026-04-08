#pragma once

#include <stdint.h>

enum class InputClass : uint8_t {
  Unknown = 0,
  Keypad,
  Fader,
  Encoder,
  TouchArea,
  Gyro,
  Accelerometer,
  Temperature,
  Battery,
  Generic,
};

enum class InputClusterShape : uint8_t {
  Scalar = 0,
  Linear1D,
  Grid2D,
  Area2D,
};
