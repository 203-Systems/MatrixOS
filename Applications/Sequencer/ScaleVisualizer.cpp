#include "ScaleVisualizer.h"

ScaleVisualizer::ScaleVisualizer(Color color, Color rootColor, Color rootOffsetColor)
{
  this->color = color;
  this->rootColor = rootColor;
  this->rootOffsetColor = rootOffsetColor;
}

void ScaleVisualizer::SetGetRootKeyFunc(std::function<uint8_t()> func) { getRootKey = func; }
void ScaleVisualizer::SetGetRootOffsetFunc(std::function<uint8_t()> func) { getRootOffset = func; }
void ScaleVisualizer::SetGetScaleFunc(std::function<uint16_t()> func) { getScale = func; }

void ScaleVisualizer::OnChange(std::function<void(uint8_t, uint8_t, uint16_t)> callback) {
  onChange = callback;
}

Color ScaleVisualizer::GetColor() {
  return color;
}

Dimension ScaleVisualizer::GetSize() {
  return Dimension(7, 2);
}

bool ScaleVisualizer::Render(Point origin) {
  uint16_t c_aligned_scale_map =
      ((getScale() << getRootKey()) + ((getScale() & 0xFFF) >> (12 - getRootKey() % 12))) & 0xFFF;  // Root key should always < 12,
                                                                                    // might add an assert later
  for (uint8_t note = 0; note < 12; note++)
  {
    Point xy = origin + ((note < 5) ? Point((note + 1) / 2, (note + 1) % 2) : Point((note + 2) / 2, note % 2));
    if (note == getRootKey())
    { MatrixOS::LED::SetColor(xy, rootColor); }
    else if (getRootOffset() != 0 && note == (getRootOffset() + getRootKey()) % 12)
    { MatrixOS::LED::SetColor(xy, rootOffsetColor); }
    else if (bitRead(c_aligned_scale_map, note))
    { MatrixOS::LED::SetColor(xy, color); }
    else
    { MatrixOS::LED::SetColor(xy, color.DimIfNot()); }
  }
  return true;
}

bool ScaleVisualizer::KeyEvent(Point xy, KeyInfo* keyInfo) {
  if (xy == Point(0, 0) || xy == Point(3, 0))
    return false;

  if (keyInfo->State() == HOLD)
  {
    MatrixOS::UIUtility::TextScroll("Scale Visualizer", color);
  }

  if (keyInfo->State() != PRESSED)
  {
    return true;
  }

  uint8_t note = xy.x * 2 + xy.y - 1 - (xy.x > 2);

  if (offsetMode) {
    // Only allow setting rootOffset if the note is in the scale
    uint16_t c_aligned_scale_map =
        ((getScale() << getRootKey()) + ((getScale() & 0xFFF) >> (12 - getRootKey() % 12))) & 0xFFF;

    if (bitRead(c_aligned_scale_map, note)) {
      uint8_t newOffset = ((note + 12) - getRootKey()) % 12;
      if (onChange) { onChange(getRootKey(), newOffset, getScale()); }
    }
  } else {
    // apply via callback
    if (onChange) { onChange(note, 0, getScale()); }
  }

  return true;
}
