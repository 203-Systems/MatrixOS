#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

#include "Sequencer.h"

class PatternSelector : public UIComponent {
    Sequencer* sequencer;
    std::function<void(uint8_t)> changeCallback;
    bool lengthAdjustmentMode = false;

    public:
    PatternSelector(Sequencer* sequencer);

    virtual bool IsEnabled();
    Dimension GetSize();
    void OnChange(std::function<void(uint8_t)> callback);
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool Render(Point origin);
};
