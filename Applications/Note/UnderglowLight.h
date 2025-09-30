
#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

class UnderglowLight : public UIComponent {
 public:
  Dimension dimension;
  Color color;

  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return dimension; }
  void SetColor(Color newColor) { this->color = newColor; }

  bool IsUnderglow(Point pos)
  {
    return !(pos.x >= 0 && pos.x < Device::x_size && pos.y >= 0 && pos.y < Device::y_size);
  }

    virtual bool Render(Point origin) {
      uint8_t index = 0;
      for (int8_t y = 0; y < dimension.y; y++)
      {
        for (int8_t x = 0; x < dimension.x; x++)
        {
          Point globalPos = origin + Point(x, y);
          if (IsUnderglow(globalPos))
          { MatrixOS::LED::SetColor(globalPos, color); }
          index++;
        }
      }
      return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    return false;
  }

  UnderglowLight(Dimension dimension, Color color) {
    this->dimension = dimension;
    this->color = color;
  }
};
