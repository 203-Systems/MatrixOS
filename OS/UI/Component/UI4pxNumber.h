#pragma once
#include <cmath>
#include "UIComponent.h"
#include "../Data/4pxNumber.h"
#include "../UIUtilities.h"

// TODO add negative support?
class UI4pxNumber : public UIComponent {
public:
  string name;
  uint8_t digits;
  int32_t* valuePtr;
  UICallback<int32_t()> getValueFunc;
  uint8_t spacing;
  Color color;
  Color alternativeColor;
  UICallback<Color(uint16_t digit)> colorFunc;

  UI4pxNumber() {
    this->color = Color(0);
    this->alternativeColor = Color::White;
    this->digits = 0;
    this->valuePtr = nullptr;
    this->spacing = 0;
  }

  virtual Dimension GetSize() {
    return Dimension(digits * 3 + (digits - 1) * (digits > 0) * spacing, 4);
  }
  virtual Color GetColor(uint16_t digit) {
    if (colorFunc)
    {
      return colorFunc(digit);
    }
    else if (digit == UINT16_MAX)
    {
      return color;
    }
    else
    {
      return digit % 2 ? alternativeColor : color;
    }
  };

  void SetName(string name) {
    this->name = name;
  }
  void SetColor(Color color) {
    this->color = color;
  }
  void SetAlternativeColor(Color alternativeColor) {
    this->alternativeColor = alternativeColor;
  }
  void SetDigits(uint8_t digits) {
    this->digits = digits;
  }

  void SetValuePointer(int32_t* valuePtr) {
    this->valuePtr = valuePtr;
    this->getValueFunc.Reset();
  }

  template <typename F> void SetValueFunc(F&& f) {
    this->getValueFunc = UICallback<int32_t()>(static_cast<F&&>(f));
    this->valuePtr = nullptr;
  }

  int32_t GetValue() {
    if (getValueFunc)
    {
      return getValueFunc();
    }
    return (valuePtr != nullptr) ? *valuePtr : 0;
  }

  void SetValue(int32_t value) {
    // SetValue should not work if getValueFunc is set
    if (!getValueFunc && valuePtr != nullptr)
    {
      *valuePtr = value;
    }
  }

  void SetSpacing(uint8_t spacing) {
    this->spacing = spacing;
  }
  template <typename F> void SetColorFunc(F&& f) {
    this->colorFunc = UICallback<Color(uint16_t digit)>(static_cast<F&&>(f));
  }
  void Render4pxNumber(Point origin, Color color, uint8_t value) {
    // MLOGD("4PX", "Num: %d, render at %d-%d", value, origin.x, origin.y);
    if (value < 11 /*&& value >= 0*/)
    {
      for (int8_t x = 0; x < 3; x++)
      {
        for (int8_t y = 0; y < 4; y++)
        {
          MatrixOS::LED::SetColor(origin + Point(x, 3 - y), bitRead(number4px[value][x], y) ? color : Color(0));
        }
      }
    }
  }

  virtual bool Render(Point origin) {
    int32_t value = GetValue(); // Use GetValue() to support both pointer and function
    uint8_t sigFigure = int(log10(value) + 1);
    Point renderOrigin = origin;
    // MLOGD("4PX", "Render %d, sigfig %d", value, sigFigure);
    for (int8_t digit = digits - 1; digit >= 0; digit--)
    {
      if (digit < sigFigure || digit == 0)
      {
        Render4pxNumber(renderOrigin, GetColor(digit), (int)(value / std::pow(10, digit)) % 10);
      }
      else
      {
        Render4pxNumber(renderOrigin, Color(0), 10);
      }

      renderOrigin = renderOrigin + Point(3 + spacing, 0);
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo) {
    if (name.empty())
    {
      return false;
    }

    if (keypadInfo->state == KeypadState::Hold)
    {
      MatrixOS::UIUtility::TextScroll(name, GetColor(UINT16_MAX));
    }
    return true;
  }
};