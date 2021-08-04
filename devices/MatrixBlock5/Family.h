//Declear Family specific function
#ifndef __FAMILY__
#define __FAMILY__

#include "stm32f1xx_hal.h"

namespace Device
{
    void SystemClock_Config();
    void GPIO_Init();
    void LED_Init();
}

#endif