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
        for (uint8_t i = 0; i < 16; i++)
        {
            gpio_set_level(TouchClock_Pin, 1);

            Fract16 reading = gpio_get_level(TouchData_Pin) * UINT16_MAX;

            gpio_set_level(TouchClock_Pin, 0);

            uint8_t key_id = touchbar_map[i];
            bool updated = touchbarState[key_id].update(reading);
            if(updated)
            {
                uint16_t keyID = (2 << 12) + key_id;
                if(addToList(keyID))
                {
                    return; //List is full
                }
            }
        }
    }
}