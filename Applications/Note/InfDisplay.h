#pragma once
#include "MatrixOS.h"
#include "UI/UI.h"


class InfDisplay : public UIComponent {
 public:
  Color color;

  InfDisplay(Color color) {
    this->color = color;
  }

  virtual Dimension GetSize() { return Dimension(8, 4); }

  virtual bool Render(Point origin) {
    Dimension size = GetSize();
    for(int8_t y = 0; y < size.y; y++)
    {
      for(int8_t x = 0; x < size.x; x++)
      {
        MatrixOS::LED::SetColor(origin + Point(x, y), Color(0));
      }
    }

    // I
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 1), color);
    MatrixOS::LED::SetColor(origin + Point(1, 2), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);

    // N
    MatrixOS::LED::SetColor(origin + Point(2, 1), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(origin + Point(2, 2), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(origin + Point(2, 3), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color(0xFFFFFF));

    // F
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);


    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    return false;// passthrough keypress
  }
};