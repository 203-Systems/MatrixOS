#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

class ScaleSelector : public UIComponent {
 public:
  ScaleSelector(Color color);

  void SetScaleFunc(std::function<uint16_t()> func);
  void OnChange(std::function<void(uint16_t)> callback);

  Color GetColor();
  Dimension GetSize() override;
  bool Render(Point origin) override;
  bool KeyEvent(Point xy, KeyInfo* keyInfo) override;

 private:
  Color color;
  std::function<uint16_t()> getScale;
  std::function<void(uint16_t)> onChange;
};
