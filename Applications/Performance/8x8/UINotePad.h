#include "MatrixOS.h"

class UINotePad : public UIComponent {
 public:
  Color color;
  uint8_t channel;
  uint8_t* map;
  Dimension dimension;
  bool forceSensitive;

  UINotePad(Dimension dimension, Color color, uint8_t channel, uint8_t* map, bool forceSensitive) {
    this->color = color;
    this->channel = channel;
    this->dimension = dimension;
    this->map = map;
    this->forceSensitive = forceSensitive;
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
    Fract16 force = keyInfo->Force();
    if(!forceSensitive)
    {
      if(keyInfo->State() == AFTERTOUCH){
        return true;
      };
      if(keyInfo->Force() > 0){
        force = FRACT16_MAX;
      };
    }
    if (keyInfo->State() == PRESSED)
    { MatrixOS::MIDI::Send(MidiPacket::NoteOn(channel, note, force.to7bits()), MIDI_PORT_ALL); }
    else if (keyInfo->State() == AFTERTOUCH)
    { MatrixOS::MIDI::Send(MidiPacket::AfterTouch(channel, note, force.to7bits()), MIDI_PORT_ALL); }
    else if (keyInfo->State() == RELEASED)
    {
      MatrixOS::MIDI::Send(MidiPacket::NoteOn(channel, note, 0), MIDI_PORT_ALL);
    }
    return true;
  }

  void SetforceSensitive(bool forceSensitive) { this->forceSensitive = forceSensitive; }
};