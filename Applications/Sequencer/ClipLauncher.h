#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

#include "Sequencer.h"

class ClipLauncher : public UIComponent {
    Sequencer* sequencer;
    std::function<void(uint8_t track, uint8_t clip)> changeCallback;

    public:
    ClipLauncher(Sequencer* sequencer);

    Dimension GetSize();

    void OnChange(std::function<void(uint8_t track, uint8_t clip)> callback);

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);

    virtual bool Render(Point origin);
};
