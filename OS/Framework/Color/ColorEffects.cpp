#include "MatrixOS.h"
#include "ColorEffects.h"
#include <math.h>

namespace ColorEffects
{
    Color Rainbow(uint16_t period, int32_t offset)
    {
        float hue = ((MatrixOS::SYS::Millis() - offset) % period) / (float)period;
        return Color::HsvToRgb(hue, 1.0, 1.0);
    }

    uint8_t Breath(uint16_t period, int32_t offset)
    {
       float brightness = (cos(2 * M_PI * (MatrixOS::SYS::Millis() - offset + period / 2) / (period)) + 1) / 2 * 255;
       return (uint8_t)brightness;
    }

    Color ColorBreath(Color color, uint16_t period, int32_t offset)
    {
        return color.Scale(Breath(period, offset)).Gamma();
    }

    uint8_t BreathLowBound(uint8_t lowBound, uint16_t period, int32_t offset)
    {
       float brightness = ((cos(2 * M_PI * (MatrixOS::SYS::Millis() - offset + period / 2) / (period)) + 1) / 2) * (255 - lowBound) + lowBound;
       return (uint8_t)brightness;
    }

    Color ColorBreathLowBound(Color color, uint8_t lowBound, uint16_t period, int32_t offset)
    {
        return color.Scale(BreathLowBound(lowBound, period, offset)).Gamma();
    }

    uint8_t Strobe(uint16_t period, int32_t offset)
    {
        return (MatrixOS::SYS::Millis() - offset) % period < (period / 2) ? 255 : 0;
    }

    Color ColorStrobe(Color color, uint16_t period, int32_t offset)
    {
        return color.Scale(Strobe(period, offset));
    }

    uint8_t Saw(uint16_t period, int32_t offset)
    {
        return (MatrixOS::SYS::Millis() - offset) % period * 255 / period;
    }

    Color ColorSaw(Color color, uint16_t period, int32_t offset)
    {
        return color.Scale(Saw(period, offset)).Gamma();
    }

    uint8_t Triangle(uint16_t period, int32_t offset)
    {
        uint16_t timeInPeriod = (MatrixOS::SYS::Millis() - offset) % period;
        return timeInPeriod < (period / 2) ? (timeInPeriod * 255 / (period / 2)) : ((period - timeInPeriod) * 255 / (period / 2));
    }

    Color ColorTriangle(Color color, uint16_t period, int32_t offset)
    {
        return color.Scale(Triangle(period, offset)).Gamma();
    }
}