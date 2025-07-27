#pragma once

#include <string>

struct LEDPartition {
    std::string name;
    float default_multiplier;
    uint16_t start;
    uint16_t size;
};
