#pragma once
#include "UI/UI.h"

class UIKeyboardKey : public UIComponent {
 public:
  Color color;
  KeyboardKeycode keycode;
  bool active = false;

  UIKeyboardKey(Color color, KeyboardKeycode keycode) {
    this->color = color;
    this->keycode = keycode;
  }
  
  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return Dimension(1, 1); }

  
  virtual bool Render(Point origin) {
    MatrixOS::LED::SetColor(origin, active ? Color(0xFFFFFF) : GetColor());
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->State() == PRESSED)
    {
      active = true;
      MatrixOS::HID::Keyboard::Press(keycode); 
    }
    else if (keyInfo->State() == RELEASED)
    {
      active = false;
      MatrixOS::HID::Keyboard::Release(keycode);
    }
    return true;
  }
};
