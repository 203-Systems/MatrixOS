#pragma once

#include "MatrixOS.h"
#include "SequenceEvent.h"
#include <vector>
#include <cstdint>
#include <unordered_map>

#define SEQUENCE_VERSION 1

struct SequencePattern {
    uint8_t steps = 16;
    std::multimap<uint16_t, SequenceEvent> events;

    void Clear();
    void AddEvent(uint16_t timestamp, const SequenceEvent& event);
    bool HasEventInRange(uint16_t startTime, uint16_t endTime, SequenceEventType eventType = SequenceEventType::Invalid);
    bool RemoveNoteEventsInRange(uint16_t startTime, uint16_t endTime, uint8_t note);
    bool RemoveAllEventsInRange(uint16_t startTime, uint16_t endTime);
    void CopyEventsInRange(uint16_t sourceStart, uint16_t destStart, uint16_t length);
    void ClearStepEvents(uint8_t step, uint16_t pulsesPerStep);
    void CopyStepEvents(uint8_t src, uint8_t dest, uint16_t pulsesPerStep);
};

#define SEQUENCE_MAX_PATTERN_COUNT 16


struct SequenceClip {
    bool enabled = true;
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
    uint8_t barLength = 16;
    uint32_t solo = 0;
    uint32_t mute = 0;
    uint32_t record = 0xFFFFFFFF;
    vector<SequenceTrack> tracks;
};

// CBOR serialization helpers
bool SerializeSequenceData(const SequenceData& data, std::vector<uint8_t>& out);
bool DeserializeSequenceData(const uint8_t* in, size_t len, SequenceData& out);
