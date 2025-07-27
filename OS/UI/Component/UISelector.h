#pragma once
#include "UISelectorBase.h"

class UISelector : public UISelectorBase {
 public:
  uint16_t* valuePtr;
  UISelectorLitMode litMode;
  std::unique_ptr<std::function<void(uint16_t)>> changeCallback;

  UISelector() : UISelectorBase() {
    this->valuePtr = nullptr;
    this->litMode = UISelectorLitMode::LIT_EQUAL;
    this->changeCallback = nullptr;
  }

  void SetLitMode(UISelectorLitMode litMode) { this->litMode = litMode; }
  void SetValuePointer(uint16_t* valuePtr) { this->valuePtr = valuePtr; }
  void OnChange(std::function<void(uint16_t)> changeCallback) { this->changeCallback = std::make_unique<std::function<void(uint16_t)>>(changeCallback); }

  virtual void OnChangeCallback(uint16_t value) {
    if (changeCallback != nullptr) {
      (*changeCallback)(value);
    }
  }

  uint16_t GetValue() { return (valuePtr != nullptr) ? *valuePtr : 0; }
  void SetValue(uint16_t value) { if(valuePtr != nullptr) { *valuePtr = value; } }

  virtual void Selected(uint16_t value) override {
    SetValue(value);
    OnChangeCallback(value);
  }

  virtual bool ShouldLit(uint16_t index) override{
    switch (litMode) {
      case UISelectorLitMode::LIT_EQUAL:
        return index == *valuePtr;
      case UISelectorLitMode::LIT_LESS_EQUAL_THAN:
        return index <= *valuePtr;
      case UISelectorLitMode::LIT_GREATER_EQUAL_THAN:
        return index >= *valuePtr;
      case UISelectorLitMode::LIT_ALWAYS:
        return true;
    }
    return false;
  }
};