#ifndef __TestApp_H
#define __TestApp_H

#include "framework/Timer.h"
// #include "application/Application.h"
#include "tusb.h"

class TestApp
{
    // using Application::Application;
    public:
    char name[64] = "Matrix OS TestApp";
    char author[64] = "203 Electronics";
    uint32_t version = 10000;

    Timer timer;
    uint32_t led_id = 0;
    uint8_t colorIndex = 0;
    Color colorList[5] = {Color(64, 64, 64), Color(127, 0, 0), Color(0, 127, 0), Color(0, 0, 127), Color(0, 0, 0)};
    
    void main();

    void midi_task(void);
    void LED_task(void);
};

#endif