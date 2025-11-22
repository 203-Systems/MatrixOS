#include "ScaleVisualizer.h"

ScaleVisualizer::ScaleVisualizer(uint8_t* rootKey, uint8_t* rootOffset, uint16_t* scale, Color color, Color rootColor, Color rootOffsetColor) {
  this->rootKey = rootKey;
  this->rootOffset = rootOffset;
  this->scale = scale;
  this->color = color;
  this->rootColor = rootColor;
  this->rootOffsetColor = rootOffsetColor;
}

void ScaleVisualizer::OnChange(std::function<void()> callback) {
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
      ((*scale << *rootKey) + ((*scale & 0xFFF) >> (12 - *rootKey % 12))) & 0xFFF;  // Root key should always < 12,
                                                                                    // might add an assert later
  for (uint8_t note = 0; note < 12; note++)
  {
    Point xy = origin + ((note < 5) ? Point((note + 1) / 2, (note + 1) % 2) : Point((note + 2) / 2, note % 2));
    if (note == *rootKey)
    { MatrixOS::LED::SetColor(xy, rootColor); }
    else if (*rootOffset != 0 && note == (*rootOffset + *rootKey) % 12)
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
        ((*scale << *rootKey) + ((*scale & 0xFFF) >> (12 - *rootKey % 12))) & 0xFFF;

    if (bitRead(c_aligned_scale_map, note)) {
      *rootOffset = ((note + 12) - *rootKey) % 12;
      if (onChange) { onChange(); }
    }
  } else {
    *rootKey = note;
    *rootOffset = 0;
    if (onChange) { onChange(); }
  }

  return true;
}
