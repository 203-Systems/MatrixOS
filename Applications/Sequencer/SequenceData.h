#pragma once

#include "MatrixOS.h"
#include "SequenceEvent.h"

#define SEQUENCE_VERSION 1

struct SequencePattern {
    uint8_t quarterNotes = 16;
    std::multimap<uint16_t, SequenceEvent> events;

    void Clear()
    {
        events.clear();
    }

    void AddEvent(uint16_t timestamp, const SequenceEvent& event)
    {
        events.insert({timestamp, event});
    }

    bool HasEventInRange(uint16_t startTime, uint16_t endTime, SequenceEventType eventType = SequenceEventType::Invalid)
    {
        auto it = events.lower_bound(startTime);
        while (it != events.end() && it->first <= endTime)
        {
            if (eventType == SequenceEventType::Invalid || it->second.eventType == eventType)
            {
                return true;
            }
            ++it;
        }
        return false;
    }

    bool RemoveNoteEventsInRange(uint16_t startTime, uint16_t endTime, uint8_t note)
    {
        bool removed = false;
        auto it = events.lower_bound(startTime);
        while (it != events.end() && it->first <= endTime)
        {
            if (it->second.eventType == SequenceEventType::NoteEvent)
            {
                const SequenceEventNote& noteData = std::get<SequenceEventNote>(it->second.data);
                if (noteData.note == note)
                {
                    it = events.erase(it);
                    removed = true;
                    continue;
                }
            }
            ++it;
        }
        return removed;
    }

    bool RemoveAllEventsInRange(uint16_t startTime, uint16_t endTime)
    {
        bool removed = false;
        auto it = events.lower_bound(startTime);
        while (it != events.end() && it->first <= endTime)
        {
            it = events.erase(it);
            removed = true;
        }
        return removed;
    }

    void CopyEventsInRange(uint16_t sourceStart, uint16_t destStart, uint16_t length)
    {
        vector<std::pair<uint16_t, SequenceEvent>> eventsToCopy;

        uint16_t sourceEnd = sourceStart + length - 1;

        // Collect events in source range
        auto it = events.lower_bound(sourceStart);
        while (it != events.end() && it->first <= sourceEnd)
        {
            uint16_t offset = it->first - sourceStart;
            uint16_t newTimestamp = destStart + offset;
            eventsToCopy.push_back({newTimestamp, it->second});
            ++it;
        }

        // Add copied events to destination
        for (const auto& [timestamp, event] : eventsToCopy)
        {
            events.insert({timestamp, event});
        }
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
