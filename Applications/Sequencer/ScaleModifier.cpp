#include "ScaleModifier.h"

SequenceScaleModifier::SequenceScaleModifier(Color color, Color rootColor) {
  this->color = color;
  this->rootColor = rootColor;
}

// SetScaleFunc and OnChange are now inline templates in ScaleModifier.h

void SequenceScaleModifier::OnChangeCallback(uint16_t newScale) {
  if (changeCallback)
  {
    changeCallback(newScale);
  }
}

Color SequenceScaleModifier::GetColor() {
  return color;
}

Dimension SequenceScaleModifier::GetSize() {
  return Dimension(8, 2);
}

bool SequenceScaleModifier::Render(Point origin) {
  // Root is always 0, so we just use the scale directly
  uint16_t scaleMap = getScale() & 0xFFF;

  for (uint8_t note = 0; note < 12; note++)
  {
    Point xy = origin + ((note < 5) ? Point((note + 1) / 2, (note + 1) % 2) : Point((note + 2) / 2, note % 2));
    if (note == 0)
    {
      MatrixOS::LED::SetColor(xy, rootColor);
    }
    else if (bitRead(scaleMap, note))
    {
      MatrixOS::LED::SetColor(xy, color);
    }
    else
    {
      MatrixOS::LED::SetColor(xy, color.DimIfNot());
    }
  }
  MatrixOS::LED::SetColor(origin + Point(7, 1), rootColor);
  return true;
}

bool SequenceScaleModifier::KeyEvent(Point xy, KeypadInfo* keypadInfo) {
  if (keypadInfo->state == KeypadState::Hold)
  {
    MatrixOS::UIUtility::TextScroll("Custom Scale Modifier", color);
  }
  else if (keypadInfo->state == KeypadState::Released && keypadInfo->hold == false)
  {
    uint8_t note = xy.x * 2 + xy.y - 1 - (xy.x > 2);

    // Flip the bit for the selected note
    uint16_t newScale = getScale() ^ (1 << note);
    OnChangeCallback(newScale);
  }

  return true;
}
