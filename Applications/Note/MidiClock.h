#pragma once

#include "MatrixOS.h"

class MidiClock {
private:
    uint16_t bpm;
    uint8_t ppqn;
    uint32_t last_tick;
    uint32_t tick_count;
    uint32_t pulse_length; // microseconds

public:
    MidiClock(uint16_t bpm, uint8_t ppqn = 96);

    uint8_t PPQN();
    void SetBPM(uint16_t bpm);
    uint32_t TickCount();
    bool Tick();
};