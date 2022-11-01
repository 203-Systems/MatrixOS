// Declear Family specific function
#pragma once

#include "stm32f1xx_hal.h"
#include "WS2812/WS2812.h"
#include "Drivers/NVS.h"

#define FUNCTION_KEY 0  // Keypad Code for main function key

namespace Device
{
  void SystemClock_Config();
  void USB_Init();
  void LED_Init();
  void KeyPad_Init();
  void TouchBar_Init();
}

#undef USB  // CMSIS defined the USB, undef so we can use USB as MatrixOS namespace