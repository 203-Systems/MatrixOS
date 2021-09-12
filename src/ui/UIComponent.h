#pragma once

#include "MatrixOS.h"

namespace UIComponent
    void textScroll(char ascii[], Color color, uint8_t speed = 10, bool loop = false, uint8_t length = 0);
    void renderAsciiChar(char ascii, Point xy, Color color);
    void renderHalfHeightNum(uint16_t num, Point xy, Color color);
    // void renderHalfHeightDigit(uint8_t num, Point xy, Color color);
    uint8_t Input8bit(uint8_t currentNum, Point xy, Color color);
    uint8_t Input8bitBinary(uint8_t currentNum, Point xy, Color color);
    uint8_t Input8bitSimple(uint8_t currentNum, Point xy, Color color);
}
