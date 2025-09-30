#pragma once
#include "UI/UI.h"

class UIGamepadKey : public UIComponent {
 public:
  Color color;
  uint8_t keycode;
  bool active = false;

  UIGamepadKey(Color color, uint8_t keycode) {
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
      MatrixOS::HID::Gamepad::Press(keycode); 
    }
    else if (keyInfo->State() == RELEASED)
    {
      active = false;
      MatrixOS::HID::Gamepad::Release(keycode); 
    }
    return true;
  }
};
