//Define Device Keypad Function
#include "Device.h"

namespace Device::KeyPad
{
    uint16_t Scan()
    {

    }

    KeyInfo GetKey(Point keyXY)
    {
        uint16_t keyID = XY2ID(keyXY);
        return GetKey(keyID);
        
    }

    KeyInfo GetKey(uint16_t keyID)
    {
        uint8_t keyClass = keyID >> 14;
        switch(keyClass)
        {   
            case 0: //System
            {
                uint16_t index = keyID & (0b0011111111111111);
                switch(index)
                {
                    case 0:
                        return fnState;
                }
                break;
            }
            case 1: //Main Grid
            {
                int16_t x = (keyID & (0b0011111110000000)) >> 7;
                int16_t y = keyID & (0b0000000001111111);
                if(x < x_size && y < y_size)  return keypadState[x][y];
                break;
            }
            case 2: //Touch Bar
            {
                uint16_t index = keyID & (0b0011111111111111);
                if(index < touchbar_size) return touchbarState[index];
                break;
            }
        }
        return KeyInfo(); //Return an empty KeyInfo
    }

    void FNScan()
    {
        fnState.update(HAL_GPIO_ReadPin(FN_GPIO_Port, FN_Pin));
    }

    void KeyPadScan()
    {
        // bool changed = false;

        // changed = changed || scanFN();

        // for(u8 x = 0; x < XSIZE; x ++)
        // {
        //     digitalWrite(keypad_pins[x], HIGH);
        //     for(u8 y = 0; y < YSIZE; y ++)
        //     {
        //     keypadState[x][y] = KeyPad::updateKey(keypadState[x][y], digitalRead(keypad_pins[y+8]));
        //     if(keypadState[x][y].changed)
        //     {
        //         changed = true;
        //         #ifdef DEBUG
        //         CompositeSerial.print("Key Action ");
        //         CompositeSerial.print(xytoxy(x,y), HEX);
        //         CompositeSerial.print(" ");
        //         CompositeSerial.print(keypadState[x][y].state);
        //         CompositeSerial.print(" ");
        //         CompositeSerial.println(keypadState[x][y].velocity);
        //         #endif
        //         if(!KeyPad::addtoList(xytoxy(x,y)))
        //         return changed;
        //     }
        //     }
        //     digitalWrite(keypad_pins[x], LOW);
        //     delayMicroseconds(10); // To make the Keypad Matrix happy
        // }

        // return changed;
    }

    void TouchBarScan()
    {
        
    }

    uint16_t XY2ID(Point xy)
    {
        if(xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8) //Main grid
        {
            return (1 << 12) + (xy.x << 7) + xy.y;
        }
        else if(xy.x >= 0 && xy.x < 8 && xy.y == 8) //Touch Bar
        {
            return (2 << 12) + xy.x;
        }
        return UINT16_MAX;
    }

    //Matrix use the following ID Struct
    // CCCC IIIIIIIIIIII
    // C as class (4 bits), I as index (12 bits). I could be spilted by the class defination, for example, class 0 (grid), it's spilted to XXXXXXX YYYYYYY.
    // Class List:
    // Class 0 - System - IIIIIIIIIIII
    // Class 1 - Grid - XXXXXX YYYYYY
    // Class 2 - TouchBar - IIIIIIIIIIII
    // Class 3 - Underglow - IIIIIIIIIIII

    Point ID2XY(uint16_t keyID)
    {
        uint8_t keyClass = keyID >> 12;
        switch(keyClass)
        {
            case 1: //Main Grid
            {
                int16_t x = (keyID & (0b00001111110000000)) >> 6;
                int16_t y = keyID & (0b0000000000111111);
                if(x < Device::x_size && y < Device::y_size)  return Point(x, y);
                break;
            }
            case 2: //TouchBar
            {
                uint16_t index = keyID & (0b0000111111111111);
                if(index < Device::touchbar_size) return Point(Device::y_size, index);
                break;
            }
        }
        return Point(INT16_MIN, INT16_MIN);
    }
}