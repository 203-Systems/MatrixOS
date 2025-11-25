#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

class SequencerScaleVisualizer : public UIComponent {
 public:
  std::function<uint8_t()> getRootKey;
  std::function<uint8_t()> getRootOffset;
  std::function<uint16_t()> getScale;
  Color color;
  Color rootColor;
  Color rootOffsetColor;
  bool offsetMode = false;

  SequencerScaleVisualizer(Color color, Color rootColor, Color rootOffsetColor);

  void SetGetRootKeyFunc(std::function<uint8_t()> func);
  void SetGetRootOffsetFunc(std::function<uint8_t()> func);
  void SetGetScaleFunc(std::function<uint16_t()> func);

  void OnChange(std::function<void(uint8_t root, uint8_t offset, uint16_t scale)> callback);

 private:
  std::function<void(uint8_t, uint8_t, uint16_t)> onChange;

  virtual Color GetColor();
  virtual Dimension GetSize();
  virtual bool Render(Point origin);
  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
};
