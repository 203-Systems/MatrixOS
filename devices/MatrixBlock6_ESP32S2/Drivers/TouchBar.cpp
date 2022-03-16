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

    uint8_t last_result = 0;
    void TouchBarScan()
    {
        uint16_t result = 0; //Bit Map
        char result_text[16];
        for (uint8_t i = 0; i < 16; i++)
        {
            gpio_set_level(TouchClock_Pin, 1);

            bool reading = gpio_get_level(TouchData_Pin);

            // changed |= rawInput[lut[i]] != reading;
            result &= reading << i;
            // result_text[i] = reading ? "1" : "0";

            gpio_set_level(TouchClock_Pin, 0);
        }


        // Process data here!

        //Print Data
        // MatrixOS::Logging::LogDebug("TouchBar", std::to_string(result));
        // ESP_LOGI("TouchBar", "%d", result);
        last_result = result;
    }
}