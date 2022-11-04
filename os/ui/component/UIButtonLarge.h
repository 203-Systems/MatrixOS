#pragma once
#include "UIButton.h"

class UIButtonLarge : public UIButton {
 public:
  std::function<Color()> color_func;
  Dimension dimension;

  UIButtonLarge(string name, Color color, Dimension dimension, std::function<void()> callback = nullptr,
                std::function<void()> hold_callback = nullptr)
      : UIButton(name, color, callback, hold_callback) {
    this->dimension = dimension;
  }

  virtual Dimension GetSize() { return dimension; }

  virtual bool Render(Point origin) {
    for (uint16_t x = 0; x < dimension.x; x++)
    {
      for (uint16_t y = 0; y < dimension.y; y++)
      { MatrixOS::LED::SetColor(origin + Point(x, y), GetColor()); }
    }
    return true;
  }
};