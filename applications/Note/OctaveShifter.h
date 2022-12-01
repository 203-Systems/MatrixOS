#include "MatrixOS.h"
#include "NotePad.h"

class OctaveShifter : public UIComponent {
 public:
  uint8_t count;
  NoteLayoutConfig* configs;
  uint8_t* activeConfig;

  OctaveShifter(uint8_t count, NoteLayoutConfig* configs, uint8_t* activeConfig) {
    this->count = count;
    this->configs = configs;
    this->activeConfig = activeConfig;
  }

  virtual Color GetColor() { return configs[*activeConfig].color; }
  virtual Dimension GetSize() { return Dimension(1, count); }

  virtual bool Render(Point origin) {
    for (uint16_t octave = 0; octave < count; octave++)
    {
      // Maybe allow different direction
      Point xy = origin + Point(0, count - octave - 1);
      MatrixOS::LED::SetColor(xy, (octave == configs[*activeConfig].octave) ? configs[*activeConfig].rootColor : configs[*activeConfig].color);
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    // if (keyInfo->state == HOLD)
    // {
    //   MatrixOS::UIInterface::TextScroll(name, GetColor());
    //   return true;
    // }
    int8_t octave = count - xy.y - 1;
    if (keyInfo->state == PRESSED)
    { configs[*activeConfig].octave = octave; }
    return true;
  }
};