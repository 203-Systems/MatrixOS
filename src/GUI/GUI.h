#ifndef __GUI_H
#define __GUI_H

#include "../system/MatrixOS.h"

namespace GUI
{
    void textScroll(char ascii[], Color color, uint8_t speed = 10, bool loop = false, uint8_t length = 0);
    void renderAsciiChar(char ascii, CPoint xy, Color color);
    void renderHalfHeightNum(uint16_t num, CPoint xy, Color color);
    // void renderHalfHeightDigit(uint8_t num, CPoint xy, Color color);
    uint8_t Input8bit(uint8_t currentNum, CPoint xy, Color color);
    uint8_t Input8bitBinary(uint8_t currentNum, CPoint xy, Color color);
    uint8_t Input8bitSimple(uint8_t currentNum, CPoint xy, Color color);
}

#endif
