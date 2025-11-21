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
                uint8_t sourceClip = sequencer->sequence.GetPosition(sourceTrack).clip;

                // Copy clip (includes all patterns and enabled state)
                sequencer->sequence.CopyClip(sourceTrack, sourceClip, track, clip);

                // Select the newly copied clip
                sequencer->track = track;
                sequencer->sequence.SetClip(track, clip);

                if (changeCallback != nullptr)
                {
                    changeCallback(track, clip);
                }
            }
            // If clip doesn't exist, create it or stop track
            else if(!sequencer->sequence.ClipExists(track, clip))
            {
                if(sequencer->ClearActive())
                {
                    // Don't create clip if Clear is active
                    return true;
                }

                // If track is playing, stop it when clicking empty slot
                if(sequencer->sequence.Playing(track))
                {
                    sequencer->sequence.Stop(track);
                }
                else
                {
                    sequencer->sequence.NewClip(track, clip);

                    // Select the newly created clip
                    sequencer->track = track;
                    sequencer->sequence.SetClip(track, clip);

                    if (changeCallback != nullptr)
                    {
                        changeCallback(track, clip);
                    }
                }
            }
            // If clip exists and Clear is active, delete it
            else if(sequencer->ClearActive())
            {
                sequencer->sequence.DeleteClip(track, clip);

                // Update selection if we deleted the current clip
                if(sequencer->sequence.GetPosition(track).clip == clip)
                {
                    // Find first available clip or default to 0
                    sequencer->sequence.SetClip(track, 0);
                }
            }
            // Select clip
            else
            {
                sequencer->track = track;
                sequencer->sequence.SetClip(track, clip);

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

        uint8_t breathingScale = sequencer->sequence.ClockQuarterNoteProgressBreath();

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
                    // Clip doesn't exist - show black if track is playing, otherwise dim gray
                    if(sequencer->sequence.Playing(track))
                    {
                        color = Color::Black;
                    }
                    else
                    {
                        color = Color(0x202020);
                    }
                }
                else
                {
                    Color trackColor = sequencer->meta.tracks[track].color;
                    bool isCurrentTrack = (sequencer->track == track);
                    bool isSelectedInTrack = (sequencer->sequence.GetPosition(track).clip == clip);
                    bool isPlaying = sequencer->sequence.Playing(track) && isSelectedInTrack;

                    if(isPlaying)
                    {
                        // Playing clip pulse toward white
                        color = Color::Crossfade(trackColor, Color::White, Fract16(breathingScale / 4, 8));
                    }
                    else if(isCurrentTrack && isSelectedInTrack)
                    {
                        // Selected clip in current editing track shows white
                        color = Color::White;
                    }
                    else if(isSelectedInTrack)
                    {
                        // Selected clip in other tracks shows red
                        uint16_t brightness = breathingScale / 2 + 128;
                        if(brightness > 255) { brightness = 255; }
                        color = trackColor.Scale(brightness);
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
