#include "PatternPad.h"

PatternPad::PatternPad(Sequencer* sequencer, vector<uint8_t>* stepSelected, std::unordered_map<uint8_t, uint8_t>* noteSelected, std::unordered_multiset<uint8_t>* noteActive)
{
    this->sequencer = sequencer;
    this->stepSelected = stepSelected;
    this->noteSelected = noteSelected;
    this->noteActive = noteActive;
    width = sequencer->sequence.GetTrackCount();
}

Dimension PatternPad::GetSize() { return Dimension(8, 2); }

bool PatternPad::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    if(keyInfo->state != PRESSED && keyInfo->state != RELEASED)
    {
        return true;
    }

    uint8_t step = xy.x + xy.y * width;
    uint8_t track = sequencer->track;
    uint8_t clip = sequencer->sequence.GetPosition(track).clip;
    uint8_t patternIdx = sequencer->sequence.GetPosition(track).pattern;
    uint8_t channel = sequencer->sequence.GetChannel(track);
    SequencePattern& pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);

    uint16_t startTime = step * Sequence::PPQN;
    uint16_t endTime = startTime + Sequence::PPQN - 1;

    if(keyInfo->state == PRESSED)
    {
        bool HasEvent = pattern.HasEventInRange(startTime, endTime);

        // Check for shift-click to enter StepDetail view (only when not playing locally)
        if(HasEvent && sequencer->ShiftActive() && !sequencer->sequence.Playing(track))
        {
            sequencer->ShiftEventOccured();
            sequencer->sequence.SetPosition(track, clip, patternIdx, step);
            sequencer->SetView(Sequencer::ViewMode::StepDetail);
            sequencer->ClearActiveNotes();
            sequencer->ClearSelectedNotes();
            return true;
        }

        // Check if step already exists in selection
        if(std::find(stepSelected->begin(), stepSelected->end(), step) == stepSelected->end())
        {
            stepSelected->push_back(step);
        }

        bool playAllNotes = false;
        
        if(sequencer->ClearActive())
        {
            sequencer->ClearStep(&pattern, step);
        }
        else if(sequencer->CopyActive() && sequencer->stepSelected.size() >= 2) // Self is included
        {
            sequencer->CopyStep(&pattern, sequencer->stepSelected[0], step);
        }
        else if(!noteSelected->empty())
        {
            bool existAlready = false;

            uint16_t startTime = step * Sequence::PPQN;
            uint16_t endTime = startTime + Sequence::PPQN - 1;

            for (const auto& [note, velocity] : sequencer->noteSelected)
            {
                if (pattern.RemoveNoteEventsInRange(startTime, endTime, note))
                {
                    existAlready = true;
                    sequencer->sequence.SetDirty();
                }
            }

            if (existAlready == false)
            {
                for (const auto& [note, velocity] : sequencer->noteSelected)
                {
                    SequenceEvent event = SequenceEvent::Note(note, velocity, false);
                    pattern.AddEvent(step * Sequence::PPQN, event);
                    sequencer->sequence.SetDirty();
                }
            }
        }
        
        // Populate noteActive with notes from this step and send MIDI NoteOn
        auto it = pattern.events.lower_bound(startTime);
        while (it != pattern.events.end() && it->first <= endTime)
        {
            if (it->second.eventType == SequenceEventType::NoteEvent)
            {
                const SequenceEventNote& noteData = std::get<SequenceEventNote>(it->second.data);
                sequencer->noteActive.insert(noteData.note);
                MatrixOS::MIDI::Send(MidiPacket::NoteOn(channel, noteData.note, noteData.velocity));
            }
            ++it;
        }
    }
    else if(keyInfo->state == RELEASED)
    {
        // Remove step from selection
        
        auto stepIt = std::find(stepSelected->begin(), stepSelected->end(), step);
        if(stepIt != stepSelected->end())
        {
            stepSelected->erase(stepIt);
        }

        // Clear noteActive from notes in this step and send MIDI NoteOff
        auto eventIt = pattern.events.lower_bound(startTime);
        while (eventIt != pattern.events.end() && eventIt->first <= endTime)
        {
            if (eventIt->second.eventType == SequenceEventType::NoteEvent)
            {
                const SequenceEventNote& noteData = std::get<SequenceEventNote>(eventIt->second.data);
                auto noteIt = sequencer->noteActive.find(noteData.note);
                if (noteIt != sequencer->noteActive.end())
                {
                    sequencer->noteActive.erase(noteIt);
                }

                // Only send NoteOff if note is not currently selected on NotePad
                if(noteSelected->find(noteData.note) == noteSelected->end())
                {
                    MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, noteData.note, 0));
                }
            }
            ++eventIt;
        }
    }

    return true;
}

