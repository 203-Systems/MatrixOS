#pragma once

#include "MatrixOS.h"
#include "MidiEffect.h"
#include <vector>
#include <deque>

using std::vector;
using std::deque;

enum ArpDirection {
    ARP_UP,
    ARP_DOWN,
    ARP_UP_DOWN,
    ARP_DOWN_UP,
    ARP_UP_N_DOWN,
    ARP_DOWN_N_UP,
    ARP_RANDOM,
    ARP_PLAY_ORDER,
    ARP_CONVERGE,
    ARP_DIVERGE,
    ARP_CON_DIVERGE,
    ARP_DIV_CONVERGE,
    ARP_PINKY_UP,
    ARP_PINKY_DOWN,
    ARP_THUMB_UP,
    ARP_THUMB_DOWN,

};

enum ArpDivision {
    DIV_OFF = 0,
    DIV_WHOLE = 1,
    DIV_HALF = 2,
    DIV_THIRD = 3,
    DIV_QUARTER = 4,
    DIV_SIXTH = 6,
    DIV_EIGHTH = 8,
    DIV_TWELFTH = 12,
    DIV_SIXTEENTH = 16,
    DIV_TWENTYFOURTH = 24,
    DIV_THIRTYSECOND = 32,
    DIV_SIXTYFOURTH = 64
};

enum ArpClockSource {
    CLOCK_INTERNAL,
    CLOCK_INTERNAL_CLOCKOUT,  // Send clock to external devices
    CLOCK_EXTERNAL
};

struct ArpeggiatorConfig {
    // Timing
    uint32_t bpm = 120;                    // BPM (60-299)
    ArpClockSource clockSource = CLOCK_INTERNAL;
    uint8_t swingAmount = 50;              // Swing amount (20-80, 50=no swing)

    // Playback
    uint8_t gateTime = 90;                 // Gate time (0% to 100%)
    ArpDirection direction = ARP_UP;       // Direction of arpeggiator
    uint8_t step = 1;                      // Octave step (1 to 8)
    int8_t stepOffset = 0;                 // Step offset (-24 to 24 semitones)
};

struct ArpNote {
    uint8_t note;
    uint8_t velocity;
    uint8_t channel;
    uint32_t timestamp;  // When the note was pressed
};

class Arpeggiator : public MidiEffect {
private:
    ArpeggiatorConfig* config;       // Configuration pointer
    vector<ArpNote> notePool;        // All active notes
    vector<ArpNote> arpSequence;     // Current arp sequence
    uint8_t currentIndex = 0;        // Current position in sequence

    uint64_t lastStepTime = 0;       // Last step time in microseconds
    uint32_t stepDuration[2];        // [0] = on-beat, [1] = off-beat (for swing)
    ArpNote lastPlayedNote = {255, 0, 0, 0}; // Track last played note for note-off

    bool disableOnNextTick = false;

    // Helper functions
    void ProcessNoteOn(const MidiPacket& packet, deque<MidiPacket>& output);
    void ProcessNoteOff(const MidiPacket& packet, deque<MidiPacket>& output);
    void ProcessAfterTouch(const MidiPacket& packet, deque<MidiPacket>& output);
    void UpdateSequence();
    void StepArpeggiator(deque<MidiPacket>& output);
    void CalculateStepDurations();

public:
    ArpDivision division = DIV_OFF;  // Note division (internal control)
    
    Arpeggiator(ArpeggiatorConfig* cfg);

    void Tick(deque<MidiPacket>& input, deque<MidiPacket>& output) override;
    void Reset() override;
    void SetEnabled(bool state) override;

    // Configuration access
    void UpdateConfig(ArpeggiatorConfig* cfg);

    // Division control
    void SetDivision(ArpDivision div);
};