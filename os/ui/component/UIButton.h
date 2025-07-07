#pragma once
#include "UIComponent.h"
#include "../UIUtilities.h"

class UIButton : public UIComponent {
 public:
  string name;
  Color color = Color(0);
  std::function<void()> press_callback;
  std::function<void()> hold_callback;
  std::function<Color()> color_func;
  Dimension dimension = Dimension(1, 1);

  UIButton() {}
  
  virtual string GetName() { return name; }
  void SetName(string name) { this->name = name; }

  virtual Color GetColor() { 
    if (color_func)
    {
      return color_func();
    }
    return color;
  }
  void SetColor(Color color) { this->color = color; }
  void SetColorFunc(std::function<Color()> color_func) {
    this->color = Color(0);
    this->color_func = color_func;
  }

  virtual Dimension GetSize() { return dimension; }
  void SetSize(Dimension dimension) { this->dimension = dimension; }

  virtual bool PressCallback() {
    if (press_callback != nullptr)
    {
      press_callback();
      return true;
    }
    return false;
  }

  void OnPress(std::function<void()> press_callback) { this->press_callback = press_callback; }

  virtual bool HoldCallback() {
    if (hold_callback)
    {
      hold_callback();
      return true;
    }
    return false;
  }
  void OnHold(std::function<void()> hold_callback) { this->hold_callback = hold_callback; }


  virtual bool Render(Point origin) {
    Dimension dimension = GetSize();
    Color color = GetColor();
    for (uint16_t x = 0; x < dimension.x; x++)
    {
      for (uint16_t y = 0; y < dimension.y; y++)
      {
        MatrixOS::LED::SetColor(origin + Point(x, y), color);
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->state == RELEASED && keyInfo->hold == false)
    {
      if (PressCallback())
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
        MatrixOS::UIUtility::TextScroll(GetName(), GetColor());
        return true;
      }
    }
    return false;
  }

  virtual ~UIButton(){};
};