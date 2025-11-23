#include "SequenceData.h"

void SequencePattern::Clear()
{
    events.clear();
}

void SequencePattern::AddEvent(uint16_t timestamp, const SequenceEvent& event)
{
    events.insert({timestamp, event});
}

bool SequencePattern::HasEventInRange(uint16_t startTime, uint16_t endTime, SequenceEventType eventType)
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

bool SequencePattern::RemoveNoteEventsInRange(uint16_t startTime, uint16_t endTime, uint8_t note)
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

bool SequencePattern::RemoveAllEventsInRange(uint16_t startTime, uint16_t endTime)
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

void SequencePattern::CopyEventsInRange(uint16_t sourceStart, uint16_t destStart, uint16_t length)
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

void SequencePattern::ClearStepEvents(uint8_t step, uint16_t pulsesPerStep)
{
    uint16_t startTime = step * pulsesPerStep;
    uint16_t endTime = startTime + pulsesPerStep - 1;
    RemoveAllEventsInRange(startTime, endTime);
}

void SequencePattern::CopyStepEvents(uint8_t src, uint8_t dest, uint16_t pulsesPerStep)
{
    uint16_t sourceStartTime = src * pulsesPerStep;
    uint16_t destStartTime = dest * pulsesPerStep;
    CopyEventsInRange(sourceStartTime, destStartTime, pulsesPerStep);
}
