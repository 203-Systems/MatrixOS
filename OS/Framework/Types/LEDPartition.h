#pragma once

#include <string>

enum LEDType {
  MONO_1B = 0x01,
  MONO_8B = 0x08,

  RGB_24B = 0x10,

  RGBW_32B_6K5 = 0x20
};

struct LEDPartition {
    std::string name;
    float default_multiplier;
    uint16_t start;
    uint16_t size;
    LEDType type;
};
