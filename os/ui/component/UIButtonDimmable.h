#pragma once
#include "UIButton.h"

class UIButtonDimmable : public UIButton {
 public:
  std::function<bool()> dim_func;  // If this returns false, then dim button

  UIButtonDimmable(string name, Color color, std::function<bool()> dim_func, std::function<void()> callback = nullptr,
                   std::function<void()> hold_callback = nullptr)
      : UIButton(name, color, callback, hold_callback) {
    this->dim_func = dim_func;
  }

  virtual Color GetColor() { return color.ToLowBrightness(dim_func()); }
};
