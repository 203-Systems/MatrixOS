#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

#include "Sequencer.h"

class TrackSelector : public UIComponent {
    Sequencer* sequencer;
    uint8_t width = 8;
    bool textScroll = false;
    std::function<void(uint8_t)> changeCallback;

    public:
    TrackSelector(Sequencer* sequence, bool textScroll = false);

    Dimension GetSize();
    void OnChange(std::function<void(uint8_t)> callback);
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool Render(Point origin);
};
