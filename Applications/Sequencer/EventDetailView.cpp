#include "EventDetailView.h"
#include "Sequence.h"

EventDetailView::EventDetailView(Sequencer* sequencer)
{
    this->sequencer = sequencer;
    this->eventIndex = 0;
}

bool EventDetailView::IsEnabled() {
    bool enabled = this->enabled;
    if (enableFunc) {
        enabled = (*enableFunc)();
    }

    if(enabled && wasEnabled == false)
    {
        MLOGD("EventDetailView", "View became enabled, rebuilding event list");
        eventIndex = 0;
        RebuildEventList();
    }

    wasEnabled = enabled;
    return enabled;
}


Dimension EventDetailView::GetSize()
{
    return Dimension(8, 7);
}

void EventDetailView::RebuildEventList()
{
    eventRefs.clear();

    uint8_t track = sequencer->track;
    uint8_t clip = sequencer->sequence.GetPosition(track).clip;
    uint8_t patternIdx = sequencer->sequence.GetPosition(track).pattern;
    SequencePattern& pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);

    uint8_t step = sequencer->sequence.GetPosition(track).quarterNote;
    uint16_t startTime = step * Sequence::PPQN;
    uint16_t endTime = startTime + Sequence::PPQN - 1;

    MLOGD("EventDetailView", "RebuildEventList: track=%d, clip=%d, pattern=%d, step=%d", track, clip, patternIdx, step);
    MLOGD("EventDetailView", "Time range: %d - %d", startTime, endTime);

    auto it = pattern.events.lower_bound(startTime);
    while (it != pattern.events.end() && it->first <= endTime)
    {
        eventRefs.push_back(it);
        MLOGD("EventDetailView", "Found event at time %d, type %d", it->first, (int)it->second.eventType);
        ++it;
    }

    MLOGD("EventDetailView", "Total events found: %d, eventIndex: %d", eventRefs.size(), eventIndex);

    // Validate eventIndex is within bounds
    if (eventIndex >= eventRefs.size() && eventRefs.size() > 0)
    {
        eventIndex = eventRefs.size() - 1;
    }
    else if (eventRefs.size() == 0)
    {
        eventIndex = 0;
    }
}

bool EventDetailView::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    // Route to appropriate handler based on Y position
    if (xy.y == 0)
    {
        return EventSelectorKeyHandler(xy, keyInfo);
    }
    else if (xy.y == 1 && xy.x < 6)
    {
        return MicroStepSelectorKeyHandler(xy, keyInfo);
    }

    // Route to event type specific handlers for Y >= 2
    if (eventIndex < eventRefs.size())
    {
        auto& event = eventRefs[eventIndex]->second;
        switch(event.eventType)
        {
            case SequenceEventType::NoteEvent:
                return NoteConfigKeyHandler(xy, keyInfo);
            case SequenceEventType::ControlChangeEvent:
                return CCConfigKeyHandler(xy, keyInfo);
            default:
                break;
        }
    }

    return true;
}

bool EventDetailView::Render(Point origin)
{
    MLOGD("EventDetailView", "Render called, origin=(%d,%d), eventRefs.size=%d", origin.x, origin.y, eventRefs.size());

    RenderEventSelector(origin);
    RenderMicroStepSelector(origin + Point(0, 1));

    // Render event type specific content for Y >= 2
    if (eventIndex < eventRefs.size())
    {
        auto& event = eventRefs[eventIndex]->second;
        switch(event.eventType)
        {
            case SequenceEventType::NoteEvent:
                RenderNoteConfig(origin);
                break;
            case SequenceEventType::ControlChangeEvent:
                RenderCCConfig(origin);
                break;
            default:
                break;
        }
    }

    return true;
}

void EventDetailView::RenderEventSelector(Point origin)
{
    uint8_t track = sequencer->track;
    Color trackColor = sequencer->meta.tracks[track].color;

    const Color noteColor = Color(0xFFFF00);
    const Color drumColor = Color(0xFF8000);
    const Color ccColor = Color(0x0080FF);
    const Color pcColor = Color(0x00FF80);
    const Color unknownColor = Color(0xFF0000);

    // Render events in row Y=0, horizontally from X=0 to X=7
    for (uint8_t x = 0; x < 8; x++)
    {
        Point point = origin + Point(x, 0);

        if (x < eventRefs.size())
        {
            // Determine color based on event type
            Color color;
            auto& event = eventRefs[x]->second;

            switch (event.eventType)
            {
                case SequenceEventType::NoteEvent:
                    color = noteColor;
                    break;
                case SequenceEventType::ControlChangeEvent:
                    color = ccColor;
                    break;
                default:
                    color = unknownColor;
                    break;
            }

            // Highlight selected event
            if (x == eventIndex)
            {
                color = Color::Crossfade(color, Color::White, Fract16(0x9000));
            }

            MLOGD("EventDetailView", "Rendering event at X=%d, point=(%d,%d), color=0x%06X", x, point.x, point.y, color.RGB());
            MatrixOS::LED::SetColor(point, color);
        }
        else
        {
            // Empty slot
            MatrixOS::LED::SetColor(point, Color::Black);
        }
    }
}

