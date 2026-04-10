#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

class SequenceScaleModifier : public UIComponent {
public:
  UICallback<uint16_t()> getScale;
  UICallback<void(uint16_t)> changeCallback;
  Color color;
  Color rootColor;

  SequenceScaleModifier(Color color, Color rootColor);

  template <typename F> void SetScaleFunc(F&& f) {
    this->getScale = UICallback<uint16_t()>(static_cast<F&&>(f));
  }
  template <typename F> void OnChange(F&& f) {
    this->changeCallback = UICallback<void(uint16_t)>(static_cast<F&&>(f));
  }
  virtual void OnChangeCallback(uint16_t newScale);

  virtual Color GetColor();
  virtual Dimension GetSize();
  virtual bool Render(Point origin);
  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo);
};
