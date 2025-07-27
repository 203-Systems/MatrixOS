#pragma once
#include "ui/UI.h"

class UIDPad : public UIComponent {
 public:
  Color color;
  GamepadDPadDirection lastDirection = GAMEPAD_DPAD_CENTERED;

  UIDPad(Color color) {
    this->color = color;
  }
  
  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return Dimension(3, 3); }

  Point DirectionToPoint(GamepadDPadDirection direction)
  {
    switch(direction)
    {
      case GAMEPAD_DPAD_UP_LEFT:
        return Point(0,0);
      case GAMEPAD_DPAD_UP:
        return Point(1,0);
      case GAMEPAD_DPAD_UP_RIGHT:
        return Point(2,0);
      case GAMEPAD_DPAD_LEFT:
        return Point(0,1);
      case GAMEPAD_DPAD_CENTERED:
        return Point(1,1);
      case GAMEPAD_DPAD_RIGHT:
        return Point(2,1);
      case GAMEPAD_DPAD_DOWN_LEFT:
        return Point(0,2);
      case GAMEPAD_DPAD_DOWN:
        return Point(1,2);
      case GAMEPAD_DPAD_DOWN_RIGHT:
        return Point(2,2);
    }
    return Point(1,1);
  }

  GamepadDPadDirection PointToDirection(Point p)
  {
    if(p == Point(0,0))
    {
      return GAMEPAD_DPAD_UP_LEFT;
    }
    else if(p == Point(1,0))
    {
      return GAMEPAD_DPAD_UP;
    }
    else if(p == Point(2,0))
    {
      return GAMEPAD_DPAD_UP_RIGHT;
    }
    else if(p == Point(0,1))
    {
      return GAMEPAD_DPAD_LEFT;
    }
    else if(p == Point(1,1))
    {
      return GAMEPAD_DPAD_CENTERED;
    }
    else if(p == Point(2,1))
    {
      return GAMEPAD_DPAD_RIGHT;
    }
    else if(p == Point(0,2))
    {
      return GAMEPAD_DPAD_DOWN_LEFT;
    }
    else if(p == Point(1,2))
    {
      return GAMEPAD_DPAD_DOWN;
    }
    else if(p == Point(2,2))
    {
      return GAMEPAD_DPAD_DOWN_RIGHT;
    }
    return GAMEPAD_DPAD_CENTERED;
  }

  virtual bool Render(Point origin) {
    for(int x = 0; x < 3; x++)
    {
      for(int y = 0; y < 3; y++)
      {
        MatrixOS::LED::SetColor(origin + Point(x,y), Point(x,y) == DirectionToPoint(lastDirection) ? Color(0xFFFFFF) : GetColor());
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    GamepadDPadDirection direction = GAMEPAD_DPAD_CENTERED;
    if (keyInfo->state == PRESSED)
    {
      direction = PointToDirection(xy);
    }
    else if (keyInfo->state == RELEASED)
    {
      if(xy == DirectionToPoint(lastDirection))
      {
        direction = GAMEPAD_DPAD_CENTERED;
      }
      else
      {
        return true;
      }
    }
    else
    { return true; }

    MatrixOS::HID::Gamepad::DPad(direction);

    lastDirection = direction;
    
    return true;
  }
};
