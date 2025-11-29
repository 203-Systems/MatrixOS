#include "TrackSelector.h"

TrackSelector::TrackSelector(Sequencer* sequence, bool textScroll)
{
    this->sequencer = sequence;
    this->textScroll = textScroll;

    width = sequencer->sequence.GetTrackCount();
}

Dimension TrackSelector::GetSize() { return Dimension(8, 1); }

void TrackSelector::OnChange(std::function<void(uint8_t)> callback)
{
    changeCallback = callback;
}

bool TrackSelector::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    if(keyInfo->State() == PRESSED)
    {
        if(xy.x != sequencer->track)
        {
            sequencer->track = xy.x;
            sequencer->ClearState();
            if (changeCallback != nullptr) {
                (changeCallback)(xy.x);
            }
        }
        else
        {
            sequencer->trackSelected = true;
            if(sequencer->ClearActive())
            {
                sequencer->sequence.ClearAllStepsInClip(sequencer->track, sequencer->sequence.GetPosition(sequencer->track).clip);
            }
        }
    }
    else if(keyInfo->State() == HOLD)
    {
        if(textScroll)
        {
            Color color = sequencer->meta.tracks[xy.x].color;
            MatrixOS::UIUtility::TextScroll("Track " + std::to_string(xy.x + 1), color);
        }
    }
    else if(keyInfo->State() == RELEASED && xy.x == sequencer->track)
    {
        sequencer->trackSelected = false;
    }
    return true;
}

bool TrackSelector::Render(Point origin)
{
    if(sequencer->ClearActive())
    {
        MatrixOS::LED::SetColor(origin + Point(sequencer->track, 0), sequencer->meta.tracks[sequencer->track].color);
    }
    else
    {
        for(uint8_t i = 0; i < width; i++)
        {
            Color color = sequencer->meta.tracks[i].color;
            if(i == sequencer->track)
            {
                MatrixOS::LED::SetColor(origin + Point(i, 0), Color::Crossfade(color, Color::White, Fract16(0xB000)));
            }
            else if(sequencer->sequence.GetEnabled(i) == false)
            {

                MatrixOS::LED::SetColor(origin + Point(i, 0), color.Dim());
            }
            else
            {
                uint32_t timeSinceLastEvent = MatrixOS::SYS::Millis() - sequencer->sequence.GetLastEventTime(i);

                const uint16_t fadeLengthMs = 200;
                if(timeSinceLastEvent < fadeLengthMs)
                {
                    float ratio = (float)timeSinceLastEvent / fadeLengthMs;
                    color = Color::Crossfade(Color::White, color, Fract16(ratio * UINT16_MAX));
                }

                MatrixOS::LED::SetColor(origin + Point(i, 0), color);
            }
        }
    }

    return true;
}
