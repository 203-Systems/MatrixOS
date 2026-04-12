#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class MixerControl : public UIComponent {
  Sequencer* sequencer;

public:
  MixerControl(Sequencer* sequencer);

  Dimension GetSize() override;
  virtual bool IsEnabled() override;
  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo) override;
  virtual bool Render(Point origin) override;
};
