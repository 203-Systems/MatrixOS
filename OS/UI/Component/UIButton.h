#pragma once
#include "UIComponent.h"
#include "../UIUtilities.h"

class UIButton : public UIComponent {
 public:
  string name;
  Color color = Color(0);
  std::unique_ptr<std::function<void()>> pressCallback;
  std::unique_ptr<std::function<void()>> holdCallback;
  std::unique_ptr<std::function<Color()>> colorFunc;
  Dimension dimension = Dimension(1, 1);

  UIButton() {
    this->name = "";
    this->color = Color(0xFFFFFF);
    this->dimension = Dimension(1, 1);
    this->pressCallback = nullptr;
    this->holdCallback = nullptr;
    this->colorFunc = nullptr;
  }
  
  virtual string GetName() { return name; }
  void SetName(string name) { this->name = name; }

  virtual Color GetColor() { 
    if (colorFunc)
    {
      return (*colorFunc)();
    }
    return color;
  }
  void SetColor(Color color) { this->color = color; }
  void SetColorFunc(std::function<Color()> colorFunc) {
    this->colorFunc = std::make_unique<std::function<Color()>>(colorFunc);
  }

  virtual Dimension GetSize() { return dimension; }
  void SetSize(Dimension dimension) { this->dimension = dimension; }

  virtual bool PressCallback() {
    if (pressCallback)
    {
      (*pressCallback)();
      return true;
    }
    return false;
  }

  void OnPress(std::function<void()> pressCallback) { this->pressCallback = std::make_unique<std::function<void()>>(pressCallback); }

  virtual bool HoldCallback() {
    if (holdCallback)
    {
      (*holdCallback)();
      return true;
    }
    return false;
  }
  void OnHold(std::function<void()> holdCallback) { this->holdCallback = std::make_unique<std::function<void()>>(holdCallback); }


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
    if (keyInfo->State() == RELEASED && keyInfo->Hold() == false)
    {
      if (PressCallback())
      {
        MLOGD("UI Button", "Key Event Callback");
        MatrixOS::KeyPad::Clear();
        return true;
      }
    }
    else if (keyInfo->State() == HOLD)
    {
      if (HoldCallback())
      {
        MatrixOS::KeyPad::Clear();
        return true;
      }
      else if(GetName() != "")
      {
        MatrixOS::UIUtility::TextScroll(GetName(), GetColor());
        return true;
      }
    }
    return false;
  }

  virtual ~UIButton(){};
};