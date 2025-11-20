#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class MixerControl : public UIComponent {
    Sequencer* sequencer;

    public:
    MixerControl(Sequencer* sequencer)
    {
        this->sequencer = sequencer;
    }

    Dimension GetSize() { return Dimension(8, 3); }

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo)
    {
        if(keyInfo->State() == PRESSED)
        {
            uint8_t track = xy.x;
            uint8_t row = xy.y;

            // Check if track exists
            if(track >= sequencer->sequence.GetTrackCount())
            {
                return true;
            }

            // Mute (Y=0)
            if(row == 0)
            {
                // Check if any track is in solo mode
                bool anySolo = false;
                for(uint8_t t = 0; t < sequencer->sequence.GetTrackCount(); t++)
                {
                    if(sequencer->sequence.GetSolo(t))
                    {
                        anySolo = true;
                        break;
                    }
                }

                // Don't allow mute toggle if solo is active
                if(!anySolo)
                {
                    sequencer->sequence.SetMute(track, !sequencer->sequence.GetMute(track));
                }
            }
            // Solo (Y=1)
            else if(row == 1)
            {
                sequencer->sequence.SetSolo(track, !sequencer->sequence.GetSolo(track));
            }
            // Record (Y=2)
            else if(row == 2)
            {
                sequencer->sequence.SetRecord(track, !sequencer->sequence.GetRecord(track));
            }
        }
        return true;
    }

    virtual bool Render(Point origin)
    {
        uint8_t trackCount = sequencer->sequence.GetTrackCount();

        // Check if any track is in solo mode
        bool anySolo = false;
        for(uint8_t t = 0; t < trackCount; t++)
        {
            if(sequencer->sequence.GetSolo(t))
            {
                anySolo = true;
                break;
            }
        }

        // Render all tracks
        for(uint8_t track = 0; track < 8; track++)
        {
            if(track >= trackCount)
            {
                // No track - render black
                MatrixOS::LED::SetColor(origin + Point(track, 0), Color::Black);
                MatrixOS::LED::SetColor(origin + Point(track, 1), Color::Black);
                MatrixOS::LED::SetColor(origin + Point(track, 2), Color::Black);
            }
            else
            {
                Color trackColor = sequencer->meta.tracks[track].color;

                // Mute (Y=0)
                {
                    Color color;
                    if(anySolo)
                    {
                        // When solo is active, show soloed tracks bright, others dim
                        bool soloed = sequencer->sequence.GetSolo(track);
                        color = trackColor.DimIfNot(soloed);
                    }
                    else
                    {
                        bool muted = sequencer->sequence.GetMute(track);
                        color = trackColor.DimIfNot(!muted);
                    }
                    MatrixOS::LED::SetColor(origin + Point(track, 0), color);
                }

                // Solo (Y=1)
                {
                    bool solo = sequencer->sequence.GetSolo(track);
                    Color color = Color(0x0080FF).DimIfNot(solo);
                    MatrixOS::LED::SetColor(origin + Point(track, 1), color);
                }

                // Record (Y=2)
                {
                    bool record = sequencer->sequence.GetRecord(track);
                    Color color = Color(0xFF0000).DimIfNot(record);
                    MatrixOS::LED::SetColor(origin + Point(track, 2), color);
                }
            }
        }

        return true;
    }
};
