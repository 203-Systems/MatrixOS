#pragma once
#include "UIComponent.h"
#include <limits.h>

// TODO add negative support?
class UINumberModifier : public UIComponent {
public:
  Color color;
  uint8_t length;
  int32_t* valuePtr;
  const int32_t* modifiers;
  const uint8_t* controlGradient;
  int32_t lowerLimit;
  int32_t upperLimit;
  UICallback<void(int32_t)> changeCallback;

  UINumberModifier() {
    this->color = Color(0);
    this->length = 0;
    this->valuePtr = nullptr;
    this->modifiers = nullptr;
    this->controlGradient = nullptr;
    this->lowerLimit = INT_MIN;
    this->upperLimit = INT_MAX;
  }

  virtual Dimension GetSize() {
    return Dimension(length, 1);
  }
  virtual Color GetColor() {
    return color;
  }

  void SetColor(Color color) {
    this->color = color;
  }
  void SetLength(uint8_t length) {
    this->length = length;
  }
  void SetValuePointer(int32_t* valuePtr) {
    this->valuePtr = valuePtr;
  }
  void SetModifiers(const int32_t* modifiers) {
    this->modifiers = modifiers;
  }
  void SetControlGradient(const uint8_t* controlGradient) {
    this->controlGradient = controlGradient;
  }
  void SetLowerLimit(int32_t lowerLimit) {
    this->lowerLimit = lowerLimit;
  }
  void SetUpperLimit(int32_t upperLimit) {
    this->upperLimit = upperLimit;
  }

  template <typename F> void OnChange(F&& f) {
    this->changeCallback = UICallback<void(int32_t)>(static_cast<F&&>(f));
  }

  virtual void OnChangeCallback(int32_t value) {
    if (changeCallback)
    {
      changeCallback(value);
    }
  }

  virtual bool Render(Point origin) {
    for (uint8_t i = 0; i < length; i++)
    {
      MatrixOS::LED::SetColor(origin + Point(i, 0), color.Scale(controlGradient[i]));
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo) {
    if (keypadInfo->state == KeypadState::Pressed)
    {
      int64_t newValue = *valuePtr;
      newValue += modifiers[xy.x];
      if (newValue > upperLimit)
      {
        newValue = upperLimit;
      }
      else if (newValue < lowerLimit)
      {
        newValue = lowerLimit;
      }

      int32_t finalValue = (int32_t)newValue;
      *valuePtr = finalValue;

      // Trigger the callback if set
      OnChangeCallback(finalValue);
    }
    return true; // Prevent leak though to cause UI text scroll
  }
};