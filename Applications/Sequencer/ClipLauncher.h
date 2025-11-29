#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

#include "Sequencer.h"

class ClipLauncher : public UIComponent {
    Sequencer* sequencer;

    std::pair<uint8_t, uint8_t> XY2Clip(Point xy) const;

    public:
    ClipLauncher(Sequencer* sequencer);

    Dimension GetSize();

    virtual bool IsEnabled() override;
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);

    virtual bool Render(Point origin);
};
