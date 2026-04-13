#include "ScaleVisualizer.h"

SequencerScaleVisualizer::SequencerScaleVisualizer(Color color, Color rootColor, Color rootOffsetColor) {
  this->color = color;
  this->rootColor = rootColor;
  this->rootOffsetColor = rootOffsetColor;
}

void SequencerScaleVisualizer::SetGetRootKeyFunc(UICallback<uint8_t()> func) {
  getRootKey = std::move(func);
}
void SequencerScaleVisualizer::SetGetRootOffsetFunc(UICallback<uint8_t()> func) {
  getRootOffset = std::move(func);
}
void SequencerScaleVisualizer::SetGetScaleFunc(UICallback<uint16_t()> func) {
  getScale = std::move(func);
}

void SequencerScaleVisualizer::OnChange(UICallback<void(uint8_t, uint8_t, uint16_t)> callback) {
  onChange = std::move(callback);
}

Color SequencerScaleVisualizer::GetColor() {
  return color;
}

Dimension SequencerScaleVisualizer::GetSize() {
  return Dimension(7, 2);
}

bool SequencerScaleVisualizer::Render(Point origin) {
  uint16_t cAlignedScaleMap =
      ((getScale() << getRootKey()) + ((getScale() & 0xFFF) >> (12 - getRootKey() % 12))) & 0xFFF; // Root key should always < 12,
                                                                                                   // might add an assert later
  for (uint8_t note = 0; note < 12; note++)
  {
    Point xy = origin + ((note < 5) ? Point((note + 1) / 2, (note + 1) % 2) : Point((note + 2) / 2, note % 2));
    if (note == getRootKey())
    {
      MatrixOS::LED::SetColor(xy, rootColor);
    }
    else if (getRootOffset() != 0 && note == (getRootOffset() + getRootKey()) % 12)
    {
      MatrixOS::LED::SetColor(xy, rootOffsetColor);
    }
    else if (bitRead(cAlignedScaleMap, note))
    {
      MatrixOS::LED::SetColor(xy, color);
    }
    else
    {
      MatrixOS::LED::SetColor(xy, color.DimIfNot());
    }
  }
  return true;
}

bool SequencerScaleVisualizer::KeyEvent(Point xy, KeypadInfo* keypadInfo) {
  if (xy == Point(0, 0) || xy == Point(3, 0))
    return false;

  if (keypadInfo->state == KeypadState::Hold)
  {
    MatrixOS::UIUtility::TextScroll("Scale Visualizer", color);
  }

  if (keypadInfo->state != KeypadState::Pressed)
  {
    return true;
  }

  uint8_t note = xy.x * 2 + xy.y - 1 - (xy.x > 2);

  if (offsetMode)
  {
    // Only allow setting rootOffset if the note is in the scale
    uint16_t cAlignedScaleMap = ((getScale() << getRootKey()) + ((getScale() & 0xFFF) >> (12 - getRootKey() % 12))) & 0xFFF;

    if (bitRead(cAlignedScaleMap, note))
    {
      uint8_t newOffset = ((note + 12) - getRootKey()) % 12;
      if (onChange)
      {
        onChange(getRootKey(), newOffset, getScale());
      }
    }
  }
  else
  {
    // apply via callback
    if (onChange)
    {
      onChange(note, 0, getScale());
    }
  }

  return true;
}
