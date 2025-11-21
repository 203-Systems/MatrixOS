#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class TrackSelector : public UIComponent {
    Sequencer* sequencer;
    uint8_t width = 8;
    bool textScroll = false;
    std::function<void(uint8_t)> changeCallback;

    public:
    TrackSelector(Sequencer* sequence, bool textScroll = false)
    {
        this->sequencer = sequence;
        this->textScroll = textScroll;
    
        width = sequencer->sequence.GetTrackCount();
    }


    Dimension GetSize() { return Dimension(8, 1); }

    void OnChange(std::function<void(uint8_t)> callback)
    {
        changeCallback = callback;
    }

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) 
    { 
        if(keyInfo->State() == PRESSED)
        {
            if(xy.x != sequencer->track)
            {
                sequencer->track = xy.x;
                if (changeCallback != nullptr) {
                    (changeCallback)(xy.x);
                }
            }
        }
        else if(textScroll && keyInfo->State() == HOLD)
        {
            Color color = sequencer->meta.tracks[xy.x].color;
            MatrixOS::UIUtility::TextScroll("Track " + std::to_string(xy.x + 1), color);
        }
        return true;
    }

    virtual bool Render(Point origin) 
    { 
        for(uint8_t i = 0; i < width; i++)
        {
            Color color = sequencer->meta.tracks[i].color;
            if(i == sequencer->track)
            {
                MatrixOS::LED::SetColor(origin + Point(i, 0), Color::Crossfade(color, Color::White, Fract16(0xA000)));
            }
            else if(sequencer->sequence.GetEnabled(i) == false)
            {
                
                MatrixOS::LED::SetColor(origin + Point(i, 0), color.Dim());
            }
            else
            {
                uint32_t timeSinceLastEvent = MatrixOS::SYS::Millis() - sequencer->sequence.GetLastEventTime(i);

                const uint16_t fadeLengthMs = 500;
                if(timeSinceLastEvent < fadeLengthMs)
                {
                    float ratio = (float)timeSinceLastEvent / fadeLengthMs;
                    color = Color::Crossfade(Color::White, color, Fract16(ratio * UINT16_MAX));
                }

                MatrixOS::LED::SetColor(origin + Point(i, 0), color);
            }
        }

        return true;
    }
};