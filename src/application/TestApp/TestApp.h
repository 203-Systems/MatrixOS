#pragma once

#include "MatrixOS.h"
#include "framework/Timer.h"
// #include "application/Application.h"
#include "tusb.h"

class TestApp
{
    // using Application::Application;
    public:
    char name[18] = "Matrix OS TestApp";
    char author[16] = "203 Electronics";
    uint32_t version = 10000;

    Timer timer;
    uint32_t led_id = 0;
    uint8_t colorIndex = 0;
    Color colorList[5] = {Color(64, 64, 64), Color(127, 0, 0), Color(0, 127, 0), Color(0, 0, 127), Color(0, 0, 0)};
    
    void main();

    // void midi_task(void);
    static void note_on_handler(uint8_t channel, uint8_t note, uint8_t velocity);
    static void note_off_handler(uint8_t channel, uint8_t note, uint8_t velocity);
    void LED_task(void);
};