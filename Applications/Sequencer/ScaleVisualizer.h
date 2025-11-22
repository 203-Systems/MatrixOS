#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

class ScaleVisualizer : public UIComponent {
 public:
  uint8_t* rootKey;
  uint8_t* rootOffset;
  uint16_t* scale;
  Color color;
  Color rootColor;
  Color rootOffsetColor;
  bool offsetMode = false;

  ScaleVisualizer(uint8_t* rootKey, uint8_t* rootOffset, uint16_t* scale, Color color = Color(0xFF00FF), Color rootColor = Color(0x8000FF), Color rootOffsetColor = Color(0xFF0080));

  void OnChange(std::function<void()> callback);

 private:
  std::function<void()> onChange;

  virtual Color GetColor();
  virtual Dimension GetSize();
  virtual bool Render(Point origin);
  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
};
