//This file is for system use variables
#pragma once

#include "framework/Framework.h"

class Application;

namespace MatrixOS::SysVar
{
    inline Application* active_app = nullptr;

    inline uint16_t fps_millis;
    inline uint16_t keypad_millis;

    inline bool led_update = true;
    inline bool keypad_scan = true;
}