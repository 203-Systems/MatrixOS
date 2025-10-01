#include "MidiClock.h"

MidiClock::MidiClock(uint16_t bpm, uint8_t ppqn) {
    this->ppqn = ppqn;
    tick_count = 0;
    last_tick = 0;
    SetBPM(bpm);
}

uint8_t MidiClock::PPQN() {
    return ppqn;
}

void MidiClock::SetBPM(uint16_t bpm) {
    this->bpm = bpm;
    pulse_length = 60000000UL / (bpm * ppqn);
}

uint32_t MidiClock::TickCount() {
    return tick_count;
}

bool MidiClock::Tick() {
    uint32_t micros = MatrixOS::SYS::Micros();
    uint32_t elapsed = micros - last_tick;

    if (elapsed >= pulse_length) {

        if (elapsed >= pulse_length * 2) {
            // If we've fallen behind significantly (missed multiple ticks), resync
            last_tick = micros;
        }
        else {
            // Normal tick, maintain consistent timing by adding pulse_length
            last_tick = micros;
        }
        tick_count++;
        return true;
    }

    return false;
}