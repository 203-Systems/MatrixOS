#pragma once
#include "../UIUtilities.h"

class UIToggle : public UIButton {
 public:
  bool* valuePtr;

  UIToggle() {}

  void SetValuePointer(bool* valuePtr) { this->valuePtr = valuePtr; }

  string GetName() override { return name + " " + (*valuePtr ? "On" : "Off"); }

  Color GetColor() override {
    if (colorFunc) {
      return (*colorFunc)();
    }
    return color.DimIfNot(*valuePtr);
  }

  bool PressCallback() override {
    *valuePtr = !*valuePtr;
    if (pressCallback) {
      (*pressCallback)();
      return true;
    }
    return true;
  }
};