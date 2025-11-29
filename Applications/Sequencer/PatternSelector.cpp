#include "PatternSelector.h"

PatternSelector::PatternSelector(Sequencer* sequencer)
{
    this->sequencer = sequencer;
}

bool PatternSelector::IsEnabled() {
    bool enable = sequencer->currentView == Sequencer::ViewMode::Sequencer &&
                  ((sequencer->ShiftActive() && ((MatrixOS::SYS::Millis() - sequencer->shiftOnTime) > 150)) || sequencer->patternView);
    sequencer->patternViewActive = enable;
    lengthAdjustmentMode &= enable;
    return enable;
}

Dimension PatternSelector::GetSize() { return Dimension(8, lengthAdjustmentMode ? 4 : 2); }

bool PatternSelector::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    if(xy.y < 2) // Selector part
    {
        if(keyInfo->State() == PRESSED)
        {
            uint8_t track = sequencer->track;
            SequencePosition position = sequencer->sequence.GetPosition(track);
            uint8_t clip = position.clip;
            uint8_t pattern = position.pattern;
            uint8_t patternCount = sequencer->sequence.GetPatternCount(track, clip);
            uint8_t patternIdx = xy.x + xy.y * 8; // Convert 2D coordinates to pattern index
            uint8_t newPatternIdx = 0;

            // Check if clicking on add button
            if(patternIdx == patternCount && patternCount < 16)
            {
                if(sequencer->ShiftActive() && sequencer->patternView)
                {
                    lengthAdjustmentMode = true;
                    newPatternIdx = sequencer->sequence.NewPattern(track, clip);
                }
                else if(sequencer->CopyActive() && sequencer->sequence.Playing(track) == false)
                {
                    // Copy mode: Copy selected pattern to new pattern
                    if(sequencer->copySourcePattern >= 0)
                    {
                        sequencer->sequence.CopyPattern(track, clip, sequencer->copySourcePattern, track, clip, 255);
                        newPatternIdx = clip;
                        sequencer->SetMessage(SequencerMessage::COPIED);
                    }
                }
                // Add new empty pattern
                else
                {
                    newPatternIdx = sequencer->sequence.NewPattern(track, clip);
                }

                if(newPatternIdx > 0)
                {
                    // Select the newly created pattern only if not playing
                    if(!sequencer->sequence.Playing(track))
                    {
                        sequencer->sequence.SetPattern(track, newPatternIdx);
                        sequencer->ClearActiveNotes();
                        sequencer->stepSelected.clear();
                    }
                }
            }
            // Check if clicking on existing pattern
            else if(patternIdx < patternCount)
            {
                // Toggle Length Adjustment based on shift
                if(sequencer->ShiftActive() && sequencer->patternView)
                {
                    if(patternIdx == pattern && lengthAdjustmentMode)
                    {
                        lengthAdjustmentMode = false;
                    }
                    else
                    {
                        lengthAdjustmentMode = true;
                        if(sequencer->sequence.Playing(track) == false)
                        {
                            sequencer->sequence.SetPattern(track, patternIdx);
                            sequencer->ClearActiveNotes();
                            sequencer->stepSelected.clear();
                        }
                    }
                }
                // Delete pattern if ClearActive
                else if(sequencer->ClearActive())
                {
                    sequencer->sequence.DeletePattern(track, clip, patternIdx);

                    // Update selected pattern index if needed
                    uint8_t currentPattern = sequencer->sequence.GetPosition(track).pattern;
                    if(currentPattern >= sequencer->sequence.GetPatternCount(track, clip))
                    {
                        currentPattern = sequencer->sequence.GetPatternCount(track, clip) - 1;
                        sequencer->sequence.SetPattern(track, currentPattern);
                    }

                    sequencer->ClearActiveNotes();
                    sequencer->stepSelected.clear();
                    sequencer->SetMessage(SequencerMessage::CLEARED);
                }
                // Copy pattern if CopyActive
                else if(sequencer->CopyActive() && sequencer->sequence.Playing(track) == false)
                {
                    // Source selection if none yet
                    if(sequencer->copySourcePattern < 0)
                    {
                        sequencer->copySourcePattern = patternIdx;
                        return true;
                    }
                    else
                    {
                        // Copy from existing source to this pattern
                        uint8_t sourcePattern = sequencer->copySourcePattern;
                        uint8_t destPattern = patternIdx;

                        sequencer->sequence.CopyPattern(track, clip, sourcePattern, track, clip, destPattern);

                        sequencer->ClearActiveNotes();
                        sequencer->stepSelected.clear();
                        sequencer->SetMessage(SequencerMessage::COPIED);
                    }
                }
                // Select pattern
                else if(patternIdx != sequencer->sequence.GetPosition(track).pattern)
                {
                    if(sequencer->sequence.Playing() == false) // Disable pattern switching if playing
                    {
                        sequencer->sequence.SetPattern(track, patternIdx);
                        sequencer->ClearActiveNotes();
                        sequencer->stepSelected.clear();
                    }
                }
                else if(patternIdx == pattern && lengthAdjustmentMode)
                {
                    // Clicking current pattern toggles length mode off
                    lengthAdjustmentMode = false;
                }
            }
            else // empty spot
            {
                lengthAdjustmentMode = false; // Just turn off Length Adjustment Mode
            }
        }
        else if(keyInfo->State() == HOLD)
        {
            uint8_t track = sequencer->track;
            uint8_t clip = sequencer->sequence.GetPosition(track).clip;
            uint8_t patternCount = sequencer->sequence.GetPatternCount(track, clip);
            uint8_t patternIdx = xy.x + xy.y * 8; // Convert 2D coordinates to pattern index

            // Enable length adjustment mode if holding on an existing pattern
            if(patternIdx < patternCount)
            {
                lengthAdjustmentMode = !lengthAdjustmentMode;
            }
        }
    }
    else // Length Adjustment Mode
    {
        if(keyInfo->State() == PRESSED)
        {
            uint8_t track = sequencer->track;
            uint8_t clip = sequencer->sequence.GetPosition(track).clip;
            uint8_t patternIdx = sequencer->sequence.GetPosition(track).pattern;
            SequencePattern* pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);
            if (!pattern) { return true; }

            uint8_t lengthIdx = xy.x + (xy.y - 2) * 8; // Convert 2D coordinates to length index (offset by 2 rows)
            uint8_t newLength = lengthIdx + 1; // Length is 1-indexed (1-16)

            if(newLength >= 1 && newLength <= 16)
            {
                sequencer->sequence.PatternSetLength(pattern, newLength);
            }
        }
        else if(keyInfo->State() == HOLD)
        {
            uint8_t lengthIdx = xy.x + (xy.y - 2) * 8;
            uint8_t newLength = lengthIdx + 1;
            MatrixOS::UIUtility::TextScroll("Pattern Length " + std::to_string(newLength), Color::White);
        }
    }
    return true;
}

