#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>
#include <set>

#include "Sequencer.h"

class PatternPad : public UIComponent {
    Sequencer* sequencer;
    uint8_t width = 8;

    public:
    PatternPad(Sequencer* sequencer);

    Dimension GetSize();
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool Render(Point origin);
};
