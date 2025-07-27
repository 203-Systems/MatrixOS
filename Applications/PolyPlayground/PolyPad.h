
#pragma once

#include "MatrixOS.h"
#include "ui/UI.h"

struct PolyPadConfig {
  uint8_t ySpacing = 4;
  uint8_t xSpacing = 3;
  uint8_t octave = 2;
  uint8_t channel = 0;
  uint8_t rootKey = 0;
  bool velocitySensitive = false;
};

class PolyPad : public UIComponent {
 public:
  Dimension dimension;
  PolyPadConfig* config;
  std::vector<uint8_t> noteMap;
  std::unordered_map<uint8_t, uint8_t> activeNotes;
  int8_t lastNote = -1;

  const Color noteColor[12] = {
      Color(0x00FFD9),
      Color(0xFF0097),
      Color(0xFFFB00),
      Color(0x5D00FF),
      Color(0xFF4B00),
      Color(0x009BFF),
      Color(0xFF003E),
      Color(0xAEFF00),
      Color(0xED00FF),
      Color(0xFFAE00),
      Color(0x1000FF),
      Color(0xFF1D00)
  };

  virtual Color GetColor() { return Color(0xAEFF00); }

  virtual Dimension GetSize() { return dimension; }

  void GenerateKeymap() {
    noteMap.reserve(dimension.Area());

    uint8_t root = 12 * config->octave + config->rootKey;

    for (int8_t y = 0; y < dimension.y; y++)
    {
      int8_t ui_y = dimension.y - y - 1; 

      uint8_t note = root + y * config->ySpacing;

      for (int8_t x = 0; x < dimension.x; x++)
      {
        uint8_t id = ui_y * dimension.x + x;

        if (note > 127)
        {
          noteMap[id] = 255;  // Mark as invalid
          continue;
        }

        noteMap[id] = note;
        note += config->xSpacing;  // Increment by xSpacing for each column
      }
    }
  }

  virtual bool Render(Point origin) {
    uint8_t index = 0;
    for (int8_t y = 0; y < dimension.y; y++)
    {
      for (int8_t x = 0; x < dimension.x; x++)
      {
        uint8_t note = noteMap[index];
        Point globalPos = origin + Point(x, y);
        if (note == 255)
        {
          MatrixOS::LED::SetColor(globalPos, Color(0));
        }
        else if (activeNotes.find(note) != activeNotes.end())  // If find the note is currently active. Show it as white
        {
          MatrixOS::LED::SetColor(globalPos, Color(0xFFFFFF));
        }
        else
        {
          uint8_t colorIndex = note % 12;
          MatrixOS::LED::SetColor(globalPos, noteColor[colorIndex]);
        }
        index++;
      }
    }

    if (lastNote != -1)
    {
      MatrixOS::LED::FillPartition("Underglow",  noteColor[lastNote % 12]);
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    uint8_t note = noteMap[xy.y * dimension.x + xy.x];
    if (note == 255)
    {
      return false;
    }
    if (keyInfo->state == PRESSED)
    {
      MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, NoteOn, config->channel, note, config->velocitySensitive ? keyInfo->velocity.to7bits() : 0x7F));
      lastNote = note;
      activeNotes[note]++;  // If this key doesn't exist, unordered_map will auto assign it to 0.
    }
    else if (config->velocitySensitive && keyInfo->state == AFTERTOUCH)
    {
      MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, AfterTouch, config->channel, note, keyInfo->velocity.to7bits()));
    }
    else if (keyInfo->state == RELEASED)
    {
      MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, NoteOff, config->channel, note, 0));
      if (activeNotes[note]-- <= 1)
      {
        activeNotes.erase(note);
      }
    }
    return true;
  }

  PolyPad(Dimension dimension, PolyPadConfig* config) {
    this->dimension = dimension;
    this->config = config;
    activeNotes.reserve(8);
    GenerateKeymap();
  }

  ~PolyPad() {
    MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, ControlChange, config->channel, 123, 0));  // All notes off
  }
};
