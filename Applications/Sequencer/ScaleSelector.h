#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

class ScaleSelector : public UIComponent {
public:
  ScaleSelector(Color color);

  void SetScaleFunc(UICallback<uint16_t()> func);
  void OnChange(UICallback<void(uint16_t)> callback);

  Color GetColor();
  Dimension GetSize() override;
  bool Render(Point origin) override;
  bool KeyEvent(Point xy, KeypadInfo* keypadInfo) override;

private:
  Color color;
  UICallback<uint16_t()> getScale;
  UICallback<void(uint16_t)> onChange;
};
