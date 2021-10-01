//Declear Family specific function
#pragma once

#include "stm32f1xx_hal.h"
#include "WS2812.h"
#include "Drivers/EEPROM.h"

namespace Device
{
    void SystemClock_Config();
    void USB_Init();
    void LED_Init();
    void KeyPad_Init();
    void TouchBar_Init();
    void EEPROM_Init();

    // namespace EEPROM
    // {
    //     int16_t FindKey(uint32_t hash);
    //     HashKey GetKey(uint32_t hash);
    // }
}