#pragma once
#include <cmath>
#include "UI/UI.h"

// TODO add negative support?
// Only 4x8 support right now
class UI4pxFloat : public UIComponent {
 public:
  string name;
  Color color;
  Color alternativeColor;
  float* value;

  UI4pxFloat() {
    this->name = "";
    this->color = Color(0);
    this->value = nullptr;
    this->alternativeColor = Color(0xFFFFFF);
  }

  virtual Dimension GetSize() { return Dimension(8, 4); }
  virtual Color GetColor() { return color; };
  virtual Color GetAlternativeColor() { return alternativeColor ? alternativeColor : color; };

  void SetName(string name) { this->name = name; }
  void SetColor(Color color) { this->color = color; }
  void SetValuePointer(float* value) { this->value = value; }
  void SetAlternativeColor(Color alternativeColor) { this->alternativeColor = alternativeColor; }

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

    // Clear area first
    for (int8_t x = 0; x < GetSize().x; x++)
    {
      for (int8_t y = 0; y < GetSize().y; y++)
      {
        MatrixOS::LED::SetColor(origin + Point(x, y), Color(0));
      }
    }
    
    if(*value == 0)
    {
        Render4pxNumber(origin + Point(5, 0), Color(0xFF0000), 0);
        return true;
    }
    else if(*value == std::numeric_limits<float>::infinity())
    {
        // FUL

        // F
        MatrixOS::LED::SetColor(origin + Point(0, 0), GetColor());
        MatrixOS::LED::SetColor(origin + Point(1, 0), GetColor());
        MatrixOS::LED::SetColor(origin + Point(2, 0), GetColor());
        MatrixOS::LED::SetColor(origin + Point(0, 1), GetColor());
        MatrixOS::LED::SetColor(origin + Point(0, 2), GetColor());
        MatrixOS::LED::SetColor(origin + Point(1, 2), GetColor());
        MatrixOS::LED::SetColor(origin + Point(0, 3), GetColor());

        // U
        MatrixOS::LED::SetColor(origin + Point(3, 0), GetAlternativeColor());
        MatrixOS::LED::SetColor(origin + Point(5, 0), GetAlternativeColor());
        MatrixOS::LED::SetColor(origin + Point(3, 1), GetAlternativeColor());
        MatrixOS::LED::SetColor(origin + Point(5, 1), GetAlternativeColor());
        MatrixOS::LED::SetColor(origin + Point(3, 2), GetAlternativeColor());
        MatrixOS::LED::SetColor(origin + Point(5, 2), GetAlternativeColor());
        MatrixOS::LED::SetColor(origin + Point(3, 3), GetAlternativeColor());
        MatrixOS::LED::SetColor(origin + Point(4, 3), GetAlternativeColor());
        MatrixOS::LED::SetColor(origin + Point(5, 3), GetAlternativeColor());

        // L
        MatrixOS::LED::SetColor(origin + Point(6, 0), GetColor());
        MatrixOS::LED::SetColor(origin + Point(6, 1), GetColor());
        MatrixOS::LED::SetColor(origin + Point(6, 2), GetColor());
        MatrixOS::LED::SetColor(origin + Point(6, 3), GetColor());
        MatrixOS::LED::SetColor(origin + Point(7, 3), GetColor());

    }
    else if(*value <= 0.99)
    {
      uint8_t digit1 = (uint8_t)(*value * 10) % 10;
      uint8_t digit2 = (uint8_t)(*value * 100) % 10;

      // Dot
      MatrixOS::LED::SetColor(origin + Point(1, 3), GetColor());

      Render4pxNumber(origin + Point(2, 0), GetAlternativeColor(), digit1);
      Render4pxNumber(origin + Point(5, 0), GetColor(), digit2);
    }
    else if(*value <= 9.9)
    {
      uint8_t digit1 = (uint8_t)(*value) % 10;
      uint8_t digit2 = (uint8_t)(*value * 10) % 10;

      Render4pxNumber(origin + Point(0, 0), GetColor(), digit1);
      MatrixOS::LED::SetColor(origin + Point(4, 3), GetAlternativeColor());
      Render4pxNumber(origin + Point(5, 0), GetColor(), digit2);
    }
    else if(*value >= 10 && *value <= 299)
    {
      uint8_t digit1 = (uint8_t)(*value / 100) % 10;
      uint8_t digit2 = (uint8_t)(*value / 10) % 10;
      uint8_t digit3 = (uint8_t)(*value) % 10;

      if (digit1 > 0)
      {
        Render4pxNumber(origin + Point(-1, 0), GetColor(), digit1);
      }

      Render4pxNumber(origin + Point(2, 0), GetAlternativeColor(), digit2);
      Render4pxNumber(origin + Point(5, 0), GetColor(), digit3);
    }
    else
    {
      // Err
      // E
      MatrixOS::LED::SetColor(origin + Point(0, 0), Color(0xFF0000));
      MatrixOS::LED::SetColor(origin + Point(1, 0), Color(0xFF0000));
      MatrixOS::LED::SetColor(origin + Point(2, 0), Color(0xFF0000));
      MatrixOS::LED::SetColor(origin + Point(0, 1), Color(0xFF0000));
      MatrixOS::LED::SetColor(origin + Point(1, 1), Color(0xFF0000));
      MatrixOS::LED::SetColor(origin + Point(0, 2), Color(0xFF0000));
      MatrixOS::LED::SetColor(origin + Point(0, 3), Color(0xFF0000));
      MatrixOS::LED::SetColor(origin + Point(1, 3), Color(0xFF0000));
      MatrixOS::LED::SetColor(origin + Point(2, 3), Color(0xFF0000));

      // r
      MatrixOS::LED::SetColor(origin + Point(3, 2), GetAlternativeColor());
      MatrixOS::LED::SetColor(origin + Point(4, 2), GetAlternativeColor());
      MatrixOS::LED::SetColor(origin + Point(3, 3), GetAlternativeColor());

      // r
      MatrixOS::LED::SetColor(origin + Point(5, 2), Color(0xFF0000));
      MatrixOS::LED::SetColor(origin + Point(6, 2), Color(0xFF0000));
      MatrixOS::LED::SetColor(origin + Point(5, 3), Color(0xFF0000));
    }

      return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->State() == HOLD)
    {
      MatrixOS::UIUtility::TextScroll(name, GetColor());
    }
    return true;
  }
};