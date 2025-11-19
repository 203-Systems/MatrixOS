#pragma once

#include <variant>

#include "MatrixOS.h"

// Forward declaration
struct SequenceData;

enum SequenceEventType {
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

    // Static factory methods
    static SequenceEvent Note(const uint8_t note, const uint8_t velocity, const bool aftertouch) {
        return {SequenceEventType::NoteEvent, SequenceEventNote{aftertouch, note, velocity}};
    }

    static SequenceEvent ControlChange(const uint8_t param, const uint8_t value) {
        return {SequenceEventType::ControlChangeEvent, SequenceEventCC{param, value}};
    }

    // static SequenceEvent ProgramChange(const uint8_t program) {
    //     return {SequenceEventType::ProgramControl, SequenceEventPC{program}};
    // }

    // static SequenceEvent BPMChange(const uint16_t bpm) {
    //     return {SequenceEventType::BPMChange, SequenceEventBPMChange{bpm}};
    // }

    // static SequenceEvent SwingChange(const uint8_t swing) {
    //     return {SequenceEventType::SwingChange, SequenceEventSwingChange{swing}};
    // }

    // Declaration only - defined in SequenceEvent.cpp
    void ExecuteEvent(SequenceData& sequenceData, uint8_t track);
};