bool PatternSelector::Render(Point origin)
{
    uint8_t track = sequencer->track;
    uint8_t clip = sequencer->sequence.GetPosition(track).clip;
    uint8_t patternCount = sequencer->sequence.GetPatternCount(track, clip);
    uint8_t selectedPattern = sequencer->sequence.GetPosition(track).pattern;
    Color trackColor = sequencer->meta.tracks[track].color;

    // Special state: Step copy mode active (from PatternPad)
    if(sequencer->CopyActive() && sequencer->copySourceStep >= 0 && sequencer->sequence.Playing(track) == false)
    {
        // Render all pattern slots dimmed when step copy is active
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t x = i % 8;
            uint8_t y = i / 8;

            if(i < patternCount)
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::White.Dim(32));
            }
            else
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);
            }
        }
        return true;
    }

    if((sequencer->CopyActive() && sequencer->sequence.Playing(track) == false) || sequencer->ClearActive())
    {
        // Copy mode rendering
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t x = i % 8;
            uint8_t y = i / 8;

            if(i < patternCount)
            {
                // Pattern exists
                if(sequencer->copySourcePattern >= 0 && i == sequencer->copySourcePattern)
                {
                    // Highlight selected source in white
                    MatrixOS::LED::SetColor(origin + Point(x, y), Color::White);
                }
                else
                {
                    // Show existing patterns in track color
                    MatrixOS::LED::SetColor(origin + Point(x, y), trackColor);
                }
            }
            else if(i == patternCount && patternCount < 16 && sequencer->copySourcePattern >= 0)
            {
                // Show add button in dim white when source is selected
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::White.Dim());
            }
            else
            {
                // Hide other empty slots in copy mode
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);
            }
        }
    }
    else
    {
        // Normal mode rendering
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t x = i % 8;
            uint8_t y = i / 8;

            if(i < patternCount)
            {
                // Pattern exists
                if(i == selectedPattern)
                {
                    if(sequencer->sequence.Playing(track))
                    {
                        uint8_t breathingScale = sequencer->sequence.QuarterNoteProgressBreath();
                        Color color = Color::Crossfade(trackColor, Color::White, Fract16(breathingScale / 4 * 3 + 64, 8));
                        MatrixOS::LED::SetColor(origin + Point(x, y), color);
                    }
                    else
                    {
                        Color color = Color::Crossfade(trackColor, Color::White, Fract16(0x9000));
                        MatrixOS::LED::SetColor(origin + Point(x, y), color);
                    }
                }
                else
                {
                    MatrixOS::LED::SetColor(origin + Point(x, y), trackColor);
                }
            }
            else if(i == patternCount)
            {
                // Add button at next available slot
                MatrixOS::LED::SetColor(origin + Point(x, y), Color(0x202020));
            }
            else
            {
                // Empty pattern slot
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);
            }
        }
    }

    // Length Adjustment Mode
    if(lengthAdjustmentMode)
    {
        uint8_t patternIdx = sequencer->sequence.GetPosition(track).pattern;
        SequencePattern* pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);
        if (pattern)
        {
            for(uint8_t i = 0; i < 16; i++)
            {
                Point point = origin + Point(i % 8, i / 8 + 2);
                if((i + 1) <= pattern->steps)
                {
                    MatrixOS::LED::SetColor(point, Color::White);
                }
                else
                {
                    MatrixOS::LED::SetColor(point, Color(0x202020));
                }
            }
        }
    }

    return true;
}
