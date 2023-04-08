#pragma once
#include <cmath>
#include "../data/4pxNumber.h"
#include "UIComponent.h"

// TODO add negative support?
class UI4pxNumber : public UIComponent {
 public:
  Color color;
  Color alternative_color;
  uint8_t digits;
  int32_t* value;
  uint8_t spacing;

  UI4pxNumber(Color color, uint8_t digits, int32_t* value, Color alternative_color = Color(0xFFFFFF), uint8_t spacing = 0) {
    this->color = color;
    this->digits = digits;
    this->value = value;
    this->spacing = spacing;
    this->alternative_color = alternative_color;
  }

  virtual Dimension GetSize() { return Dimension(digits * 3 + (digits - 1) * (digits > 0) * spacing, 4); }
  virtual Color GetColor() { return color; };
  virtual Color GetAlternativeColor() { return alternative_color ? alternative_color : color; };

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
    uint8_t sig_figure = int(log10(*value) + 1);
    Point render_origin = origin;
    // MLOGD("4PX", "Render %d, sigfig %d", *value, sig_figure);
    for (int8_t digit = digits - 1; digit >= 0; digit--)
    {
      if (digit < sig_figure || digit == 0)
      { Render4pxNumber(render_origin, digit % 2 ? GetAlternativeColor() : GetColor(), (int)(*value / std::pow(10, digit)) % 10); }
      else
      {
        Render4pxNumber(render_origin, Color(0), 10);
      }

      render_origin = render_origin + Point(3 + spacing, 0);
    }
    return true;
  }
};