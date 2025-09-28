#pragma once

#include "MatrixOS.h"
#include "MidiEffect.h"

// Note Latch Effect - Latches MIDI note-ons until all are turned off
// When all latched notes are off, the next note-on will release all latched notes
// AfterTouch from the first still-holding note is mirrored to all latched notes
class NoteLatch : public MidiEffect {
private:
    std::vector<uint8_t> latchedNotes;      // Notes that are latched (sustaining)
    std::vector<uint8_t> stillHoldingNotes; // Notes that are physically still being held
    bool disableOnNextTick = false;
    bool toggleMode = false;
public:
    void Tick(deque<MidiPacket>& input, deque<MidiPacket>& output) override;
    void Reset() override;
    void SetEnabled(bool state) override;
    void SetToggleMode(bool enable);
    bool IsToggleMode() const { return toggleMode; }

private:
    void ProcessNoteMessage(const MidiPacket& packet, deque<MidiPacket>& output);
    void ProcessAfterTouch(const MidiPacket& packet, deque<MidiPacket>& output);
    void ProcessNoteMessageToggleMode(const MidiPacket& packet, deque<MidiPacket>& output);
    void ProcessAfterTouchToggleMode(const MidiPacket& packet, deque<MidiPacket>& output);
    void ReleaseAllLatchedNotes(deque<MidiPacket>& output);
};