bool EventDetailView::EventSelectorKeyHandler(Point xy, KeyInfo* keyInfo)
{
    uint8_t track = sequencer->track;
    uint8_t channel = sequencer->sequence.GetChannel(track);

    if (keyInfo->State() == PRESSED)
    {
        // Clicking on row Y=0 selects an event
        if (xy.x < eventRefs.size())
        {
            eventIndex = xy.x;

            // If it's a note event, send note on
            auto& event = eventRefs[eventIndex]->second;
            if (event.eventType == SequenceEventType::NoteEvent)
            {
                const SequenceEventNote& noteData = std::get<SequenceEventNote>(event.data);
                sequencer->noteActive.insert(noteData.note);
                MatrixOS::MIDI::Send(MidiPacket::NoteOn(channel, noteData.note, noteData.velocity));
            }

            return true;
        }
    }
    else if (keyInfo->State() == RELEASED)
    {
        // Send note off for the selected event if it's a note event
        if (eventIndex < eventRefs.size())
        {
            auto& event = eventRefs[eventIndex]->second;
            if (event.eventType == SequenceEventType::NoteEvent)
            {
                const SequenceEventNote& noteData = std::get<SequenceEventNote>(event.data);
                auto noteIt = sequencer->noteActive.find(noteData.note);
                if (noteIt != sequencer->noteActive.end())
                {
                    sequencer->noteActive.erase(noteIt);
                    MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, noteData.note));
                }
            }
        }
    }
    return true;
}

void EventDetailView::RenderMicroStepSelector(Point origin)
{
    uint8_t track = sequencer->track;

    // Render micro step positions on Y=1 row (only 6 positions)
    // PPQN = 96, divided into 6 slots = 16 ticks per slot
    uint8_t step = sequencer->sequence.GetPosition(track).quarterNote;
    uint16_t stepStartTime = step * Sequence::PPQN;

    for (uint8_t x = 0; x < 6; x++)
    {
        Point point = origin + Point(x, 0);
        Color color = Color(0x404040); // Default: dark gray (empty)

        // Check if selected event is in this micro step slot
        if (eventIndex < eventRefs.size())
        {
            auto& eventIter = eventRefs[eventIndex];
            uint16_t eventTime = eventIter->first;
            uint16_t relativeTime = eventTime - stepStartTime; // Time within the current step

            uint16_t slotSize = Sequence::PPQN / 6; // 96 / 6 = 16
            uint8_t eventSlot = relativeTime / slotSize;
            uint16_t expectedTime = eventSlot * slotSize;

            if (x == eventSlot)
            {
                // Check if event is perfectly aligned to the grid
                if (relativeTime == expectedTime)
                {
                    color = Color(0xFFFFFF); // White: perfectly aligned
                }
                else
                {
                    color = Color(0xFF00FF); // Magenta: not perfectly aligned
                }
            }
        }

        MatrixOS::LED::SetColor(point, color);
    }
}

bool EventDetailView::MicroStepSelectorKeyHandler(Point xy, KeyInfo* keyInfo)
{
    if (keyInfo->State() == PRESSED)
    {
        if (eventIndex < eventRefs.size())
        {
            uint8_t track = sequencer->track;
            uint8_t clip = sequencer->sequence.GetPosition(track).clip;
            uint8_t patternIdx = sequencer->sequence.GetPosition(track).pattern;
            SequencePattern& pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);

            auto& eventIter = eventRefs[eventIndex];
            uint16_t oldTime = eventIter->first;

            uint8_t step = sequencer->sequence.GetPosition(track).quarterNote;
            uint16_t stepStartTime = step * Sequence::PPQN;

            uint16_t slotSize = Sequence::PPQN / 6; // 96 / 6 = 16
            uint16_t newTime = stepStartTime + (xy.x * slotSize);

            // Check if time actually changed
            if (oldTime != newTime)
            {
                // Copy the event data
                SequenceEvent eventData = eventIter->second;

                // Remove old event
                pattern.events.erase(eventIter);

                // Insert event at new time
                pattern.events.insert({newTime, eventData});

                // Rebuild event list to update iterators
                RebuildEventList();
            }
        }
    }

    return true;
}

void EventDetailView::RenderNoteConfig(Point origin)
{
}

bool EventDetailView::NoteConfigKeyHandler(Point xy, KeyInfo* keyInfo)
{
    return false;
}

void EventDetailView::RenderCCConfig(Point origin)
{
}

bool EventDetailView::CCConfigKeyHandler(Point xy, KeyInfo* keyInfo)
{
    return true;
}
