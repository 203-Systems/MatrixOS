#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequence.h"
#include "SequenceMeta.h"

class TrackSelector : public UIComponent {
    Sequence* sequence;
    SequenceMeta* sequenceMeta;
    uint8_t* track;
    uint8_t width = 8;

    public:
    TrackSelector(Sequence* sequence, SequenceMeta* sequenceMeta, uint8_t* track)
    {
        this->sequence = sequence;
        this->sequenceMeta = sequenceMeta;
        this->track = track;
        width = sequence->GetTrackCount();
    }

    Dimension GetSize() { return Dimension(8, 1); }
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) 
    { 
        if(keyInfo->State() == HOLD)
        {
            Color color = sequenceMeta->tracks[xy.x].color;
            MatrixOS::UIUtility::TextScroll("Track " + std::to_string(xy.x + 1), color);
        }
        else if(keyInfo->State() == RELEASED && keyInfo->hold == false)
        {
            *track = xy.x;
        }
        return true;
    }

    virtual bool Render(Point origin) 
    { 
        {
            for(uint8_t i = 0; i < width; i++)
            {
                if(i == *track)
                {
                    MatrixOS::LED::SetColor(origin + Point(i, 0), Color::White);
                }
                else if(sequence->GetEnabled(i) == false)
                {
                    Color color = sequenceMeta->tracks[i].color;
                    MatrixOS::LED::SetColor(origin + Point(i, 0), color.Dim());
                }
                else
                {
                    Color color = sequenceMeta->tracks[i].color;

                    // TODO: Add white fading based on lastEvent 
                    MatrixOS::LED::SetColor(origin + Point(i, 0), color);
                }
            }
        }
        return true;
    }
};