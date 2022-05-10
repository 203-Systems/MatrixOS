#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"

#define APPLICATION_NAME "Matrix Factory Test"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0xFFFFFF)
#define APPLICATION_VERSION 1
#define APPLICATION_CLASS FactoryTest
#define APPLICATION_VISIBLITY false

//I know this is very wasteful to memory but it is just a factory test so who gives a fuck
class FactoryTest : public Application
{   
    public:
    void Setup() override;
    void Loop() override;

    uint8_t current_test = 0;
    
    uint32_t led_counter = 0;
    const Color colors[8] = {Color(0xFFFFFF), Color(0xFF0000), Color(0xFFFF00), Color(0x00FF00), Color(0x00FFFF), Color(0x0000FF), Color(0xFF00FF), Color(0x000000)};
    void LEDTest();

    bool keypad_tested[Device::x_size][Device::y_size];
    void KeyPadTest();

    bool touchbar_tested[32];
    void TouchBarTest();

    void KeyEvent(uint16_t keyID, KeyInfo keyInfo) override;
};

#include "applications/RegisterApplication.h"