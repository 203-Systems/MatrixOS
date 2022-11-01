#pragma once
#include "UIComponent.h"
#include <limits.h>

// TODO add negative support?
class UINumberModifier : public UIComponent {
 public:
  Color color;
  uint8_t length;
  int32_t* value;
  int32_t* modifiers;
  uint8_t* control_gradient;
  int32_t lower_limit;
  int32_t upper_limit;

  UINumberModifier(Color color, uint8_t length, int32_t* value, int32_t* modifiers, uint8_t* control_gradient,
                   int32_t lower_limit = INT_MIN, int32_t upper_limit = INT_MAX) {
    this->color = color;
    this->length = length;
    this->value = value;
    this->modifiers = modifiers;
    this->control_gradient = control_gradient;
    this->lower_limit = lower_limit;
    this->upper_limit = upper_limit;
  }

  virtual Dimension GetSize() { return Dimension(length, 1); }
  virtual Color GetColor() { return color; }

  virtual bool Render(Point origin) {
    for (uint8_t i = 0; i < length; i++)
    { MatrixOS::LED::SetColor(origin + Point(i, 0), color.Scale(control_gradient[i])); }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->state == PRESSED)
    {
      int64_t new_value = *value;
      new_value += modifiers[xy.x];
      if (new_value > upper_limit)
      { new_value = upper_limit; }
      else if (new_value < lower_limit)
      { new_value = lower_limit; }

      *value = (int32_t)new_value;
    }
    return true;  // Prevent leak though to cause UI text scroll
  }
};