//Declear Family specific function
#pragma once

#include "stm32f1xx_hal.h"
#include "WS2812/WS2812.h"
#include "Drivers/NVS.h"

namespace Device
{
    void SystemClock_Config();
    void USB_Init();
    void LED_Init();
    void KeyPad_Init();
    void TouchBar_Init();
}