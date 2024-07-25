#pragma once
#include "MatrixOS.h"

class UIComponent {
 public:
  bool enabled = true;
  virtual Dimension GetSize() { return Dimension(0, 0); }
  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) { return false; }  //

  virtual bool Render(Point origin) { return false; }

  void SetEnabled(bool enabled) { this->enabled = enabled; }

  virtual ~UIComponent(){};

  operator UIComponent*() { return this; }
};
