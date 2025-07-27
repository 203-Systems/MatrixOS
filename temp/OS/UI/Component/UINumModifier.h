#pragma once
#include "UIComponent.h"
#include <limits.h>

// TODO add negative support?
class UINumberModifier : public UIComponent {
 public:
  Color color;
  uint8_t length;
  int32_t* valuePtr;
  int32_t* modifiers;
  uint8_t* control_gradient;
  int32_t lower_limit;
  int32_t upper_limit;

  UINumberModifier() {
    this->color = Color(0);
    this->length = 0;
    this->valuePtr = nullptr;
    this->modifiers = nullptr;
    this->control_gradient = nullptr;
    this->lower_limit = INT_MIN;
    this->upper_limit = INT_MAX;
  }

  virtual Dimension GetSize() { return Dimension(length, 1); }
  virtual Color GetColor() { return color; }

  void SetColor(Color color) { this->color = color; }
  void SetLength(uint8_t length) { this->length = length; }
  void SetValuePointer(int32_t* valuePtr) { this->valuePtr = valuePtr; }
  void SetModifiers(int32_t* modifiers) { this->modifiers = modifiers; }
  void SetControlGradient(uint8_t* control_gradient) { this->control_gradient = control_gradient; }
  void SetLowerLimit(int32_t lower_limit) { this->lower_limit = lower_limit; }
  void SetUpperLimit(int32_t upper_limit) { this->upper_limit = upper_limit; }

  virtual bool Render(Point origin) {
    for (uint8_t i = 0; i < length; i++)
    { MatrixOS::LED::SetColor(origin + Point(i, 0), color.Scale(control_gradient[i])); }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->state == PRESSED)
    {
      int64_t new_value = *valuePtr;
      new_value += modifiers[xy.x];
      if (new_value > upper_limit)
      { new_value = upper_limit; }
      else if (new_value < lower_limit)
      { new_value = lower_limit; }

      *valuePtr = (int32_t)new_value;
    }
    return true;  // Prevent leak though to cause UI text scroll
  }
};