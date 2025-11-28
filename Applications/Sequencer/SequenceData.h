#pragma once

#include "MatrixOS.h"
#include "SequenceEvent.h"
#include <vector>
#include <cstdint>
#include <unordered_map>

#define SEQUENCE_VERSION 3
#define MIN_SUPPORTED_SEQUENCE_VERSION 3

struct SequencePattern {
    uint8_t steps = 16;
    std::multimap<uint16_t, SequenceEvent> events;

    void Clear();
    void ClearStepEvents(uint8_t step, uint16_t pulsesPerStep);
};

#define SEQUENCE_MAX_PATTERN_COUNT 16


struct SequenceClip {
    vector<SequencePattern> patterns;
};

struct SequenceTrack {
    uint8_t channel = 0;
    uint8_t activeClip = 0;
    std::unordered_map<uint8_t, SequenceClip> clips;
};

struct SequenceData {
    uint8_t version = SEQUENCE_VERSION;
    uint16_t bpm = 120;
    uint8_t swing = 50;
    uint8_t beatsPerBar = 4;  // Time signature top number
    uint8_t beatUnit = 4;   // Time signature bottom number
    uint8_t stepDivision = 16;
    uint8_t patternLength = 16;
    uint32_t solo = 0;
    uint32_t mute = 0;
    uint32_t record = 0xFFFFFFFF;
    vector<SequenceTrack> tracks;
};

// Stream-based encoding (CBOR sequence) using File.
bool SerializeSequenceData(const SequenceData& data, File& file);
bool DeserializeSequenceData(File& file, SequenceData& out);
