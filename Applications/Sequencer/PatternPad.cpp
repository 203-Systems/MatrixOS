#include "PatternPad.h"

PatternPad::PatternPad(Sequencer* sequencer)
{
    this->sequencer = sequencer;
    width = sequencer->sequence.GetTrackCount();
}

Dimension PatternPad::GetSize() { return Dimension(8, TwoPatternMode() ? 4 : 2); }

bool PatternPad::IsEnabled()
{
    return sequencer->currentView == Sequencer::ViewMode::Sequencer;
}

bool PatternPad::TwoPatternMode()
{
    return sequencer->meta.tracks[sequencer->track].twoPatternMode && !sequencer->patternViewActive;
}

bool PatternPad::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    if(keyInfo->state != PRESSED && keyInfo->state != RELEASED)
    {
        return true;
    }

    uint8_t track = sequencer->track;
    SequencePosition* pos = sequencer->sequence.GetPosition(track);
    uint8_t clip = pos->clip;
    uint8_t currentPattern = pos->pattern;
    uint8_t channel = sequencer->sequence.GetChannel(track);

    // Calculate pattern and step indices based on mode
    uint8_t index = xy.x + xy.y * width;  // 0-31 in TwoPatternMode, 0-15 in normal
    uint8_t patternIdx;
    uint8_t step;

    if(TwoPatternMode())
    {
        uint8_t basePattern = (currentPattern / 2) * 2;  // Clamp to even (0,2,4,6...)
        patternIdx = basePattern + (index >= 16 ? 1 : 0);  // +0 for top, +1 for bottom
        step = index % 16;  // Step within pattern (0-15)

        // Check if this pattern exists
        if(patternIdx >= sequencer->sequence.GetPatternCount(track, clip))
        {
            return true;  // No pattern here, ignore click
        }
    }
    else
    {
        patternIdx = currentPattern;
        step = index;
    }

    SequencePattern* pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);
    if (!pattern) { return true; }

    int16_t clockTillStart = sequencer->sequence.GetClocksTillStart();
    if(sequencer->sequence.RecordEnabled() && (clockTillStart > 0))
    {
        return true;
    }

    uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
    uint16_t startTime = step * pulsesPerStep;
    uint16_t endTime = startTime + pulsesPerStep - 1;

    if(keyInfo->state == PRESSED)
    {
        bool hasEvent = sequencer->sequence.PatternHasEventInRange(pattern, startTime, endTime);
        if(sequencer->CopyActive() && sequencer->sequence.Playing(track) == false)
        {
            // Source selection if none yet
            if(!sequencer->copySource.Selected())
            {
                if(hasEvent == false)
                {
                    return true; // only allow selecting source with events
                }
                sequencer->copySource.type = SequenceSelectionType::STEP;
                sequencer->copySource.track = track;
                sequencer->copySource.clip = clip;
                sequencer->copySource.pattern = patternIdx;
                sequencer->copySource.step = step;
                return true;
            }
            else
            {
                // Copy from existing source to this step
                uint8_t srcTrack = sequencer->copySource.track;
                uint8_t srcClip = sequencer->copySource.clip;
                uint8_t srcPatternIdx = sequencer->copySource.pattern;
                uint8_t srcStep = sequencer->copySource.step;

                SequencePattern* srcPatternPtr = sequencer->sequence.GetPattern(srcTrack, srcClip, srcPatternIdx);

                // Clear destination step first
                sequencer->sequence.PatternClearStepEvents(pattern, step, pulsesPerStep);

                if(srcPatternPtr)
                {
                    // If copying within same pattern, use existing function
                    if(srcPatternPtr == pattern)
                    {
                        sequencer->sequence.PatternCopyStepEvents(pattern, srcStep, step, pulsesPerStep);
                    }
                    else
                    {
                        // Copy between different patterns - manually copy events
                        uint16_t srcStart = srcStep * pulsesPerStep;
                        uint16_t srcEnd = srcStart + pulsesPerStep - 1;
                        uint16_t destStart = step * pulsesPerStep;

                        // Collect and copy events
                        auto it = srcPatternPtr->events.lower_bound(srcStart);
                        while (it != srcPatternPtr->events.end() && it->first <= srcEnd)
                        {
                            uint16_t offset = it->first - srcStart;
                            uint16_t newTimestamp = destStart + offset;
                            pattern->events.insert({newTimestamp, it->second});
                            ++it;
                        }
                    }
                    sequencer->SetMessage(SequencerMessage::COPIED);
                }
                return true;
            }
        }

        // Check for shift-click to enter StepDetail view (only when not playing locally)
        if(hasEvent && sequencer->ShiftActive() && !sequencer->sequence.Playing(track))
        {
            sequencer->ShiftEventOccured();
            sequencer->sequence.SetPosition(track, clip, patternIdx, step);
            sequencer->SetView(Sequencer::ViewMode::StepDetail);
            sequencer->ClearActiveNotes();
            sequencer->ClearSelectedNotes();
            sequencer->stepSelected.clear();
            return true;
        }

        // Check if step already exists in selection
        if(sequencer->stepSelected.find(std::make_pair(patternIdx, step)) == sequencer->stepSelected.end())
        {
            sequencer->stepSelected.insert(std::make_pair(patternIdx, step));
        }

        bool playAllNotes = false;
        
        if(sequencer->ClearActive())
        {
            uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
            uint16_t startTime = step * pulsesPerStep;
            uint16_t endTime = startTime + pulsesPerStep - 1;
            uint8_t channel = sequencer->sequence.GetChannel(track);
            
            // Remove notes from noteActive and send noteOff
            auto it = pattern->events.lower_bound(startTime);
            while (it != pattern->events.end() && it->first <= endTime)
            {
                if (it->second.eventType == SequenceEventType::NoteEvent)
                {
                    const SequenceEventNote &noteData = std::get<SequenceEventNote>(it->second.data);
                    auto activeIt = sequencer->noteActive.find(noteData.note);
                    if (activeIt != sequencer->noteActive.end())
                    {
                        MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, noteData.note, 0));
                        sequencer->noteActive.erase(activeIt);
                    }
                }
                ++it;
            }

            if(sequencer->sequence.PatternClearStepEvents(pattern, step, pulsesPerStep))
            {
                sequencer->SetMessage(SequencerMessage::CLEARED);
            }
        }
        else if(!sequencer->noteSelected.empty())
        {
            bool existAlready = false;

            uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
            uint16_t startTime = step * pulsesPerStep;
            uint16_t endTime = startTime + pulsesPerStep - 1;

            for (const auto& [note, velocity] : sequencer->noteSelected)
            {
                if (sequencer->sequence.PatternClearNotesInRange(pattern, startTime, endTime, note))
                {
                    existAlready = true;
                }
            }

            if (existAlready == false)
            {
                for (const auto& [note, velocity] : sequencer->noteSelected)
                {
                    SequenceEvent event = SequenceEvent::Note(note, velocity, false);
                    sequencer->sequence.PatternAddEvent(pattern, step * sequencer->sequence.GetPulsesPerStep(), event);
                }
            }
        }
        
        // Populate noteActive with notes from this step and send MIDI NoteOn
        auto it = pattern->events.lower_bound(startTime);
        while (it != pattern->events.end() && it->first <= endTime)
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
        auto stepIt = sequencer->stepSelected.find(std::make_pair(patternIdx, step));
        if(stepIt != sequencer->stepSelected.end())
        {
            sequencer->stepSelected.erase(stepIt);
        }

        // Clear noteActive from notes in this step and send MIDI NoteOff
        auto eventIt = pattern->events.lower_bound(startTime);
        while (eventIt != pattern->events.end() && eventIt->first <= endTime)
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
                if(sequencer->noteSelected.find(noteData.note) == sequencer->noteSelected.end())
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
    SequencePosition* pos = sequencer->sequence.GetPosition(track);
    uint8_t clip = pos->clip;
    uint8_t currentPattern = pos->pattern;

    // Handle countdown mode
    int16_t clockTillStart = sequencer->sequence.GetClocksTillStart();
    if(sequencer->sequence.RecordEnabled() && (clockTillStart > 0))
    {
        uint8_t totalSteps = 16;
        for(uint8_t step = 0; step < 16; step++)
        {
            bool lit = clockTillStart <= 6 * (totalSteps - step);
            Point xy = Point(step % width, step / width);
            MatrixOS::LED::SetColor(origin + xy, Color::Red.DimIfNot(lit));
            if(TwoPatternMode())
            {
                xy = xy + Point(0, 2);
                MatrixOS::LED::SetColor(origin + xy, Color::Red.DimIfNot(lit));
            }
        }
        return true;
    }

    // Determine which patterns to render
    uint8_t basePattern = TwoPatternMode() ? (currentPattern / 2) * 2 : currentPattern;
    uint8_t patternCount = sequencer->sequence.GetPatternCount(track, clip);
    uint8_t patternsToRender = TwoPatternMode() ? 2 : 1;
    Color trackColor = sequencer->meta.tracks[track].color;

    for(uint8_t i = 0; i < patternsToRender; i++)
    {
        uint8_t patternIdx = basePattern + i;

        // Skip if pattern doesn't exist
        if(patternIdx >= patternCount) { continue; }

        // Calculate origin for this pattern
        Point patternOrigin = origin + Point(0, i * 2);

        // Render single pattern
        SequencePattern* pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);
        if (!pattern) { continue; }

        // Special state: Pattern copy mode active (from PatternSelector)
        if(sequencer->CopyActive() && sequencer->copySource.IsType(SequenceSelectionType::PATTERN) &&
           sequencer->sequence.Playing(track) == false)
        {
            // Render all slots dimmed when pattern copy is active
            for(uint8_t step = 0; step < pattern->steps; step++)
            {
                Point point = Point(step % width, step / width);
                MatrixOS::LED::SetColor(patternOrigin + point, Color::White.Dim(32));
            }
            continue;
        }

        if((sequencer->CopyActive() && sequencer->sequence.Playing(track) == false) || sequencer->ClearActive())
        {
            // Check if we have a step copy source
            bool hasStepCopySource = sequencer->copySource.IsType(SequenceSelectionType::STEP);
            bool isStepCopySourceInThisPattern = hasStepCopySource &&
                                                 sequencer->copySource.track == track &&
                                                 sequencer->copySource.clip == clip &&
                                                 sequencer->copySource.pattern == patternIdx;
            bool isWrongCopyType = sequencer->CopyActive() && sequencer->copySource.Selected() && !sequencer->copySource.IsType(SequenceSelectionType::STEP);

            // Render Base
            for(uint8_t step = 0; step < pattern->steps; step++)
            {
                Point point = Point(step % width, step / width);
                MatrixOS::LED::SetColor(patternOrigin + point, hasStepCopySource ? trackColor.Dim() : Color::White.Dim(32));
            }

            // Render Step
            uint16_t hasNote = 0;
            uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
            for(uint8_t slot = 0; slot < pattern->steps; slot++)
            {
                uint16_t startTime = slot * pulsesPerStep;
                uint16_t endTime = startTime + pulsesPerStep - 1;

                bool hasEventInSlot = sequencer->sequence.PatternHasEventInRange(pattern, startTime, endTime, SequenceEventType::NoteEvent);
                if(hasEventInSlot)
                {
                    if(isWrongCopyType)
                    {
                        // Gray out when wrong copy type is selected
                        MatrixOS::LED::SetColor(patternOrigin + Point(slot % width, slot / width), Color::White.Dim(32));
                    }
                    else
                    {
                        MatrixOS::LED::SetColor(patternOrigin + Point(slot % width, slot / width), trackColor);
                    }
                }
            }

            // Render Selected source step in this pattern
            if(isStepCopySourceInThisPattern)
            {
                uint8_t sourceStep = sequencer->copySource.step;
                Point point = Point(sourceStep % width, sourceStep / width);
                MatrixOS::LED::SetColor(patternOrigin + point, Color::White);
            }
        }
        else // Normal mode
        {
            // Render Base
            for(uint8_t step = 0; step < pattern->steps; step++)
            {
                Point point = Point(step % width, step / width);
                MatrixOS::LED::SetColor(patternOrigin + point, trackColor.Dim());
            }

            // Render Step
            uint16_t hasNote = 0;
            uint16_t pulsesPerStep = sequencer->sequence.GetPulsesPerStep();
            for(uint8_t slot = 0; slot < pattern->steps; slot++)
            {
                uint16_t startTime = slot * pulsesPerStep;
                uint16_t endTime = startTime + pulsesPerStep - 1;

                bool hasEventInSlot = sequencer->sequence.PatternHasEventInRange(pattern, startTime, endTime, SequenceEventType::NoteEvent);
                bool shouldRender = false;

                // If notes are selected, only render events with matching notes
                bool noteFilter = !sequencer->noteSelected.empty();
                if(noteFilter)
                {
                    // Check if any event in this slot has a note that's selected
                    auto it = pattern->events.lower_bound(startTime);
                    while (it != pattern->events.end() && it->first <= endTime)
                    {
                        if (it->second.eventType == SequenceEventType::NoteEvent)
                        {
                            const SequenceEventNote& noteData = std::get<SequenceEventNote>(it->second.data);
                            if(sequencer->noteSelected.find(noteData.note) != sequencer->noteSelected.end())
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
                    shouldRender = hasEventInSlot;
                }

                if(shouldRender)
                {
                    hasNote |= 1 << slot;
                    Color color = noteFilter ? Color::Green : trackColor;
                    Point point = Point(slot % width, slot / width);
                    MatrixOS::LED::SetColor(patternOrigin + point, color);
                }
                else if(noteFilter && hasEventInSlot)
                {
                    // Event exists but not in current filter; dim the track color
                    Point point = Point(slot % width, slot / width);
                    MatrixOS::LED::SetColor(patternOrigin + point, Color::Crossfade(trackColor, Color::White, Fract16(0xA000)).Scale(127));
                }
            }

            // Render Selected (only for current pattern)
            for(const auto& selection : sequencer->stepSelected)
            {
                if(selection.first == patternIdx) // Only render selections for current pattern
                {
                    uint8_t selectedStep = selection.second;
                    Point point = Point(selectedStep % width, selectedStep / width);
                    MatrixOS::LED::SetColor(patternOrigin + point, Color::White);
                }
            }

            // Render Cursor
            if(sequencer->sequence.Playing(track))
            {
                SequencePosition* pos = sequencer->sequence.GetPosition(track);
                if(patternIdx == pos->pattern)
                {
                    uint8_t slot = pos->step;
                    Point point = Point(slot % width, slot / width);

                    if(sequencer->sequence.RecordEnabled() && sequencer->sequence.ShouldRecord(track))
                    {
                        Color baseColor = hasNote & (1 << slot) ? Color(0xFF0040) : Color(0xFF0000);
                        MatrixOS::LED::SetColor(patternOrigin + point, baseColor);
                    }
                    else
                    {
                        Color color = Color::Crossfade(trackColor, Color::White, Fract16(0xA000));
                        MatrixOS::LED::SetColor(patternOrigin + point, color);
                    }
                }
            }
        }
    }

    return true;
}
