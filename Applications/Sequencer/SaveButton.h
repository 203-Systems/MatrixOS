#pragma once

#include "UI/UI.h"
#include "Sequencer.h"

// A preconfigured save UI component for the Sequencer menu
class SaveButton : public UIComponent {
public:
  explicit SaveButton(Sequencer* sequencer);
  Dimension GetSize() override;
  bool Render(Point origin) override;
  bool KeyEvent(Point xy, KeypadInfo* keypadInfo) override;
  bool IsEnabled() override;
  uint32_t LastEnableTime() const {
    return lastEnableTime;
  }

private:
  Sequencer* sequencer;
  uint32_t createTime = 0;
  uint32_t lastEnableTime = 0;
};
