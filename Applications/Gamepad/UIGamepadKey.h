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
      MatrixOS::HID::Gamepad::Press(keycode);
    }
    else if (keypadInfo->state == KeypadState::Released)
    {
      active = false;
      MatrixOS::HID::Gamepad::Release(keycode);
    }
    return true;
  }
};
