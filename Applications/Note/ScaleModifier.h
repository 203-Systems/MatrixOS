#include "MatrixOS.h"

class NoteScaleModifier : public UIComponent {
public:
  uint16_t* scale;
  UICallback<void(uint16_t)> changeCallback;
  Color color;
  Color rootColor;

  NoteScaleModifier(uint16_t* scale, Color color = Color(0x00FFFF), Color rootColor = Color(0x0040FF)) {
    this->scale = scale;
    this->color = color;
    this->rootColor = rootColor;
  }

  void ChangeScalePtr(uint16_t* scale) {
    this->scale = scale;
  }

  template <typename F> void OnChange(F&& f) {
    this->changeCallback = UICallback<void(uint16_t)>(static_cast<F&&>(f));
  }

  virtual void OnChangeCallback(uint16_t newScale) {
    if (changeCallback)
    {
      changeCallback(newScale);
    }
  }

  virtual Color GetColor() {
    return color;
  }
  virtual Dimension GetSize() {
    return Dimension(7, 2);
  }

  virtual bool Render(Point origin) {
    // Root is always 0, so we just use the scale directly
    uint16_t scaleMap = *scale & 0xFFF;

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
    return true;
  }

  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo) {
    if (xy == Point(0, 0) || xy == Point(3, 0))
      return false;

    if (keypadInfo->state == KeypadState::Hold)
    {
      MatrixOS::UIUtility::TextScroll("Custom Scale Modifier", color);
    }
    else if (keypadInfo->state == KeypadState::Released && keypadInfo->hold == false)
    {
      uint8_t note = xy.x * 2 + xy.y - 1 - (xy.x > 2);

      // Flip the bit for the selected note
      *scale ^= (1 << note);
      OnChangeCallback(*scale);
    }

    return true;
  }
};
