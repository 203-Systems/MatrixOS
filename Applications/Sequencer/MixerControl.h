#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class MixerControl : public UIComponent {
    Sequencer* sequencer;

    public:
    MixerControl(Sequencer* sequencer);

    Dimension GetSize();
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool Render(Point origin);
};
