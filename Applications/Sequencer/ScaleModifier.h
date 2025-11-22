#include "MatrixOS.h"

class SequenceScaleModifier : public UIComponent {
 public:
  uint16_t* scale;
  std::unique_ptr<std::function<void(uint16_t)>> changeCallback;
  Color color;
  Color rootColor;

  SequenceScaleModifier(uint16_t* scale, Color color = Color(0x00FFFF), Color rootColor = Color(0x0040FF)) {
    this->scale = scale;
    this->changeCallback = nullptr;
    this->color = color;
    this->rootColor = rootColor;
  }

  void ChangeScalePtr(uint16_t* scale)
  {
    this->scale = scale;
  }

  void OnChange(std::function<void(uint16_t)> changeCallback) {
    this->changeCallback = std::make_unique<std::function<void(uint16_t)>>(changeCallback);
  }

  virtual void OnChangeCallback(uint16_t newScale) {
    if (changeCallback != nullptr) {
      (*changeCallback)(newScale);
    }
  }

  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return Dimension(8, 2); }

  virtual bool Render(Point origin) {
    // Root is always 0, so we just use the scale directly
    uint16_t scale_map = *scale & 0xFFF;

    for (uint8_t note = 0; note < 12; note++)
    {
      Point xy = origin + ((note < 5) ? Point((note + 1) / 2, (note + 1) % 2) : Point((note + 2) / 2, note % 2));
      if (note == 0)
      { MatrixOS::LED::SetColor(xy, rootColor); }
      else if (bitRead(scale_map, note))
      { MatrixOS::LED::SetColor(xy, color); }
      else
      { MatrixOS::LED::SetColor(xy, color.DimIfNot()); }
    }
    MatrixOS::LED::SetColor(origin + Point(7, 1), rootColor);
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if(keyInfo->State() == HOLD)
    {
      MatrixOS::UIUtility::TextScroll("Custom Scale Modifier", color);
    }
    else if (keyInfo->State() == RELEASED && keyInfo->Hold() == false)
    {
      uint8_t note = xy.x * 2 + xy.y - 1 - (xy.x > 2);

      // Flip the bit for the selected note
      *scale ^= (1 << note);
      OnChangeCallback(*scale);
    }

    return true;
  }
};
