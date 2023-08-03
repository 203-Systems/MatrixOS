#pragma once
#include "ui/UI.h"
#include <functional>

// TODO add negative support?
class UI4pxNumberWithColorFunc : public UI4pxNumber {
 public:
  std::function<Color()> color_func;
  UI4pxNumberWithColorFunc(std::function<Color()> color_func, uint8_t digits, int32_t* value,
                           Color alternative_color = Color(0), uint8_t spacing = 0)
      : UI4pxNumber(color_func(), digits, value, alternative_color, spacing) {
    this->color_func = color_func;
  }

  virtual Color GetColor() { return color_func(); };
  virtual Color GetAlternativeColor() { return alternative_color ? alternative_color : color_func(); };
};
