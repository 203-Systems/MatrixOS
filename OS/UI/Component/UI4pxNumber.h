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
  std::unique_ptr<std::function<int32_t()>> getValueFunc;
  uint8_t spacing;
  Color color;
  Color alternativeColor;
  std::unique_ptr<std::function<Color(uint16_t digit)>> colorFunc;

  UI4pxNumber() {
    this->color = Color(0);
    this->alternativeColor = Color(0xFFFFFF);
    this->digits = 0;
    this->valuePtr = nullptr;
    this->getValueFunc = nullptr;
    this->spacing = 0;
    this->colorFunc = nullptr;
  }

  virtual Dimension GetSize() { return Dimension(digits * 3 + (digits - 1) * (digits > 0) * spacing, 4); }
  virtual Color GetColor(uint16_t digit) { 
    if (colorFunc)
    {
      return (*colorFunc)(digit);
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

  void SetName(string name) { this->name = name; }
  void SetColor(Color color) { this->color = color; }
  void SetAlternativeColor(Color alternativeColor) { this->alternativeColor = alternativeColor; }
  void SetDigits(uint8_t digits) { this->digits = digits; }

  void SetValuePointer(int32_t* valuePtr) {
    this->valuePtr = valuePtr;
    this->getValueFunc = nullptr;  // Clear getValueFunc when setting pointer
  }

  void SetValueFunc(std::function<int32_t()> getValueFunc) {
    this->getValueFunc = std::make_unique<std::function<int32_t()>>(getValueFunc);
    this->valuePtr = nullptr;  // Clear valuePtr when setting function
  }

  int32_t GetValue() {
    // Prioritize getValueFunc if set
    if (getValueFunc != nullptr) {
      return (*getValueFunc)();
    }
    return (valuePtr != nullptr) ? *valuePtr : 0;
  }

  void SetValue(int32_t value) {
    // SetValue should not work if getValueFunc is set
    if (getValueFunc == nullptr && valuePtr != nullptr) {
      *valuePtr = value;
    }
  }

  void SetSpacing(uint8_t spacing) { this->spacing = spacing; }
  void SetColorFunc(std::function<Color(uint16_t digit)> colorFunc) { this->colorFunc = std::make_unique<std::function<Color(uint16_t digit)>>(colorFunc); } 
  void Render4pxNumber(Point origin, Color color, uint8_t value) {
    // MLOGD("4PX", "Num: %d, render at %d-%d", value, origin.x, origin.y);
    if (value < 11 /*&& value >= 0*/)
    {
      for (int8_t x = 0; x < 3; x++)
      {
        for (int8_t y = 0; y < 4; y++)
        { MatrixOS::LED::SetColor(origin + Point(x, 3 - y), bitRead(number4px[value][x], y) ? color : Color(0)); }
      }
    }
  }

  virtual bool Render(Point origin) {
    int32_t value = GetValue();  // Use GetValue() to support both pointer and function
    uint8_t sig_figure = int(log10(value) + 1);
    Point render_origin = origin;
    // MLOGD("4PX", "Render %d, sigfig %d", value, sig_figure);
    for (int8_t digit = digits - 1; digit >= 0; digit--)
    {
      if (digit < sig_figure || digit == 0)
      { Render4pxNumber(render_origin, GetColor(digit), (int)(value / std::pow(10, digit)) % 10); }
      else
      {
        Render4pxNumber(render_origin, Color(0), 10);
      }

      render_origin = render_origin + Point(3 + spacing, 0);
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->State() == HOLD)
    {
      MatrixOS::UIUtility::TextScroll(name, GetColor(UINT16_MAX));
    }
    return true;
  }
};