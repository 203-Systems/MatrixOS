#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class ClipLauncher : public UIComponent {
    Sequencer* sequencer;
    std::function<void(uint8_t track, uint8_t clip)> changeCallback;

    public:
    ClipLauncher(Sequencer* sequencer)
    {
        this->sequencer = sequencer;
    }

    Dimension GetSize() { return Dimension(8, 7); }

    void OnChange(std::function<void(uint8_t track, uint8_t clip)> callback)
    {
        changeCallback = callback;
    }

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo)
    {
        if(keyInfo->State() == PRESSED)
        {
            uint8_t track = xy.x;
            uint8_t clip = xy.y;

            // Check if track exists
            if(track >= sequencer->sequence.GetTrackCount())
            {
                return true;
            }

            // Handle copy mode
            if(sequencer->CopyActive())
            {
                uint8_t sourceTrack = sequencer->track;
                uint8_t sourceClip = sequencer->trackClipIdx[sourceTrack];

                // Copy clip (includes all patterns and enabled state)
                sequencer->sequence.CopyClip(sourceTrack, sourceClip, track, clip);

                // Select the newly copied clip
                sequencer->track = track;
                sequencer->trackClipIdx[track] = clip;

                if (changeCallback != nullptr)
                {
                    changeCallback(track, clip);
                }
            }
            // If clip doesn't exist, create it
            else if(!sequencer->sequence.ClipExists(track, clip))
            {
                if(sequencer->ClearActive())
                {
                    // Don't create clip if Clear is active
                    return true;
                }

                sequencer->sequence.NewClip(track, clip);

                // Select the newly created clip
                sequencer->track = track;
                sequencer->trackClipIdx[track] = clip;

                if (changeCallback != nullptr)
                {
                    changeCallback(track, clip);
                }
            }
            // If clip exists and Clear is active, delete it
            else if(sequencer->ClearActive())
            {
                sequencer->sequence.DeleteClip(track, clip);

                // Update selection if we deleted the current clip
                if(sequencer->trackClipIdx[track] == clip)
                {
                    // Find first available clip or default to 0
                    sequencer->trackClipIdx[track] = 0;
                }
            }
            // Select clip
            else
            {
                sequencer->track = track;
                sequencer->trackClipIdx[track] = clip;

                if (changeCallback != nullptr)
                {
                    changeCallback(track, clip);
                }
            }
        }
        return true;
    }

    virtual bool Render(Point origin)
    {
        uint8_t trackCount = sequencer->sequence.GetTrackCount();

        // Render all clip slots
        for(uint8_t track = 0; track < 8; track++)
        {
            for(uint8_t clip = 0; clip < 7; clip++)
            {
                Point point = origin + Point(track, clip);
                Color color;

                if(track >= trackCount)
                {
                    // No track at this position
                    color = Color::Black;
                }
                else if(!sequencer->sequence.ClipExists(track, clip))
                {
                    // Clip doesn't exist
                    color = Color(0x404040);
                }
                else
                {
                    Color trackColor = sequencer->meta.tracks[track].color;
                    bool isSelected = (sequencer->track == track && sequencer->trackClipIdx[track] == clip);
                    bool isPlaying = sequencer->sequence.Playing() && sequencer->sequence.GetPosition(track).clip == clip;

                    if(isPlaying)
                    {
                        // Playing clip shows green
                        color = Color::Green;
                    }
                    else if(isSelected)
                    {
                        // Selected clip shows white
                        color = Color::White;
                    }
                    else
                    {
                        // Clip exists - show track color, dim if disabled
                        bool enabled = sequencer->sequence.GetClipEnabled(track, clip);
                        color = trackColor.DimIfNot(enabled);
                    }
                }

                MatrixOS::LED::SetColor(point, color);
            }
        }

        return true;
    }
};
