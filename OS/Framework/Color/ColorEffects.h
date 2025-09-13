#pragma once

#include "Color.h"

namespace ColorEffects
{
    Color Rainbow(uint16_t period = 1000, int32_t offset = 0);
    uint8_t Breath(uint16_t period = 1000, int32_t offset = 0);
    Color ColorBreath(Color color, uint16_t period = 1000, int32_t offset = 0);
    uint8_t BreathLowBound(uint8_t lowBound = 64, uint16_t period = 1000, int32_t offset = 0);
    Color ColorBreathLowBound(Color color, uint8_t lowBound = 64, uint16_t period = 1000, int32_t offset = 0);
    uint8_t Strobe(uint16_t period = 1000, int32_t offset = 0);
    Color ColorStrobe(Color color, uint16_t period = 1000, int32_t offset = 0);
    uint8_t Saw(uint16_t period = 1000, int32_t offset = 0);
    Color ColorSaw(Color color, uint16_t period = 1000, int32_t offset = 0);
    uint8_t Triangle(uint16_t period = 1000, int32_t offset = 0);
    Color ColorTriangle(Color color, uint16_t period = 1000, int32_t offset = 0);
}