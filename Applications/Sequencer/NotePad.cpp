#include "NotePad.h"

NotePad::NotePad(Sequencer* sequencer, vector<uint8_t>* noteSelected)
{
    this->sequencer = sequencer;
    this->noteSelected = noteSelected;
    width = sequencer->sequence.GetTrackCount();
}

void NotePad::OnSelect(std::function<void(uint8_t)> callback)
{
    selectCallback = callback;
}

Dimension NotePad::GetSize()
{
    return Dimension(8, 4);
}

bool NotePad::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    return false;
}

bool NotePad::Render(Point origin)
{

}
