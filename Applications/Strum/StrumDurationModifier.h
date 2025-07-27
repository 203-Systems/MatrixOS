#include "MatrixOS.h"
#include "ui/UI.h"

class StrumDurationModifier: public UIComponent {
 public:
  uint8_t count;
  uint16_t* note_length;

  const uint16_t step = 100;

  StrumDurationModifier(uint8_t count, uint16_t* note_length) {
    this->count = count;
    this->note_length = note_length;
  }

  virtual string GetName() { return "Duration Modifier"; }
  virtual Color GetColor() { return Color(0xFFB000); }
  virtual Dimension GetSize() { return Dimension(count, 1); }

  virtual bool Render(Point origin) {
    for (uint8_t duration = 0; duration < count; duration++)
    {
      // Maybe allow different direction
      Point xy = origin + Point(duration, 0);
      MatrixOS::LED::SetColor(xy, ((duration + 1) * step) <= *this->note_length ? GetColor() : GetColor().Dim());
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) 
  {
    if (keyInfo->state == HOLD)
    {
        MatrixOS::UIUtility::TextScroll(GetName(), GetColor());
        return true;
    }
    else if (keyInfo->state == PRESSED)
    { 
      *this->note_length = (xy.x + 1) * step;
      return true;
    }
    return true;
  }
};