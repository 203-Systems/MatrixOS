#pragma once
#include "../UIUtilities.h"

class UIToggle : public UIButton {
 public:
  bool* valuePtr;

  UIToggle() {}

  void SetValuePointer(bool* valuePtr) { this->valuePtr = valuePtr; }

  string GetName() override { return name + " " + (*valuePtr ? "On" : "Off"); }

  Color GetColor() override {
    if (color_func) {
      return (*color_func)();
    }
    return color.DimIfNot(*valuePtr);
  }

  bool PressCallback() override {
    *valuePtr = !*valuePtr;
    if (press_callback) {
      (*press_callback)();
      return true;
    }
    return true;
  }
};