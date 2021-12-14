#include "Device.h"

namespace Device
{   

    void LED_Init()
    {
        WS2812::Init(RMT_CHANNEL_0, LED_Pin, numsOfLED);
    }

    namespace LED
    {
        void Update(Color* frameBuffer, uint8_t brightness) //Render LED
        {
            // ESP_LOGI("LED", "LED Update");
            // ESP_LOGI("LED", "%d", frameBuffer[0].RGB());
            WS2812::Show(frameBuffer, brightness);
        }

        uint16_t XY2Index(Point xy)
        {
            if(xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8) //Main grid
            {
                return xy.x + xy.y * 8;
            }
            return UINT16_MAX;
        }

        // Point Index2XY(uint16_t index)
        // {
        //     if(xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8) //Main grid
        //     {
        //         return xy.x + xy.y * 8;
        //     }
        //     if(index < 64)
        //     {

        //     }
        //     return UINT16_MAX;
        // }

        //TODO This text is very wrong (GRID)
        //Matrix use the following ID Struct
        // CCCC IIIIIIIIIIII
        // C as class (4 bits), I as index (12 bits). I could be spilted by the class defination, for example, class 1 (grid), it's spilted to XXXXXXX YYYYYYY.
        // Class List:
        // Class 0 - System - IIIIIIIIIIII
        // Class 1 - Grid - XXXXXX YYYYYY
        // Class 2 - TouchBar - IIIIIIIIIIII
        // Class 3 - Underglow - IIIIIIIIIIII

        uint16_t ID2Index(uint16_t ledID)
        {
            uint8_t ledClass = ledID >> 12;
            switch(ledClass)
            {
                case 1: //Main Grid
                {
                    uint16_t index = ledID & (0b0000111111111111);
                    if(index < 64) return index;
                    break;
                }
                case 3: //Underglow
                    break;//TODO: Underglow
            }
            return UINT16_MAX;
        }
    }
}