#pragma once
#include "MatrixOS.h"
#include <memory>

class UIComponent {
 public:
  bool enabled = true;
  std::unique_ptr<std::function<bool()>> enable_func;

  // Default constructor
  UIComponent() = default;
  
  // Copy constructor
  UIComponent(const UIComponent& other) : enabled(other.enabled) {
    if (other.enable_func) {
      enable_func = std::make_unique<std::function<bool()>>(*other.enable_func);
    }
  }
  
  // Copy assignment operator
  UIComponent& operator=(const UIComponent& other) {
    if (this != &other) {
      enabled = other.enabled;
      if (other.enable_func) {
        enable_func = std::make_unique<std::function<bool()>>(*other.enable_func);
      } else {
        enable_func.reset();
      }
    }
    return *this;
  }
  
  // Move constructor
  UIComponent(UIComponent&& other) noexcept = default;
  
  // Move assignment operator
  UIComponent& operator=(UIComponent&& other) noexcept = default;

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
