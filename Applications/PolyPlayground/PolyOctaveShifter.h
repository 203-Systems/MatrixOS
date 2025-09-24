#include "MatrixOS.h"
#include "PolyPad.h"

class PolyOctaveShifter : public UIComponent {
 public:
  uint8_t count;
  PolyPadConfig* config;

  const Color active_color = Color(0xFF00FF);
  const Color color = Color(0x8800FF);

  PolyOctaveShifter(uint8_t count, PolyPadConfig* config) {
    this->count = count;
    this->config = config;
  }

  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return Dimension(1, count); }

  virtual bool Render(Point origin) {
    for (uint16_t octave = 0; octave < count; octave++)
    {
      // Maybe allow different direction
      Point xy = origin + Point(0, count - octave - 1);
      MatrixOS::LED::SetColor(xy, (octave == config->octave) ? active_color : color);
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    // if (keyInfo->State() == HOLD)
    // {
    //   MatrixOS::UIUtility::TextScroll(name, GetColor());
    //   return true;
    // }
    int8_t octave = count - xy.y - 1;
    if (keyInfo->State() == PRESSED)
    { config->octave = octave; }
    return true;
  }
};