#include "MatrixOS.h"
#include "NotePad.h"

class OctaneShifter : public UIComponent {
 public:
  uint8_t count;
  string name;
  NoteLayoutConfig* configs;
  uint8_t* activeConfig;

  OctaneShifter(uint8_t count, string name, NoteLayoutConfig* configs, uint8_t* activeConfig) {
    this->name = name;
    this->count = count;
    this->configs = configs;
    this->activeConfig = activeConfig;
  }

  virtual Color GetColor() { return configs[*activeConfig].color; }
  virtual Dimension GetSize() { return Dimension(1, count); }

  virtual bool Render(Point origin) {
    for (uint16_t octane = 0; octane < count; octane++)
    {
      // Maybe allow different direction
      Point xy = origin + Point(0, count - octane - 1);
      MatrixOS::LED::SetColor(xy, (octane == configs[*activeConfig].octane) ? configs[*activeConfig].rootColor : configs[*activeConfig].color);
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    // if (keyInfo->state == HOLD)
    // {
    //   MatrixOS::UIInterface::TextScroll(name, GetColor());
    //   return true;
    // }
    int8_t octane = count - xy.y - 1;
    if (keyInfo->state == RELEASED)
    { configs[*activeConfig].octane = octane; }
    return true;
  }
};