#pragma once
#include "ui/UI.h"

enum GamepadAxis {
  GAMEPAD_AXIS_LEFT_X = 0,
  GAMEPAD_AXIS_LEFT_Y = 1,
  GAMEPAD_AXIS_RIGHT_X = 2,
  GAMEPAD_AXIS_RIGHT_Y = 3,
  GAMEPAD_AXIS_LEFT_TRIGGER = 4,
  GAMEPAD_AXIS_RIGHT_TRIGGER = 5,
};

class UIGamepadAxis : public UIComponent {
 public:
  Color color;
  GamepadAxis axis;
  int16_t range = 127;
  Fract16 value = 0;

  UIGamepadAxis(Color color, GamepadAxis axis, int16_t range) {
    this->color = color;
    this->axis = axis;
    this->range = range;
  }
  
  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return Dimension(1,1); }

  virtual bool Render(Point origin) {
    if (value)
    {
      MatrixOS::LED::SetColor(origin, Color(0xFFFFFF).Scale(value.to8bits()));
    }
    else
    {
      MatrixOS::LED::SetColor(origin, color);
    }
    
    return true;
  }

  void SetAxis(GamepadAxis axis, int8_t value)
  {
    switch(axis)
    {
      case GAMEPAD_AXIS_LEFT_X:
        MatrixOS::HID::Gamepad::XAxis(value);
        break;
      case GAMEPAD_AXIS_LEFT_Y:
        MatrixOS::HID::Gamepad::YAxis(value);
        break;
      case GAMEPAD_AXIS_RIGHT_X:
        MatrixOS::HID::Gamepad::RXAxis(value);
        break;
      case GAMEPAD_AXIS_RIGHT_Y:
        MatrixOS::HID::Gamepad::RYAxis(value);
        break;
      case GAMEPAD_AXIS_LEFT_TRIGGER:
        MatrixOS::HID::Gamepad::ZAxis(value);
        break;
      case GAMEPAD_AXIS_RIGHT_TRIGGER:
        MatrixOS::HID::Gamepad::RZAxis(value);
        break;
    }
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->state == PRESSED || keyInfo->state == AFTERTOUCH)
    {
      value = keyInfo->velocity;

    }
    else if (keyInfo->state == RELEASED)
    {
      value = 0;
    }
  
    SetAxis(axis, range * (uint32_t)value / FRACT16_MAX);
    
    return true;
  }
};
