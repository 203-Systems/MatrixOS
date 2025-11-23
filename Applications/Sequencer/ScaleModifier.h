#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>
#include <memory>

class SequenceScaleModifier : public UIComponent {
 public:
  std::function<uint16_t()> getScale;
  std::unique_ptr<std::function<void(uint16_t)>> changeCallback;
  Color color;
  Color rootColor;

  SequenceScaleModifier(Color color, Color rootColor);

  void SetScaleFunc(std::function<uint16_t()> getScale);
  void OnChange(std::function<void(uint16_t)> changeCallback);
  virtual void OnChangeCallback(uint16_t newScale);

  virtual Color GetColor();
  virtual Dimension GetSize();
  virtual bool Render(Point origin);
  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
};
