#pragma once
#include "UIComponent.h"
#include "../UIUtilities.h"

class UIButton : public UIComponent {
 public:
  string name;
  Color color = Color(0);
  std::unique_ptr<std::function<void()>> press_callback;
  std::unique_ptr<std::function<void()>> hold_callback;
  std::unique_ptr<std::function<Color()>> color_func;
  Dimension dimension = Dimension(1, 1);

  UIButton() {
    this->name = "";
    this->color = Color(0xFFFFFF);
    this->dimension = Dimension(1, 1);
    this->press_callback = nullptr;
    this->hold_callback = nullptr;
    this->color_func = nullptr;
  }
  
  virtual string GetName() { return name; }
  void SetName(string name) { this->name = name; }

  virtual Color GetColor() { 
    if (color_func)
    {
      return (*color_func)();
    }
    return color;
  }
  void SetColor(Color color) { this->color = color; }
  void SetColorFunc(std::function<Color()> color_func) {
    this->color_func = std::make_unique<std::function<Color()>>(color_func);
  }

  virtual Dimension GetSize() { return dimension; }
  void SetSize(Dimension dimension) { this->dimension = dimension; }

  virtual bool PressCallback() {
    if (press_callback)
    {
      (*press_callback)();
      return true;
    }
    return false;
  }

  void OnPress(std::function<void()> press_callback) { this->press_callback = std::make_unique<std::function<void()>>(press_callback); }

  virtual bool HoldCallback() {
    if (hold_callback)
    {
      (*hold_callback)();
      return true;
    }
    return false;
  }
  void OnHold(std::function<void()> hold_callback) { this->hold_callback = std::make_unique<std::function<void()>>(hold_callback); }


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
        MatrixOS::KeyPad::Clear();
        return true;
      }
    }
    else if (keyInfo->state == HOLD)
    {
      if (HoldCallback())
      {
        MatrixOS::KeyPad::Clear();
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