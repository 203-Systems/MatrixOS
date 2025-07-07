#pragma once
#include "MatrixOS.h"
#include <memory>

class UIComponent {
 public:
  bool enabled;
  std::unique_ptr<std::function<bool()>> enable_func;

  UIComponent() {
    this->enabled = true;
    this->enable_func = nullptr;
  }

  virtual Dimension GetSize() { return Dimension(0, 0); }
  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) { return false; }  //

  virtual bool Render(Point origin) { return false; }

  void SetEnabled(bool enabled) { this->enabled = enabled; }
  void ShouldEnable(std::function<bool()> enable_func) { this->enable_func = std::make_unique<std::function<bool()>>(enable_func); }  

  bool IsEnabled() {
    if (enable_func) {
      return (*enable_func)();
    }
    return enabled;
  }

  virtual ~UIComponent(){};

  operator UIComponent*() { return this; }
};
