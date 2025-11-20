#pragma once

#include "MatrixOS.h"
#include "SequenceEvent.h"

#define SEQUENCE_VERSION 1

struct SequencePattern {
    uint8_t quarterNotes = 16;
    std::map<uint16_t, SequenceEvent> events;

    void Clear()
    {
        events.clear();
    }
};

#define SEQUENCE_MAX_PATTERN_COUNT 8

struct SequenceTrack {
    uint8_t channel = 0;
    vector<SequencePattern> patterns;
};

struct SequenceData {
    uint8_t version = SEQUENCE_VERSION;
    uint16_t bpm = 120;
    uint8_t swing = 50;
    uint32_t solo = 0;
    uint32_t mute = 0;
    uint32_t record = 0xFFFFFFFF;
    vector<SequenceTrack> tracks;
};
