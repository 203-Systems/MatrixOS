#pragma once
#include "MatrixOS.h"
#include "../UICallback.h"

class UIComponent {
public:
  bool enabled;
  UICallback<bool()> enableFunc;

  UIComponent() {
    this->enabled = true;
  }

  virtual Dimension GetSize() {
    return Dimension(0, 0);
  }
  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo) {
    return false;
  } //

  virtual bool Render(Point origin) {
    return false;
  }

  virtual void SetEnabled(bool enabled) {
    this->enabled = enabled;
  }
  template <typename F> void SetEnableFunc(F&& f) {
    this->enableFunc = UICallback<bool()>(static_cast<F&&>(f));
  }

  virtual bool IsEnabled() {
    if (enableFunc)
    {
      return enableFunc();
    }
    return enabled;
  }

  virtual ~UIComponent() {};

  operator UIComponent*() {
    return this;
  }
};
