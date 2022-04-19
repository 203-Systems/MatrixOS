#include "Device.h"

namespace Device::KeyPad
{
    void InitTouchBar()
    {
        //Set Touch Data Pin
        gpio_set_direction(TouchData_Pin, GPIO_MODE_INPUT);

        //Set Touch Clock Pin
        gpio_set_direction(TouchClock_Pin, GPIO_MODE_OUTPUT);
    }

    void TouchBarScan()
    {
        uint16_t result = 0; //Bit Map
        for (uint8_t i = 0; i < 16; i++)
        {
            gpio_set_level(TouchClock_Pin, 1);

            Fract16 reading = gpio_get_level(TouchData_Pin) * UINT16_MAX;

            gpio_set_level(TouchClock_Pin, 0);

            bool updated = touchbarState[i].update(reading);
            if(updated)
            {
                uint16_t keyID = (2 << 12) + touchbar_map[i];
                if(addToList(keyID))
                {
                    return; //List is full
                }
            }
        }
    }
}