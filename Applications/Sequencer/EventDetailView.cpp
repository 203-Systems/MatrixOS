#include "EventDetailView.h"
#include "Sequence.h"
#include <algorithm>


const Color noteColor = Color(0xFFFF00);
const Color aftertouchColor = Color(0x00FF80);
const Color drumColor = Color(0xFF6000);
const Color ccColor = Color(0x0080FF);
const Color pcColor = Color(0x6000FF);
const Color unknownColor = Color(0xFF0000);

EventDetailView::EventDetailView(Sequencer* sequencer)
{
    this->sequencer = sequencer;
}

bool EventDetailView::IsEnabled() {
    bool enabled = sequencer->currentView == Sequencer::ViewMode::StepDetail;

    if (!enabled && wasEnabled == true)
    {
        sequencer->ClearActiveNotes();
    }

    if(enabled && wasEnabled == false)
    {
        MatrixOS::KeyPad::Clear();
        RebuildEventList();
        lastOnTime = MatrixOS::SYS::Millis();
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

    position = *sequencer->sequence.GetPosition(sequencer->track);
    pattern = sequencer->sequence.GetPattern(sequencer->track, position.clip, position.pattern);
    if (!pattern)
    {
        sequencer->SetView(Sequencer::ViewMode::Sequencer);
        return;
    }
    
    uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
    uint16_t startTime = position.step * pulsesPerStep;
    uint16_t endTime = startTime + pulsesPerStep - 1;

    bool matchedSelection = false;

    auto it = pattern->events.lower_bound(startTime);
    while (it != pattern->events.end() && it->first <= endTime)
    {
        eventRefs.push_back(it);
        if(it == selectedEventIter)
        {
            matchedSelection = true;
        }
        ++it;
    }

    if (eventRefs.empty())
    {
        sequencer->SetView(Sequencer::ViewMode::Sequencer);
    }
    else if(!matchedSelection)
    {
        selectedEventIter = eventRefs.front();
    }
}

bool EventDetailView::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    bool handled = false;

    // Route to appropriate handler based on Y position
    if (xy.y == 0)
    {
        handled = EventSelectorKeyHandler(xy, keyInfo);
    }
    else if (xy.y == 1)
    {
        if (xy.x < 6)
        {
            handled = MicroStepSelectorKeyHandler(xy, keyInfo);
        }
        else if (xy.x == 7)
        {
            handled = DeleteEventKeyHandler(xy, keyInfo);
        }
    }
    else
    {
        auto& event = selectedEventIter->second;
        if (event.eventType == SequenceEventType::NoteEvent)
        {
            handled = NoteConfigKeyHandler(xy - Point(0, 2), keyInfo);
        }
        else if (event.eventType == SequenceEventType::ControlChangeEvent)
        {
            handled = CCConfigKeyHandler(xy, keyInfo);
        }
    }

    if (!handled && keyInfo->State() == HOLD)
    {
        MatrixOS::UIUtility::TextScroll("Event Config", Color::White);
        return true;
    }

    return handled;
}

bool EventDetailView::Render(Point origin)
{
    RenderEventSelector(origin);
    RenderMicroStepSelector(origin + Point(0, 1));

    auto& event = selectedEventIter->second;
    switch(event.eventType)
    {
        case SequenceEventType::NoteEvent:
            RenderNoteConfig(origin + Point(0, 2));
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
                {
                    const SequenceEventNote& noteData = std::get<SequenceEventNote>(event.data);
                    color = noteData.aftertouch ? aftertouchColor : noteColor;
                    break;
                }
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
    uint8_t channel = sequencer->sequence.GetChannel(sequencer->track);

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
                    const SequenceEventNote& noteData = std::get<SequenceEventNote>(event.data);
                    if (noteData.aftertouch)
                    {
                        MatrixOS::UIUtility::TextScroll("Aftertouch Event", aftertouchColor);
                    }
                    else
                    {
                        MatrixOS::UIUtility::TextScroll("Note Event", noteColor);
                    }
                }
            }
        }
        return true;
    }
    return false;
}

