#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

#include "Sequencer.h"

class PatternSelector : public UIComponent {
    Sequencer* sequencer;
    bool lengthAdjustmentMode = false;

    public:
    PatternSelector(Sequencer* sequencer);

    virtual bool IsEnabled();
    Dimension GetSize();
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool Render(Point origin);
};
