#pragma once
#include "UISelectorBase.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

template <class T>
class UIItemSelector : public UISelectorBase {
 public:
  T* itemPtr;
  T* items;
  UISelectorLitMode litMode;
  std::unique_ptr<std::function<void(const T&)>> changeCallback;

  UIItemSelector() : UISelectorBase() {
    this->litMode = UISelectorLitMode::LIT_EQUAL;
    this->changeCallback = nullptr;
  }

  void SetLitMode(UISelectorLitMode litMode) { this->litMode = litMode; }
  void SetItemPointer(T* itemPtr) { this->itemPtr = itemPtr; }
  void SetItems(T* items) { this->items = items; }
  void OnChange(std::function<void(const T&)> changeCallback) { this->changeCallback = std::make_unique<std::function<void(const T&)>>(changeCallback); }

  protected:
  virtual void OnChangeCallback(const T& item) {
    if (changeCallback != nullptr) {
      (*changeCallback)(item);
    }
  }

  T& GetItem() { return *itemPtr; }
  void SetItem(const T& item) { if(itemPtr != nullptr) { *itemPtr = item; } }

  virtual void Selected(uint16_t index) override {
    SetItem(items[index]);
    OnChangeCallback(items[index]);
  }

  uint16_t currentItemIndex;
  virtual bool ShouldLit(uint16_t index) override{
    switch (litMode) {
      case UISelectorLitMode::LIT_EQUAL:
        return index == currentItemIndex;
      case UISelectorLitMode::LIT_LESS_EQUAL_THAN:
        return index <= currentItemIndex;
      case UISelectorLitMode::LIT_GREATER_EQUAL_THAN:
        return index >= currentItemIndex;
      case UISelectorLitMode::LIT_ALWAYS:
        return true;
    }
    return false;
  }

  public:
  virtual bool Render(Point origin) override {
    // Find the index of the item pointed to by itemPtr in items array
    currentItemIndex = UINT16_MAX;
    if (itemPtr != nullptr && items != nullptr) {
      for (uint16_t i = 0; i < MAX(dimension.Area(), count); i++) {
        if (items[currentItemIndex] == *itemPtr) {
          break;
        }
        currentItemIndex++;
      }
    }

    return UISelectorBase::Render(origin);
  }
};