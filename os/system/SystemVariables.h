// This file is for system use variables
#pragma once

#include "framework/Framework.h"

#define REGISTERED_APP_INFOS
#define REGISTERED_APP_SWITCH

namespace MatrixOS::SysVar
{
  inline uint16_t fps_millis;
  inline uint16_t keypad_millis;

  inline bool led_update = true;
  inline bool keypad_scan = true;
}