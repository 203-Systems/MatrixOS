#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>
#include <memory>

class SequenceScaleModifier : public UIComponent {
 public:
  uint16_t* scale;
  std::unique_ptr<std::function<void(uint16_t)>> changeCallback;
  Color color;
  Color rootColor;

  SequenceScaleModifier(uint16_t* scale, Color color = Color(0x00FFFF), Color rootColor = Color(0x0040FF));

  void ChangeScalePtr(uint16_t* scale);
  void OnChange(std::function<void(uint16_t)> changeCallback);
  virtual void OnChangeCallback(uint16_t newScale);

  virtual Color GetColor();
  virtual Dimension GetSize();
  virtual bool Render(Point origin);
  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
};
