#pragma once
#include "MatrixOS.h"
#include <memory>

class UIComponent {
 public:
  bool enabled;
  std::unique_ptr<std::function<bool()>> enableFunc;

  UIComponent() {
    this->enabled = true;
    this->enableFunc = nullptr;
  }

  virtual Dimension GetSize() { return Dimension(0, 0); }
  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) { return false; }  //

  virtual bool Render(Point origin) { return false; }

  virtual void SetEnabled(bool enabled) { this->enabled = enabled; }
  virtual void SetEnableFunc(std::function<bool()> enableFunc) { this->enableFunc = std::make_unique<std::function<bool()>>(enableFunc); }  

  virtual bool IsEnabled() {
    if (enableFunc) {
      return (*enableFunc)();
    }
    return enabled;
  }

  virtual ~UIComponent(){};

  operator UIComponent*() { return this; }
};
