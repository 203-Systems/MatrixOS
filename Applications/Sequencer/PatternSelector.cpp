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
        uint8_t patternIdx = xy.x + xy.y * 8; // Convert 2D coordinates to pattern index

        if(keyInfo->State() == PRESSED)
        {
            uint8_t track = sequencer->track;
            SequencePosition* pos = sequencer->sequence.GetPosition(track);
            uint8_t clip = pos->clip;
            uint8_t pattern = pos->pattern;
            uint8_t patternCount = sequencer->sequence.GetPatternCount(track, clip);
            uint8_t newPatternIdx = 0;
            bool copyActive = sequencer->CopyActive() && (sequencer->copySource.IsType(SequenceSelectionType::PATTERN) || sequencer->copySource.IsType(SequenceSelectionType::NONE));

            sequencer->patternSelected.insert(patternIdx);

            // Check if clicking on add button
            if(patternIdx == patternCount && patternCount < 16)
            {   
                if(sequencer->ClearActive())
                {
                    return true; // Not relevent
                }
                else if(copyActive)
                {
                    // Copy mode: Copy selected pattern to new pattern
                    if(sequencer->copySource.IsType(SequenceSelectionType::PATTERN))
                    {
                        uint8_t sourceTrack = sequencer->copySource.track;
                        uint8_t sourceClip = sequencer->copySource.clip;
                        uint8_t sourcePattern = sequencer->copySource.pattern;
                        newPatternIdx = sequencer->sequence.NewPattern(track, clip);
                        sequencer->sequence.CopyPattern(sourceTrack, sourceClip, sourcePattern, track, clip, newPatternIdx);
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
                // Delete pattern if ClearActive
                if(sequencer->ClearActive())
                {
                    sequencer->sequence.DeletePattern(track, clip, patternIdx);

                    // Update selected pattern index if needed
                    uint8_t currentPattern = pos->pattern;
                    if(currentPattern >= sequencer->sequence.GetPatternCount(track, clip))
                    {
                        currentPattern = sequencer->sequence.GetPatternCount(track, clip) - 1;
                        sequencer->sequence.SetPattern(track, currentPattern);
                    }

                    sequencer->patternSelected.clear();
                    sequencer->SetMessage(SequencerMessage::CLEARED);
                }
                // Copy pattern if CopyActive
                else if(copyActive)
                {
                    // Source selection if none yet
                    if(!sequencer->copySource.Selected())
                    {
                        sequencer->copySource.type = SequenceSelectionType::PATTERN;
                        sequencer->copySource.track = track;
                        sequencer->copySource.clip = clip;
                        sequencer->copySource.pattern = patternIdx;
                        return true;
                    }
                    else
                    {
                        // Copy from existing source to this pattern
                        uint8_t sourceTrack = sequencer->copySource.track;
                        uint8_t sourceClip = sequencer->copySource.clip;
                        uint8_t sourcePattern = sequencer->copySource.pattern;
                        uint8_t destPattern = patternIdx;

                        sequencer->sequence.CopyPattern(sourceTrack, sourceClip, sourcePattern, track, clip, destPattern);

                        sequencer->SetMessage(SequencerMessage::COPIED);
                    }
                }
                // Select pattern
                else if(patternIdx != pos->pattern)
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
            uint8_t clip = sequencer->sequence.GetPosition(track)->clip;
            uint8_t patternCount = sequencer->sequence.GetPatternCount(track, clip);
            uint8_t patternIdx = xy.x + xy.y * 8; // Convert 2D coordinates to pattern index

            // Enable length adjustment mode if holding on an existing pattern
            if(patternIdx < patternCount)
            {
                lengthAdjustmentMode = true;
            }
        }
        else if(keyInfo->State() == RELEASED)
        {
            if(keyInfo->Hold() == true && 
                patternIdx == sequencer->sequence.GetPosition(sequencer->track)->pattern &&
                lengthAdjustmentMode)
            {
                lengthAdjustmentMode = false;
            }
            sequencer->patternSelected.erase(patternIdx);
        }
    }
    else // Length Adjustment Mode
    {
        if(keyInfo->State() == PRESSED)
        {
            uint8_t track = sequencer->track;
            SequencePosition* pos = sequencer->sequence.GetPosition(track);
            uint8_t clip = pos->clip;
            uint8_t patternIdx = pos->pattern;
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
    SequencePosition* pos = sequencer->sequence.GetPosition(track);
    uint8_t clip = pos->clip;
    uint8_t patternCount = sequencer->sequence.GetPatternCount(track, clip);
    uint8_t patternIdx = pos->pattern;
    Color trackColor = sequencer->meta.tracks[track].color;

    for(uint8_t i = 0; i < 16; i++)
    {
        uint8_t x = i % 8;
        uint8_t y = i / 8;

        // Check if this pattern is currently selected (held down)
        bool isSelected = sequencer->patternSelected.find(i) != sequencer->patternSelected.end();

        // If selected, render white regardless of whether pattern exists
        if(isSelected)
        {
            MatrixOS::LED::SetColor(origin + Point(x, y), Color::White);
        }
        else if(i < patternCount)
        {

            bool patternSelectedForCopy = sequencer->CopyActive() && sequencer->copySource.IsType(SequenceSelectionType::PATTERN);
            bool isPatternCopySource = patternSelectedForCopy &&
                        sequencer->copySource.track == track &&
                        sequencer->copySource.clip == clip &&
                        i == sequencer->copySource.pattern;

            if(patternSelectedForCopy)
            {
                if(isPatternCopySource)
                {
                    MatrixOS::LED::SetColor(origin + Point(x, y), Color::White);
                }
                else
                {
                    MatrixOS::LED::SetColor(origin + Point(x, y), trackColor);
                }
            }
            else  if(i == patternIdx )
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
            if(sequencer->ClearActive())
            {
                // Slot not available for clear
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);
            }
            else if(sequencer->CopyActive() && !sequencer->copySource.Selected())
            {
                // Slot not available as copy source
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);
            }
            else
            {
                // Add button at next available slot
                MatrixOS::LED::SetColor(origin + Point(x, y), trackColor.Dim(32));
            }
        }
        else
        {
            // Empty pattern slot
            MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);
        }
    }

    // Length Adjustment Mode
    if(lengthAdjustmentMode)
    {   
        uint8_t patternIdx = sequencer->sequence.GetPosition(track)->pattern;
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
