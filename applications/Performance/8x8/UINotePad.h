#include "MatrixOS.h"

class UINotePad : public UIComponent {
 public:
  Color color;
  uint8_t channel;
  uint8_t* map;
  Dimension dimension;

  UINotePad(Dimension dimension, Color color, uint8_t channel, uint8_t* map) {
    this->color = color;
    this->channel = channel;
    this->dimension = dimension;
    this->map = map;
  }

  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return dimension; }

  void SetChannel(uint8_t channel) { this->channel = channel; }
  void SetMap(uint8_t* map) { this->map = map; }
  void SetColor(Color color) { this->color = color; }

  virtual bool Render(Point origin) {
    for (uint8_t x = 0; x < dimension.x; x++)
    {
      for (uint8_t y = 0; y < dimension.y; y++)
      {
        Point target_coord = origin + Point(x, y);
        Color target_color = MatrixOS::KEYPAD::GetKey(target_coord)->active() ? Color(0xFFFFFF) : color;
        MatrixOS::LED::SetColor(target_coord, target_color);
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    uint8_t note = map[xy.y * dimension.x + xy.x];
    if (keyInfo->state == PRESSED)
    { MatrixOS::MIDI::Send(MidiPacket(0, NoteOn, channel, note, keyInfo->velocity.to7bits())); }
    else if (keyInfo->state == AFTERTOUCH)
    { MatrixOS::MIDI::Send(MidiPacket(0, AfterTouch, channel, note, keyInfo->velocity.to7bits())); }
    else if (keyInfo->state == RELEASED)
    {
      MatrixOS::MIDI::Send(MidiPacket(0, /*compatibilityMode ? NoteOn : */ NoteOff, channel, note,
                                      keyInfo->velocity.to7bits()));
    }
    return true;
  }
};