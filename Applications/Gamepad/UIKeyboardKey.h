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

  virtual Color GetColor() {
    return color;
  }
  virtual Dimension GetSize() {
    return Dimension(1, 1);
  }

  virtual bool Render(Point origin) {
    MatrixOS::LED::SetColor(origin, active ? Color::White : GetColor());
    return true;
  }

  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo) {
    if (keypadInfo->state == KeypadState::Pressed)
    {
      active = true;
      MatrixOS::HID::Keyboard::Press(keycode);
    }
    else if (keypadInfo->state == KeypadState::Released)
    {
      active = false;
      MatrixOS::HID::Keyboard::Release(keycode);
    }
    return true;
  }
};
