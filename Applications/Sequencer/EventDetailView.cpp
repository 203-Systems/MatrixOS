#include "EventDetailView.h"
#include "Sequence.h"

EventDetailView::EventDetailView(Sequencer* sequencer)
{
    this->sequencer = sequencer;
}

bool EventDetailView::IsEnabled() {
    bool enabled = this->enabled;
    if (enableFunc) {
        enabled = (*enableFunc)();
    }

    if (!enabled && wasEnabled == true)
    {
        sequencer->ClearActiveNotes();
    }

    if(enabled && wasEnabled == false)
    {
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

    bool matchedSelection = false;

    auto it = pattern.events.lower_bound(startTime);
    while (it != pattern.events.end() && it->first <= endTime)
    {
        eventRefs.push_back(it);
        if(it == selectedEventIter)
        {
            matchedSelection = true;
        }
        ++it;
    }

    if(!matchedSelection)
    {
        selectedEventIter = eventRefs.front();
    }
}

bool EventDetailView::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    // Route to appropriate handler based on Y position
    if (xy.y == 0)
    {
        return EventSelectorKeyHandler(xy, keyInfo);
    }
    else if (xy.y == 1)
    {
        if (xy.x < 6)
        {
            return MicroStepSelectorKeyHandler(xy, keyInfo);
        }
        else if (xy.x == 7)
        {
            return DeleteEventKeyHandler(xy, keyInfo);
        }
    }

    auto& event = selectedEventIter->second;
    switch (event.eventType)
    {
        case SequenceEventType::NoteEvent:
            return NoteConfigKeyHandler(xy, keyInfo);
        case SequenceEventType::ControlChangeEvent:
            return CCConfigKeyHandler(xy, keyInfo);
        default:
            break;
    }

    return true;
}

bool EventDetailView::Render(Point origin)
{
    RenderEventSelector(origin);
    RenderMicroStepSelector(origin + Point(0, 1));

    auto& event = selectedEventIter->second;
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
            if (selectedEventIter == eventRefs[x])
            {
                color = Color::Crossfade(color, Color::White, Fract16(0x9000));
            }

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
            selectedEventIter = eventRefs[xy.x];

            // If it's a note event, send note on
            auto& event = eventRefs[xy.x]->second;
            if (event.eventType == SequenceEventType::NoteEvent)
            {
                const SequenceEventNote& noteData = std::get<SequenceEventNote>(event.data);
                sequencer->noteActive.insert(noteData.note);
                MatrixOS::MIDI::Send(MidiPacket::NoteOn(channel, noteData.note, noteData.velocity));
            }

            return true;
        }
    }
    else if (keyInfo->State() == RELEASED || keyInfo->State() == HOLD)
    {
        if (xy.x < eventRefs.size())
        {
            auto& event = eventRefs[xy.x]->second;
            if (event.eventType == SequenceEventType::NoteEvent)
            {
                const SequenceEventNote& noteData = std::get<SequenceEventNote>(event.data);
                auto noteIt = sequencer->noteActive.find(noteData.note);
                if (noteIt != sequencer->noteActive.end())
                {
                    sequencer->noteActive.erase(noteIt);
                    MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, noteData.note));
                }

                if (keyInfo->State() == HOLD)
                {
                    MatrixOS::UIUtility::TextScroll("Note Event", Color(0xFFFF00));
                }
            }
        }
        return true;
    }
    return false;
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
        if (selectedEventIter != std::multimap<uint16_t, SequenceEvent>::iterator())
        {
            uint16_t eventTime = selectedEventIter->first;
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

    // Delete button at X=7 (global), render bright red (or white when pressed)
    Point deletePoint = origin + Point(7, 0);
    bool deleteActive = MatrixOS::KeyPad::GetKey(deletePoint)->Active();
    Color deleteColor = deleteActive ? Color::White : Color::Red;
    MatrixOS::LED::SetColor(deletePoint, deleteColor);
}

bool EventDetailView::MicroStepSelectorKeyHandler(Point xy, KeyInfo* keyInfo)
{
    uint16_t slotSize = Sequence::PPQN / 6; // 96 / 6 = 16
    uint8_t step = sequencer->sequence.GetPosition(sequencer->track).quarterNote;
    uint16_t stepStartTime = step * Sequence::PPQN;
    uint16_t targetTime = stepStartTime + (xy.x * slotSize);

    if (keyInfo->State() == PRESSED)
    {
        uint8_t track = sequencer->track;
        uint8_t clip = sequencer->sequence.GetPosition(track).clip;
        uint8_t patternIdx = sequencer->sequence.GetPosition(track).pattern;
        SequencePattern& pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);

        auto eventIter = selectedEventIter;
        uint16_t oldTime = eventIter->first;

        if (oldTime != targetTime)
        {
            // Copy the event data
            SequenceEvent eventData = eventIter->second;

            // Remove old event
            pattern.events.erase(eventIter);

            // Insert event at new time
            auto insertedIter = pattern.events.insert({targetTime, eventData});
            selectedEventIter = insertedIter;

            // Rebuild event list to update iterators
            RebuildEventList();
        }
    }
    else if (keyInfo->State() == HOLD)
    {
        MatrixOS::UIUtility::TextScroll("Microstep " + std::to_string(xy.x + 1), Color::White);
    }

    return true;
}

bool EventDetailView::DeleteEventKeyHandler(Point xy, KeyInfo* keyInfo)
{
    if (keyInfo->State() == HOLD)
    {
        MatrixOS::UIUtility::TextScroll("Delete Event", Color(0xFF0000));
        return true;
    }
    else if (keyInfo->State() == RELEASED)
    {
        uint8_t track = sequencer->track;
        uint8_t clip = sequencer->sequence.GetPosition(track).clip;
        uint8_t patternIdx = sequencer->sequence.GetPosition(track).pattern;
        uint8_t channel = sequencer->sequence.GetChannel(track);
        SequencePattern& pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);

        auto eventIter = selectedEventIter;
        SequenceEvent eventData = eventIter->second;

        if (eventData.eventType == SequenceEventType::NoteEvent)
        {
            const SequenceEventNote& noteData = std::get<SequenceEventNote>(eventData.data);
            auto noteIt = sequencer->noteActive.find(noteData.note);
            if (noteIt != sequencer->noteActive.end())
            {
                sequencer->noteActive.erase(noteIt);
                MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, noteData.note));
            }
        }

        pattern.events.erase(eventIter);

        RebuildEventList();
        selectedEventIter = eventRefs.front();

        return true;
    }

    return false;
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
