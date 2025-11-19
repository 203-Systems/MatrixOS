#pragma once

#include "MatrixOS.h"
#include "SequenceEvent.h"

#define SEQUENCE_VERSION 1

struct SequencePattern {
    uint8_t quarterNotes;
    std::map<uint16_t, SequenceEvent> events;

    void Clear()
    {
        events.clear();
    }
};

struct SequenceTrack {
    uint8_t channel;
    vector<SequencePattern> patterns;
};  

struct SequenceData {
    uint8_t version;
    uint16_t bpm;
    uint8_t swing;  
    uint32_t solo;
    uint32_t mute;
    uint32_t record;
    vector<SequenceTrack> tracks;
};
