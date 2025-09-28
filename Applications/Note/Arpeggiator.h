#pragma once

#include "MatrixOS.h"
#include "MidiEffect.h"
#include <vector>
#include <deque>
#include <map>

using std::vector;
using std::deque;
using std::map;

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
    ARP_PINKY_UP_DOWN,
    ARP_THUMB_UP,
    ARP_THUMB_UP_DOWN,
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

inline const char* arpDirectionNames[16] = {
    "Up", 
    "Down", 
    "Up Down", 
    "Down Up", 
    "Up & Down", 
    "Down & Up",
    "Random", 
    "Play Order", 
    "Converge", 
    "Diverge", 
    "Con & Diverge", 
    "Div & Converge",
    "Pinky Up", 
    "Pinky Up Down", 
    "Thumb Up", 
    "Thumb Up Down"
};

enum ArpClockSource {
    CLOCK_INTERNAL,
    CLOCK_INTERNAL_CLOCKOUT,    // Send clock to external devices
    CLOCK_EXTERNAL
};

struct ArpeggiatorConfig {
    // Timing
    uint32_t bpm = 120;                    // BPM (20-299)
    ArpClockSource clockSource = CLOCK_INTERNAL;
    uint8_t swing = 50;              // Swing amount (20-80, 50=no swing)

    // Playback
    uint8_t gateTime = 50;                 // Gate time (0% to 200%, 0=always on)
    ArpDirection direction = ARP_UP;       // Direction of arpeggiator
    uint8_t step = 1;                      // Step (1 to 16)
    int8_t stepOffset = 12;                // Step offset (-48 to 48 semitones)
    uint8_t repeat = 0;               // Repeat the the sequence # times before stopping (0 to 100) (0 will be inf)
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
    struct GateOffEvent {
        uint64_t gateOffTime;
        uint8_t note;
        uint8_t channel;
    };
    deque<GateOffEvent> gateOffQueue; // Chronologically ordered queue of gate-off events

    bool disableOnNextTick = false;

    // Repeat tracking
    uint8_t currentRepeat = 0;        // Current repeat count
    uint8_t lastSequenceIndex = 0;    // Track last index to detect sequence completion

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
    void UpdateConfig(ArpeggiatorConfig* cfg = nullptr);

    // Division control
    void SetDivision(ArpDivision div);
};