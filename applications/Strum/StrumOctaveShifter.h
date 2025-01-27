#include "MatrixOS.h"
#include "ui/UI.h"

class StrumOctaveShifter : public UIComponent {
 public:
  uint8_t count;
  uint8_t* octave;

  const Color active_color = Color(0x0040FF);
  const Color color = Color(0x00FFFF);

  StrumOctaveShifter(uint8_t count, uint8_t* octave) {
    this->count = count;
    this->octave = octave;
  }

  virtual Color GetColor() { return Color(0xFF00FF); }
  virtual Dimension GetSize() { return Dimension(count, 1); }

  virtual bool Render(Point origin) {
    for (uint16_t octave = 0; octave < count; octave++)
    {
      // Maybe allow different direction
      Point xy = origin + Point(octave, 0);
      MatrixOS::LED::SetColor(xy, (octave == *this->octave) ? active_color : color);
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    // if (keyInfo->state == HOLD)
    // {
    //   MatrixOS::UIUtility::TextScroll(name, GetColor());
    //   return true;
    // }
    int8_t octave = xy.x;
    if (keyInfo->state == PRESSED)
    { *this->octave = octave; }
    return true;
  }
};