#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class PatternSelector : public UIComponent {
    Sequencer* sequencer;
    std::function<void(uint8_t)> changeCallback;

    public:
    PatternSelector(Sequencer* sequencer)
    {
        this->sequencer = sequencer;
    }

    Dimension GetSize() { return Dimension(8, 2); }

    void OnChange(std::function<void(uint8_t)> callback)
    {
        changeCallback = callback;
    }

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo)
    {
        if(keyInfo->State() == PRESSED)
        {
            uint8_t track = sequencer->track;
            uint8_t clip = sequencer->trackClipIdx[track];
            uint8_t patternCount = sequencer->sequence.GetPatternCount(track, clip);
            uint8_t patternIdx = xy.x + xy.y * 8; // Convert 2D coordinates to pattern index

            // Check if clicking on add button
            if(patternIdx == patternCount && patternCount < 16)
            {
                // Copy current pattern to new pattern if CopyActive
                if(sequencer->CopyActive())
                {
                    uint8_t sourcePattern = sequencer->trackPatternIdx[track];
                    sequencer->sequence.CopyPattern(track, clip, sourcePattern, track, clip, 255);

                    // Select the newly created pattern
                    sequencer->trackPatternIdx[track] = sequencer->sequence.GetPatternCount(track, clip) - 1;

                    if (changeCallback != nullptr)
                    {
                        changeCallback(sequencer->trackPatternIdx[track]);
                    }
                }
                // Add new empty pattern
                else
                {
                    int8_t newPatternIdx = sequencer->sequence.NewPattern(track, clip, 16);
                    if(newPatternIdx >= 0)
                    {
                        sequencer->trackPatternIdx[track] = newPatternIdx;
                        if (changeCallback != nullptr)
                        {
                            changeCallback(newPatternIdx);
                        }
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
                    if(sequencer->trackPatternIdx[track] >= sequencer->sequence.GetPatternCount(track, clip))
                    {
                        sequencer->trackPatternIdx[track] = sequencer->sequence.GetPatternCount(track, clip) - 1;
                    }

                    if (changeCallback != nullptr)
                    {
                        changeCallback(sequencer->trackPatternIdx[track]);
                    }
                }
                // Copy pattern if CopyActive
                else if(sequencer->CopyActive()) //Take a short cut here and not check if current pattern is held
                {
                    uint8_t sourcePattern = sequencer->trackPatternIdx[track];
                    uint8_t destPattern = patternIdx;

                    sequencer->sequence.CopyPattern(track, clip, sourcePattern, track, clip, destPattern);

                    if (changeCallback != nullptr)
                    {
                        changeCallback(sequencer->trackPatternIdx[track]);
                    }
                }
                // Select pattern
                else if(patternIdx != sequencer->trackPatternIdx[track])
                {
                    sequencer->trackPatternIdx[track] = patternIdx;
                    if (changeCallback != nullptr)
                    {
                        changeCallback(patternIdx);
                    }
                }
            }
        }
        return true;
    }

    virtual bool Render(Point origin)
    {
        uint8_t track = sequencer->track;
        uint8_t clip = sequencer->trackClipIdx[track];
        uint8_t patternCount = sequencer->sequence.GetPatternCount(track, clip);
        uint8_t selectedPattern = sequencer->trackPatternIdx[track];
        Color trackColor = sequencer->meta.tracks[track].color;

        // Render all 16 pattern slots
        for(uint8_t i = 0; i < 16; i++)
        {
            uint8_t x = i % 8;
            uint8_t y = i / 8;

            if(i < patternCount)
            {
                // Pattern exists
                if(i == selectedPattern)
                {
                    MatrixOS::LED::SetColor(origin + Point(x, y), Color::White);
                }
                else
                {
                    MatrixOS::LED::SetColor(origin + Point(x, y), trackColor);
                }
            }
            else if(i == patternCount)
            {
                // Add button at next available slot
                MatrixOS::LED::SetColor(origin + Point(x, y), Color(0x404040));
            }
            else
            {
                // Empty pattern slot
                MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);
            }
        }

        return true;
    }
};
