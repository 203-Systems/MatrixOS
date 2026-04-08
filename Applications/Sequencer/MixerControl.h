#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class MixerControl : public UIComponent {
  Sequencer* sequencer;

public:
  MixerControl(Sequencer* sequencer);

  Dimension GetSize();
  virtual bool IsEnabled() override;
  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo);
  virtual bool Render(Point origin);
};
