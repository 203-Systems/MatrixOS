#pragma once
#include "UIButtonLarge.h"

class UIButtonLargeWithColorFunc : public UIButtonLarge {
 public:
  std::function<Color()> color_func;

  UIButtonLargeWithColorFunc(string name, std::function<Color()> color_func, Dimension dimension, std::function<void()> callback = nullptr,
                std::function<void()> hold_callback = nullptr)
      : UIButtonLarge(name, Color(0xFFFFFF), dimension, callback, hold_callback) {
    this->color_func = color_func;
  }

  virtual Color GetColor() { return color_func(); }
};