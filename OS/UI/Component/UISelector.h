#pragma once
#include "UISelectorBase.h"

class UISelector : public UISelectorBase {
 public:
  uint16_t* valuePtr;
  UISelectorLitMode litMode;
  std::unique_ptr<std::function<void(uint16_t)>> changeCallback;
  std::unique_ptr<std::function<uint16_t()>> getValueFunc;

  UISelector() : UISelectorBase() {
    this->valuePtr = nullptr;
    this->litMode = UISelectorLitMode::LIT_EQUAL;
    this->changeCallback = nullptr;
    this->getValueFunc = nullptr;
  }

  void SetLitMode(UISelectorLitMode litMode) { this->litMode = litMode; }

  void SetValuePointer(uint16_t* valuePtr) {
    this->valuePtr = valuePtr;
    this->getValueFunc = nullptr;  // Clear getValueFunc when setting pointer
  }

  void SetValueFunc(std::function<uint16_t()> getValueFunc) {
    this->getValueFunc = std::make_unique<std::function<uint16_t()>>(getValueFunc);
    this->valuePtr = nullptr;  // Clear valuePtr when setting function
  }

  void OnChange(std::function<void(uint16_t)> changeCallback) { this->changeCallback = std::make_unique<std::function<void(uint16_t)>>(changeCallback); }

  virtual void OnChangeCallback(uint16_t value) {
    if (changeCallback != nullptr) {
      (*changeCallback)(value);
    }
  }

  uint16_t GetValue() {
    // Prioritize getValueFunc if set
    if (getValueFunc != nullptr) {
      return (*getValueFunc)();
    }
    return (valuePtr != nullptr) ? *valuePtr : 0;
  }

  void SetValue(uint16_t value) {
    // SetValue should not work if getValueFunc is set
    if (getValueFunc == nullptr && valuePtr != nullptr) {
      *valuePtr = value;
    }
  }

  virtual void Selected(uint16_t value) override {
    SetValue(value);
    OnChangeCallback(value);
  }

  virtual bool ShouldLit(uint16_t index) override{
    uint16_t currentValue = GetValue();  // Use GetValue() to support both pointer and function
    switch (litMode) {
      case UISelectorLitMode::LIT_EQUAL:
        return index == currentValue;
      case UISelectorLitMode::LIT_LESS_EQUAL_THAN:
        return index <= currentValue;
      case UISelectorLitMode::LIT_GREATER_EQUAL_THAN:
        return index >= currentValue;
      case UISelectorLitMode::LIT_ALWAYS:
        return true;
    }
    return false;
  }
};