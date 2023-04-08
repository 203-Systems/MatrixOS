#pragma once
#include "../UIInterfaces.h"

class UIButton : public UIComponent {
 public:
  string name;
  Color color;
  std::function<void()> callback;
  std::function<void()> hold_callback;

  UIButton(string name, Color color, std::function<void()> callback = nullptr,
           std::function<void()> hold_callback = nullptr) {
    this->name = name;
    this->color = color;
    this->callback = callback;
    this->hold_callback = hold_callback;
  }

  virtual string GetName() { return name; }
  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return Dimension(1, 1); }

  virtual bool Callback() {
    if (callback != nullptr)
    {
      callback();
      return true;
    }
    return false;
  }
  virtual bool HoldCallback() {
    if (hold_callback)
    {
      hold_callback();
      return true;
    }
    return false;
  }
  virtual bool Render(Point origin) {
    MatrixOS::LED::SetColor(origin, GetColor());
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->state == RELEASED && keyInfo->hold == false)
    {
      if (Callback())
      {
        MLOGD("UI Button", "Key Event Callback");
        MatrixOS::KEYPAD::Clear();
        return true;
      }
    }
    else if (keyInfo->state == HOLD)
    {
      if (HoldCallback())
      {
        MatrixOS::KEYPAD::Clear();
        return true;
      }
      else
      {
        MatrixOS::UIInterface::TextScroll(GetName(), GetColor());
        return true;
      }
    }
    return false;
  }
};
