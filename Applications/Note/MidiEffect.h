#pragma once

#include "MatrixOS.h"

// Base class for MIDI effects - processes arrays of packets
class MidiEffect {
protected:
    bool enabled = true;

public:
    virtual ~MidiEffect() = default;

    // Process input array and populate output array
    // Input: packets to process (can be empty for generators like arpeggiator/LFO)
    // Output: processed packets (effect should append to this)
    virtual void Tick(deque<MidiPacket>& input, deque<MidiPacket>& output) = 0;

    // Called when effect is reset - cleanup state
    virtual void Reset() {}

    // Enable/disable control
    virtual void SetEnabled(bool state) { enabled = state; }
    bool IsEnabled() const { return enabled; }
};