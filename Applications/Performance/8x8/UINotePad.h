#include "MatrixOS.h"

class UINotePad : public UIComponent {
 public:
  Color color;
  uint8_t channel;
  uint8_t* map;
  Dimension dimension;
  bool velocitySensitive;

  UINotePad(Dimension dimension, Color color, uint8_t channel, uint8_t* map, bool velocitySensitive) {
    this->color = color;
    this->channel = channel;
    this->dimension = dimension;
    this->map = map;
    this->velocitySensitive = velocitySensitive;
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
        Color target_color = MatrixOS::KeyPad::GetKey(target_coord)->Active() ? Color(0xFFFFFF) : color;
        MatrixOS::LED::SetColor(target_coord, target_color);
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    uint8_t note = map[xy.y * dimension.x + xy.x];
    if(!velocitySensitive)
    {
      if(keyInfo->state == AFTERTOUCH){
        return true;
      };
      if(keyInfo->velocity > 0){
        keyInfo->velocity = FRACT16_MAX;
      };
    }
    if (keyInfo->state == PRESSED)
    { MatrixOS::MIDI::Send(MidiPacket(MIDI_PORT_ALL, NoteOn, channel, note, keyInfo->velocity.to7bits())); }
    else if (keyInfo->state == AFTERTOUCH)
    { MatrixOS::MIDI::Send(MidiPacket(MIDI_PORT_ALL, AfterTouch, channel, note, keyInfo->velocity.to7bits())); }
    else if (keyInfo->state == RELEASED)
    {
      MatrixOS::MIDI::Send(MidiPacket(MIDI_PORT_ALL, NoteOn, channel, note, 0));
    }
    return true;
  }

  void SetVelocitySensitive(bool velocitySensitive) { this->velocitySensitive = velocitySensitive; }
};