bool PatternPad::Render(Point origin)
{
    uint8_t track = sequencer->track;
    uint8_t clip = sequencer->sequence.GetPosition(track).clip;
    uint8_t patternIdx = sequencer->sequence.GetPosition(track).pattern;

    Color trackColor = sequencer->meta.tracks[track].color;

    // Check if pattern is selected
    if(patternIdx >= sequencer->sequence.GetPatternCount(track, clip))
    {
        sequencer->sequence.SetPattern(track, 0);
        patternIdx = 0;
    }

    SequencePattern& pattern = sequencer->sequence.GetPattern(track, clip, (uint8_t)patternIdx);


    // Render base
    for(uint8_t step = 0; step < pattern.quarterNotes; step++)
    {
        Point point = Point(step % width, step / width);
        MatrixOS::LED::SetColor(origin + point, trackColor.Dim());
    }

    // Render Note
    uint16_t hasNote = 0;
    for(uint8_t slot = 0; slot < pattern.quarterNotes; slot++)
    {
        uint16_t startTime = slot * Sequence::PPQN;
        uint16_t endTime = startTime + Sequence::PPQN - 1;

        bool shouldRender = false;

        // If notes are selected, only render events with matching notes
        bool noteFilter = !noteSelected->empty();
        if(noteFilter)
        {
            // Check if any event in this slot has a note that's selected
            auto it = pattern.events.lower_bound(startTime);
            while (it != pattern.events.end() && it->first <= endTime)
            {
                if (it->second.eventType == SequenceEventType::NoteEvent)
                {
                    const SequenceEventNote& noteData = std::get<SequenceEventNote>(it->second.data);
                    if(noteSelected->find(noteData.note) != noteSelected->end())
                    {
                        shouldRender = true;
                        break;
                    }
                }
                ++it;
            }
        }
        else
        {
            // No notes selected, render all note events
            shouldRender = pattern.HasEventInRange(startTime, endTime, SequenceEventType::NoteEvent);
        }

        if(shouldRender)
        {
            Point point = Point(slot % width, slot / width);
            hasNote |= 1 << slot;
            MatrixOS::LED::SetColor(origin + point, noteFilter ? Color::Green : trackColor);
        }
    }

    // Render Selected
    for(uint8_t selectedStep : *stepSelected)
    {
        Point point = Point(selectedStep % width, selectedStep / width);
        MatrixOS::LED::SetColor(origin + point, Color::White);
    }

    // Render Cursor
    if(sequencer->sequence.Playing(track))
    {
        SequencePosition& position = sequencer->sequence.GetPosition(track);
        if(patternIdx == position.pattern)
        {
            uint8_t slot = position.quarterNote;
            Point point = Point(slot % width, slot / width);

            uint8_t breathingScale = sequencer->sequence.QuarterNoteProgressBreath();

            if(sequencer->sequence.RecordEnabled() == false)
            {
                Color baseColor = trackColor.DimIfNot(hasNote & (1 << slot));
                uint16_t scale = breathingScale / 4 * 3;
                if(hasNote & (1 << slot)) { scale += 64; }
                Color color = Color::Crossfade(baseColor, Color::White, Fract16(scale, 8));
                MatrixOS::LED::SetColor(origin + point, color);
            }
            else
            {
                Color baseColor = hasNote & (1 << slot) ? Color(0xFF0040) : Color(0xFF0000);
                uint16_t scale = breathingScale / 4 * 3 + 64;
                MatrixOS::LED::SetColor(origin + point, baseColor.Scale(scale));
            }
        }
    }

    return true;
}
