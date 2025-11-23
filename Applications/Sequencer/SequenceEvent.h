#pragma once

#include <variant>

#include "MatrixOS.h"

// Forward declaration
struct SequenceData;

enum class SequenceEventType {
    Invalid = 0x00,
    NoteEvent = 0x10,
    ControlChangeEvent = 0x20,
    // ProgramControlEvent = 0x30,
    // PitchBendEvent = 0x40,
    // BPMChangeEvent = 0x80,
    // SwingChangeEvent = 0x81,
};

// Specific event data structures
struct SequenceEventNote {
    bool aftertouch:1;
    uint8_t note:7;
    uint8_t velocity;
    uint16_t length;
};

struct SequenceEventCC {
    uint8_t param;
    uint8_t value;
};

// struct SequenceEventPC {
//     uint8_t program;
// };

// // Add more event types as needed
// struct SequenceEventBPMChange {
//     uint16_t bpm;
// };

// struct SequenceEventSwingChange {
//     uint8_t swing;
// };

// Variant containing all possible event types
using SequenceEventData = std::variant<
    SequenceEventNote,
    SequenceEventCC
>;

// Main event structure
struct SequenceEvent {
    SequenceEventType eventType;
    SequenceEventData data;

    // constructor for factory methods
    SequenceEvent(SequenceEventType type, const SequenceEventData& eventData)
        : eventType(type), data(eventData) {}

    // Static factory methods - defined in SequenceEvent.cpp
    static SequenceEvent Note(const uint8_t note, const uint8_t velocity, const bool aftertouch, const uint16_t length = UINT16_MAX /*UINT16_MAX = auto-set to default step length*/);
    static SequenceEvent ControlChange(const uint8_t param, const uint8_t value);
    // static SequenceEvent ProgramChange(const uint8_t program);
    // static SequenceEvent BPMChange(const uint16_t bpm);
    // static SequenceEvent SwingChange(const uint8_t swing);

    // Defined in SequenceEvent.cpp
    void ExecuteEvent(SequenceData& sequenceData, uint8_t track);
};
