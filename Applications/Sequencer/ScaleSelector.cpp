#include "ScaleSelector.h"
#include "Scales.h"

constexpr uint16_t scales[16] = {
    static_cast<uint16_t>(Scale::MINOR),
    static_cast<uint16_t>(Scale::MAJOR),
    static_cast<uint16_t>(Scale::DORIAN),
    static_cast<uint16_t>(Scale::PHRYGIAN),
    static_cast<uint16_t>(Scale::MIXOLYDIAN),
    static_cast<uint16_t>(Scale::MELODIC_MINOR_ASCENDING),
    static_cast<uint16_t>(Scale::HARMONIC_MINOR),
    static_cast<uint16_t>(Scale::BEBOP_DORIAN),
    static_cast<uint16_t>(Scale::BLUES),
    static_cast<uint16_t>(Scale::MINOR_PENTATONIC),
    static_cast<uint16_t>(Scale::HUNGARIAN_MINOR),
    static_cast<uint16_t>(Scale::UKRAINIAN_DORIAN),
    static_cast<uint16_t>(Scale::MARVA),
    static_cast<uint16_t>(Scale::TODI),
    static_cast<uint16_t>(Scale::WHOLE_TONE),
    static_cast<uint16_t>(Scale::CHROMATIC)};

constexpr const char* scaleNames[16] = {
    "Minor",
    "Major",
    "Dorian",
    "Phrygian",
    "Mixolydian",
    "Melodic Minor Ascending",
    "Harmonic Minor",
    "Bebop Dorian",
    "Blues",
    "Minor Pentatonic",
    "Hungarian Minor",
    "Ukrainian Dorian",
    "Marva",
    "Todi",
    "Whole Tone",
    "Chromatic"};

ScaleSelector::ScaleSelector(Color color) : color(color) {}

void ScaleSelector::SetScaleFunc(std::function<uint16_t()> func)
{
  getScale = func;
}

void ScaleSelector::OnChange(std::function<void(uint16_t)> callback)
{
  onChange = callback;
}

Color ScaleSelector::GetColor() { return color; }

Dimension ScaleSelector::GetSize() { return Dimension(8, 2); }

bool ScaleSelector::Render(Point origin)
{
  uint16_t current = getScale();
  uint16_t currentIdx = 0;
  for (uint16_t i = 0; i < 16; i++) {
    if (scales[i] == current) { currentIdx = i; break; }
  }

  for (uint16_t i = 0; i < 16; i++) {
    Point xy = origin + Point(i % 8, i / 8);
    bool lit = (i == currentIdx);
    MatrixOS::LED::SetColor(xy, lit ? color : color.Dim());
  }
  return true;
}

bool ScaleSelector::KeyEvent(Point xy, KeyInfo* keyInfo)
{
  if (keyInfo->State() == HOLD) {
    uint16_t idx = xy.x + xy.y * 8;
    if (idx < 16) {
      MatrixOS::UIUtility::TextScroll(scaleNames[idx], color);
      return true;
    }
  }

  if (keyInfo->State() == PRESSED) {
    uint16_t idx = xy.x + xy.y * 8;
    if (idx < 16 && onChange) {
      onChange(scales[idx]);
      return true;
    }
  }
  return false;
}
