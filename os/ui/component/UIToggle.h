#pragma once
#include "../UIUtilities.h"

class UIToggle : public UIButton {
 public:
  bool* value;

  UIToggle() {}

  void SetValue(bool* value) { this->value = value; }

  string GetName() override { return name + " " + (*value ? "On" : "Off"); }

  Color GetColor() override {
    if (color_func) {
      return color_func();
    }
    return color.DimIfNot(*value);
  }

  bool PressCallback() override {
    *value = !*value;
    if (press_callback != nullptr) {
      press_callback();
      return true;
    }
    return true;
  }
};