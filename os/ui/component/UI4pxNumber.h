#pragma once
#include <cmath>
#include "../data/4pxNumber.h"
#include "UIComponent.h"

// TODO add negative support?
class UI4pxNumber : public UIComponent {
 public:
  string name;
  uint8_t digits;
  int32_t* valuePtr;
  uint8_t spacing;
  Color color;
  Color alternative_color;
  std::unique_ptr<std::function<Color(uint16_t digit)>> color_func;

  UI4pxNumber() {
    this->color = Color(0);
    this->alternative_color = Color(0xFFFFFF);
    this->digits = 0;
    this->valuePtr = nullptr;
    this->spacing = 0;
    this->color_func = nullptr;
  }

  virtual Dimension GetSize() { return Dimension(digits * 3 + (digits - 1) * (digits > 0) * spacing, 4); }
  virtual Color GetColor(uint16_t digit) { 
    if (color_func)
    {
      return (*color_func)(digit);
    }
    else if (digit == UINT16_MAX)
    {
      return color;
    }
    else
    {
      return digit % 2 ? alternative_color : color;
    }
  };

  void SetName(string name) { this->name = name; }
  void SetColor(Color color) { this->color = color; }
  void SetAlternativeColor(Color alternative_color) { this->alternative_color = alternative_color; }
  void SetDigits(uint8_t digits) { this->digits = digits; }
  void SetValuePointer(int32_t* valuePtr) { this->valuePtr = valuePtr; }
  void SetSpacing(uint8_t spacing) { this->spacing = spacing; }
  void SetColorFunc(std::function<Color(uint16_t digit)> color_func) { this->color_func = std::make_unique<std::function<Color(uint16_t digit)>>(color_func); } 
  void Render4pxNumber(Point origin, Color color, uint8_t value) {
    // MLOGD("4PX", "Num: %d, render at %d-%d", value, origin.x, origin.y);
    if (value < 11 /*&& value >= 0*/)
    {
      for (int8_t x = 0; x < 3; x++)
      {
        for (int8_t y = 0; y < 4; y++)
        { MatrixOS::LED::SetColor(origin + Point(x, 3 - y), bitRead(number4px[value][x], y) ? color : Color(0)); }
      }
    }
  }

  virtual bool Render(Point origin) {
    uint8_t sig_figure = int(log10(*valuePtr) + 1);
    Point render_origin = origin;
    // MLOGD("4PX", "Render %d, sigfig %d", *valuePtr, sig_figure);
    for (int8_t digit = digits - 1; digit >= 0; digit--)
    {
      if (digit < sig_figure || digit == 0)
      { Render4pxNumber(render_origin, GetColor(digit), (int)(*valuePtr / std::pow(10, digit)) % 10); }
      else
      {
        Render4pxNumber(render_origin, Color(0), 10);
      }

      render_origin = render_origin + Point(3 + spacing, 0);
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->state == HOLD)
    {
      MatrixOS::UIUtility::TextScroll(name, GetColor(UINT16_MAX));
    }
    return true;
  }
};