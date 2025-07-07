#pragma once
#include "MatrixOS.h"
#include <optional>

class UIComponent {
 public:
  bool enabled = true;
  std::optional<std::function<bool()>> enable_func;

  virtual Dimension GetSize() { return Dimension(0, 0); }
  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) { return false; }  //

  virtual bool Render(Point origin) { return false; }

  void SetEnabled(bool enabled) { this->enabled = enabled; }
  void ShouldEnable(std::function<bool()> enable_func) { this->enable_func = enable_func; }

  bool IsEnabled() {
    if (enable_func.has_value()) {
      return (*enable_func)();
    }
    return enabled;
  }


  virtual ~UIComponent(){};

  operator UIComponent*() { return this; }
};