void EventDetailView::RenderMicroStepSelector(Point origin)
{
    // Render micro step positions on Y=1 row (only 6 positions)
    uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
    // divided into 6 slots
    uint16_t stepStartTime = position.step * pulsesPerStep;

    for (uint8_t x = 0; x < 6; x++)
    {
        Point point = origin + Point(x, 0);
        Color color = Color(0x404040); // Default: dark gray (empty)

        // Check if selected event is in this micro step slot
        if (selectedEventIter != std::multimap<uint16_t, SequenceEvent>::iterator())
        {
            uint16_t eventTime = selectedEventIter->first;
            uint16_t relativeTime = eventTime - stepStartTime; // Time within the current step

            uint16_t slotSize = pulsesPerStep / 6;
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
    if (keyInfo->State() == PRESSED)
    {
        uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
        uint16_t slotSize = pulsesPerStep / 6;
        uint16_t stepStartTime = position.step * pulsesPerStep;
        uint16_t targetTime = stepStartTime + (xy.x * slotSize);

        auto eventIter = selectedEventIter;
        uint16_t oldTime = eventIter->first;

        if (oldTime != targetTime)
        {
            SequenceEvent eventData = eventIter->second;
            pattern->events.erase(eventIter);
            auto insertedIter = pattern->events.insert({targetTime, eventData});
            sequencer->sequence.SetDirty();
            selectedEventIter = insertedIter;
            RebuildEventList();
        }
        return true;
    }
    else if (keyInfo->State() == HOLD)
    {
        MatrixOS::UIUtility::TextScroll("Microstep " + std::to_string(xy.x), Color::White);
        return true;
    }

    return false;
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
        uint8_t channel = sequencer->sequence.GetChannel(sequencer->track);

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

        pattern->events.erase(eventIter);
        sequencer->sequence.SetDirty();

        RebuildEventList();

        return true;
    }

    return false;
}

void EventDetailView::RenderNoteConfig(Point origin)
{
    SequenceEventNote& noteData = std::get<SequenceEventNote>(selectedEventIter->second.data);
    Color modeColor = noteData.aftertouch ? aftertouchColor : noteColor;
    MatrixOS::LED::SetColor(origin + Point(7, 0), modeColor);

    RenderLengthSelector(origin + Point(0, 1));
    RenderVelocitySelector(origin + Point(0, 3));
}

bool EventDetailView::NoteConfigKeyHandler(Point xy, KeyInfo* keyInfo)
{
    SequenceEventNote& noteData = std::get<SequenceEventNote>(selectedEventIter->second.data);

    if (xy == Point(7, 0))
    {
        if (keyInfo->State() == HOLD)
        {
            Color modeColor = noteData.aftertouch ? aftertouchColor : noteColor;
            MatrixOS::UIUtility::TextScroll(noteData.aftertouch ? "Aftertouch Event" : "Note On Event", modeColor);
            return true;
        }
        else if (keyInfo->State() == RELEASED)
        {
            noteData.aftertouch = !noteData.aftertouch;
            sequencer->sequence.SetDirty();
            return true;
        }
    }
    else if (xy.y >= 1 && xy.y < 3)
    {
        return LengthSelectorKeyHandler(Point(xy.x, xy.y - 1), keyInfo);
    }
    else if (xy.y >= 3 && xy.y < 5)
    {
        return VelocitySelectorKeyHandler(Point(xy.x, xy.y - 3), keyInfo);
    }

    return false;
}

void EventDetailView::RenderLengthSelector(Point origin)
{
    const SequenceEventNote& noteData = std::get<SequenceEventNote>(selectedEventIter->second.data);
    uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
    uint8_t fullSlots = noteData.length / pulsesPerStep;
    uint16_t partial = noteData.length % pulsesPerStep;

    for (uint8_t idx = 0; idx < 16; ++idx)
    {
        Point point = origin + Point(idx % 8, idx / 8);
        Color color = Color(0x202020);

        if (idx < fullSlots)
        {
            color = Color(0xA000FF);
        }
        else if (idx == fullSlots && partial > 0)
        {
            uint16_t scale = static_cast<uint16_t>(partial) * 192 / pulsesPerStep; // Add negative offset (255 to 192) to make small difference more obvious
            color = Color(0xA000FF).Dim(std::min<uint16_t>(255, scale));
        }

        MatrixOS::LED::SetColor(point, color);
    }
}

bool EventDetailView::LengthSelectorKeyHandler(Point xy, KeyInfo* keyInfo)
{
    SequenceEventNote& noteData = std::get<SequenceEventNote>(selectedEventIter->second.data);
    uint8_t lengthIdx = xy.x + xy.y * 8;
    uint8_t lengthPulses = std::clamp<uint8_t>(lengthIdx + 1, 1, 16);

    if (keyInfo->State() == PRESSED)
    {
        uint16_t currentLength = noteData.length;
        uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
        uint16_t slotStart = lengthIdx * pulsesPerStep;
        uint16_t slotEnd = slotStart + pulsesPerStep;

        if (currentLength > slotStart && currentLength <= slotEnd)
        {
            static const uint16_t fractionTicks[] = {
                static_cast<uint16_t>(pulsesPerStep * 3 / 4), // 75%
                static_cast<uint16_t>(pulsesPerStep / 2),     // 50%
                static_cast<uint16_t>(pulsesPerStep / 4)      // 25%
            };

            uint16_t coverage = currentLength - slotStart;
            uint16_t nextCoverage = pulsesPerStep; // default back to 100%

            for (uint16_t fraction : fractionTicks)
            {
                if (coverage > fraction)
                {
                    nextCoverage = fraction;
                    break;
                }
            }

            noteData.length = slotStart + nextCoverage;
        }
        else
        {
            noteData.length = std::min<uint16_t>(16, lengthPulses) * pulsesPerStep;
        }

        sequencer->sequence.SetDirty();
        return true;
    }
    else if (keyInfo->State() == HOLD)
    {
        MatrixOS::UIUtility::TextScroll("Length " + std::to_string(lengthPulses), Color(0xA000FF));
        return true;
    }

    return false;
}

void EventDetailView::RenderVelocitySelector(Point origin)
{
    const SequenceEventNote& noteData = std::get<SequenceEventNote>(selectedEventIter->second.data);
    uint8_t currentVelocity = noteData.velocity;

    for (uint8_t idx = 0; idx < 16; ++idx)
    {
        uint8_t x = idx % 8;
        uint8_t y = idx / 8;
        uint8_t slotMin = idx * 8 + 1;
        uint8_t slotMax = idx == 15 ? 127 : (slotMin + 7);
        Color color = Color(0x202020);

        if (currentVelocity > slotMin && currentVelocity < slotMax)
        {
            uint16_t fraction = (currentVelocity - slotMin) * 192 / 8;
            color = (noteData.aftertouch ? aftertouchColor : noteColor).Dim(std::min<uint16_t>(255, fraction));
        }
        else if (currentVelocity >= slotMax)
        {
            color = noteData.aftertouch ? aftertouchColor : noteColor;
        }

        MatrixOS::LED::SetColor(origin + Point(x, y), color);
    }
}

bool EventDetailView::VelocitySelectorKeyHandler(Point xy, KeyInfo* keyInfo)
{
    SequenceEventNote& noteData = std::get<SequenceEventNote>(selectedEventIter->second.data);
    uint8_t velocityIdx = xy.x + xy.y * 8;
    uint8_t slotMin = velocityIdx * 8 + 1;
    uint8_t slotMax = velocityIdx == 15 ? 127 : slotMin + 7;

    if (keyInfo->State() == PRESSED)
    {
        uint8_t currentVelocity = noteData.velocity;
        if (currentVelocity >= slotMin && currentVelocity <= slotMax)
        {
            static const uint8_t fractionSteps[] = { 5, 3, 1, 7 }; // ~75%, 50%, 25%, 100%
            uint8_t coverage = currentVelocity - slotMin; // 0-7
            uint8_t nextCoverage = 7; // default to full

            for (uint8_t fraction : fractionSteps)
            {
                if (coverage > fraction)
                {
                    nextCoverage = fraction;
                    break;
                }
            }

            noteData.velocity = std::clamp<uint8_t>(slotMin + nextCoverage, 1, 127);
        }
        else
        {
            noteData.velocity = std::clamp<uint8_t>(slotMin + 7, 1, 127);
        }

        sequencer->sequence.SetDirty();
        return true;
    }
    else if (keyInfo->State() == HOLD)
    {
        Color velocityColor = noteData.aftertouch ? aftertouchColor : noteColor;
        MatrixOS::UIUtility::TextScroll("Velocity " + std::to_string(noteData.velocity), velocityColor);
        return true;
    }

    return false;
}

void EventDetailView::RenderCCConfig(Point origin)
{
}

bool EventDetailView::CCConfigKeyHandler(Point xy, KeyInfo* keyInfo)
{
    return true;
}
