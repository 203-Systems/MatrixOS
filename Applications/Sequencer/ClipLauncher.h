#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

#include "Sequencer.h"

class ClipLauncher : public UIComponent {
    Sequencer* sequencer;

    public:
    ClipLauncher(Sequencer* sequencer);

    Dimension GetSize();

    virtual bool IsEnabled() override;
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);

    virtual bool Render(Point origin);
};
