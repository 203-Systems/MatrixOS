#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

class SequencerScaleVisualizer : public UIComponent {
public:
  UICallback<uint8_t()> getRootKey;
  UICallback<uint8_t()> getRootOffset;
  UICallback<uint16_t()> getScale;
  Color color;
  Color rootColor;
  Color rootOffsetColor;
  bool offsetMode = false;

  SequencerScaleVisualizer(Color color, Color rootColor, Color rootOffsetColor);

  void SetGetRootKeyFunc(UICallback<uint8_t()> func);
  void SetGetRootOffsetFunc(UICallback<uint8_t()> func);
  void SetGetScaleFunc(UICallback<uint16_t()> func);

  void OnChange(UICallback<void(uint8_t root, uint8_t offset, uint16_t scale)> callback);

private:
  UICallback<void(uint8_t, uint8_t, uint16_t)> onChange;

  virtual Color GetColor();
  virtual Dimension GetSize();
  virtual bool Render(Point origin);
  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo);
};
