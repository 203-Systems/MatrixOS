#pragma once
#include "UIComponent.h"
#include "../UIUtilities.h"

class UIButton : public UIComponent {
public:
  string name;
  Color color = Color(0);
  UICallback<void()> pressCallback;
  UICallback<void()> holdCallback;
  UICallback<Color()> colorFunc;
  Dimension dimension = Dimension(1, 1);

  UIButton() {
    this->name = "";
    this->color = Color::White;
    this->dimension = Dimension(1, 1);
  }

  virtual string GetName() {
    return name;
  }
  void SetName(string name) {
    this->name = name;
  }

  virtual Color GetColor() {
    if (colorFunc)
    {
      return colorFunc();
    }
    return color;
  }
  void SetColor(Color color) {
    this->color = color;
  }
  template <typename F> void SetColorFunc(F&& f) {
    this->colorFunc = UICallback<Color()>(static_cast<F&&>(f));
  }

  virtual Dimension GetSize() {
    return dimension;
  }
  void SetSize(Dimension dimension) {
    this->dimension = dimension;
  }

  virtual bool PressCallback() {
    if (pressCallback)
    {
      pressCallback();
      return true;
    }
    return false;
  }

  template <typename F> void OnPress(F&& f) {
    this->pressCallback = UICallback<void()>(static_cast<F&&>(f));
  }

  virtual bool HoldCallback() {
    if (holdCallback)
    {
      holdCallback();
      return true;
    }
    return false;
  }
  template <typename F> void OnHold(F&& f) {
    this->holdCallback = UICallback<void()>(static_cast<F&&>(f));
  }

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

  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo) {
    if (keypadInfo->state == KeypadState::Released && keypadInfo->hold == false)
    {
      if (PressCallback())
      {
        MLOGD("UI Button", "Key Event Callback");
        MatrixOS::Input::ClearInputBuffer();
        return true;
      }
    }
    else if (keypadInfo->state == KeypadState::Hold)
    {
      if (HoldCallback())
      {
        MatrixOS::Input::ClearInputBuffer();
        return true;
      }
      else if (!GetName().empty())
      {
        MatrixOS::UIUtility::TextScroll(GetName(), GetColor());
        return true;
      }
    }
    return false;
  }

  virtual ~UIButton() {};
};