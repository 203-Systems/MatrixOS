#ifndef __MatrixBlock5__LED__
#define __MatrixBlock5__LED__

#include "MatrixOS.h"

namespace Device
{
    namespace LED
    {
        void Update(Color* frameBuffer, uint8_t brightness) //Render LED
        {
            WS2812::Show(frameBuffer);
        }

        uint16_t XY2Index(uint8_t GridID, Point xy)
        {
            switch(GridID)
            {
                case 0: //Main Grid
                    return xy.x * 8 + xy.y;
                case 1: //Under Glow 
                    MatrixOS::SYS::Error_Handler();
                    return -1;
            }
            return 0;
        }

        uint16_t ID2Index(uint8_t GridID, uint16_t index)
        {
            switch(GridID)
            {
                case 0: //Main Grid
                    return index;
                case 1: //Under Glow
                    return index + 64;
            }
            return 0;
        }
    }
}

#endif