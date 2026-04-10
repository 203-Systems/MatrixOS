#pragma once
#include "UISelectorBase.h"

class UISelector : public UISelectorBase {
public:
  uint16_t* valuePtr;
  UISelectorLitMode litMode;
  UICallback<void(uint16_t)> changeCallback;
  UICallback<uint16_t()> getValueFunc;

  UISelector() : UISelectorBase() {
    this->valuePtr = nullptr;
    this->litMode = UISelectorLitMode::LIT_EQUAL;
  }

  void SetLitMode(UISelectorLitMode litMode) {
    this->litMode = litMode;
  }

  void SetValuePointer(uint16_t* valuePtr) {
    this->valuePtr = valuePtr;
    this->getValueFunc.Reset();
  }

  template <typename F> void SetValueFunc(F&& f) {
    this->getValueFunc = UICallback<uint16_t()>(static_cast<F&&>(f));
    this->valuePtr = nullptr;
  }

  template <typename F> void OnChange(F&& f) {
    this->changeCallback = UICallback<void(uint16_t)>(static_cast<F&&>(f));
  }

  virtual void OnChangeCallback(uint16_t value) {
    if (changeCallback)
    {
      changeCallback(value);
    }
  }

  uint16_t GetValue() {
    if (getValueFunc)
    {
      return getValueFunc();
    }
    return (valuePtr != nullptr) ? *valuePtr : 0;
  }

  void SetValue(uint16_t value) {
    // SetValue should not work if getValueFunc is set
    if (!getValueFunc && valuePtr != nullptr)
    {
      *valuePtr = value;
    }
  }

  virtual void Selected(uint16_t value) override {
    SetValue(value);
    OnChangeCallback(value);
  }

  virtual bool ShouldLit(uint16_t index) override {
    uint16_t currentValue = GetValue(); // Use GetValue() to support both pointer and function
    switch (litMode)
    {
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