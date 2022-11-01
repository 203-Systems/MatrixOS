#pragma once
#include "UIComponent.h"

class UIButtonWithColorFunc : public UIButton {
 public:
  std::function<Color()> color_func;

  UIButtonWithColorFunc(string name, std::function<Color()> color_func, std::function<void()> callback = nullptr,
                        std::function<void()> hold_callback = nullptr)
      : UIButton(name, Color(0xFFFFFF), callback, hold_callback) {
    this->color_func = color_func;
  }

  virtual Color GetColor() { return color_func(); }
};