#pragma once

#include "MatrixOS.h"
#include "application/Application.h"

class TestApp : public Application
{   
    public:
    string name = "TestApp";
    string author = "203 Electronics";
    uint32_t version = 0;

    Timer timer;
    uint32_t led_id = 0;
    uint8_t colorIndex = 0;
    Color colorList[5] = {Color(64, 64, 64), Color(127, 0, 0), Color(0, 127, 0), Color(0, 0, 127), Color(0, 0, 0)};
    
    // void Setup() override;
    void Loop() override;

    void KeyEvent(uint16_t keyID, KeyInfo keyInfo) override;
    void MidiEvent(MidiPacket midiPacket) override;

    void LED_task(void);
};