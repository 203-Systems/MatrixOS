#pragma once
#include "UIComponent.h"
#include <limits.h>
#include <functional>
#include <memory>

// TODO add negative support?
class UINumberModifier : public UIComponent {
 public:
  Color color;
  uint8_t length;
  int32_t* valuePtr;
  int32_t* modifiers;
  uint8_t* controlGradient;
  int32_t lowerLimit;
  int32_t upperLimit;
  std::unique_ptr<std::function<void(int32_t)>> changeCallback;

  UINumberModifier() {
    this->color = Color(0);
    this->length = 0;
    this->valuePtr = nullptr;
    this->modifiers = nullptr;
    this->controlGradient = nullptr;
    this->lowerLimit = INT_MIN;
    this->upperLimit = INT_MAX;
    this->changeCallback = nullptr;
  }

  virtual Dimension GetSize() { return Dimension(length, 1); }
  virtual Color GetColor() { return color; }

  void SetColor(Color color) { this->color = color; }
  void SetLength(uint8_t length) { this->length = length; }
  void SetValuePointer(int32_t* valuePtr) { this->valuePtr = valuePtr; }
  void SetModifiers(int32_t* modifiers) { this->modifiers = modifiers; }
  void SetControlGradient(uint8_t* controlGradient) { this->controlGradient = controlGradient; }
  void SetLowerLimit(int32_t lowerLimit) { this->lowerLimit = lowerLimit; }
  void SetUpperLimit(int32_t upperLimit) { this->upperLimit = upperLimit; }

  void OnChange(std::function<void(int32_t)> changeCallback) { this->changeCallback = std::make_unique<std::function<void(int32_t)>>(changeCallback); }

  virtual void OnChangeCallback(int32_t value) {
    if (changeCallback != nullptr) {
      (*changeCallback)(value);
    }
  }

  virtual bool Render(Point origin) {
    for (uint8_t i = 0; i < length; i++)
    { MatrixOS::LED::SetColor(origin + Point(i, 0), color.Scale(controlGradient[i])); }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->State() == PRESSED)
    {
      int64_t new_value = *valuePtr;
      new_value += modifiers[xy.x];
      if (new_value > upperLimit)
      { new_value = upperLimit; }
      else if (new_value < lowerLimit)
      { new_value = lowerLimit; }

      int32_t final_value = (int32_t)new_value;
      *valuePtr = final_value;

      // Trigger the callback if set
      OnChangeCallback(final_value);
    }
    return true;  // Prevent leak though to cause UI text scroll
  }
};