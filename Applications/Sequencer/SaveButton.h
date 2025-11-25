#pragma once

#include "UI/UI.h"
#include "Sequencer.h"
#include <functional>

// A preconfigured save UI component for the Sequencer menu
class SaveButton : public UIComponent
{
public:
    explicit SaveButton(Sequencer* sequencer);
    Dimension GetSize() override;
    bool Render(Point origin) override;
    bool KeyEvent(Point xy, KeyInfo* keyInfo) override;
private:
    Sequencer* sequencer;
};
