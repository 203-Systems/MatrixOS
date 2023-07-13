
#pragma once

#include "MatrixOS.h"
#include "Scales.h"
#include "ui/UI.h"

struct NoteLayoutConfig {
  uint8_t rootKey = 0;
  bool enfourceScale = true; 
  bool alignRoot = true; //Only works when overlap is set to 0
  uint16_t scale = NATURAL_MINOR;
  int8_t octave = 0;
  uint8_t channel = 0;
  uint8_t overlap = 0;
  bool velocitySensitive = true;
  Color color = Color(0x00FFFF);
  Color rootColor = Color(0x0040FF);
};

class NotePad : public UIComponent {
 public:
  Dimension dimension;
  NoteLayoutConfig* config;
  std::vector<uint8_t> noteMap;
  std::unordered_map<uint8_t, uint8_t> activeNotes;

  virtual Color GetColor() { return config->rootColor; }
  virtual Dimension GetSize() { return dimension; }

  uint8_t InScale(uint8_t note) {
    note %= 12;

    if (note == config->rootKey)
      return 2;  // It is a root key

    uint16_t c_aligned_scale_map = ((config->scale << config->rootKey) + ((config->scale & 0xFFF) >> (12 - config->rootKey % 12))) & 0xFFF;  // Rootkey should  always < 12
    return bitRead(c_aligned_scale_map, note);
  }

  void GenerateKeymap() {
    noteMap.reserve(dimension.Area());

    uint8_t root = 12 * config->octave + config->rootKey;
    uint8_t nextNote = root;
    uint8_t rootCount = 0;
    for (int8_t y = 0; y < dimension.y; y++)
    {
      int8_t ui_y = dimension.y - y - 1;
      if(config->overlap && config->overlap < dimension.x)
      { 
        if(y != 0) nextNote = noteMap[(ui_y + 1) * dimension.x + config->overlap]; 
      }
      else if (config->alignRoot && rootCount >= 2)
      {
        root += 12;
        rootCount = 0;
        nextNote = root;
      }
      for (int8_t x = 0; x < dimension.x; x++)
      {
        uint8_t id = ui_y * dimension.x + x;
        if (nextNote > 127)
        {
          noteMap[id] = 255;
          continue;
        }
        if(!config->enfourceScale)
        {
          noteMap[id] = nextNote;  // Add to map
          nextNote++;
          continue;
        }
        while (true)  // Find next key that we should put in
        {
          uint8_t inScale = InScale(nextNote);
          if (inScale == 2)
          { rootCount++; }  // If root detected, inc rootCount
          if (inScale)      // If is in scale
          {
            noteMap[id] = nextNote;  // Add to map
            nextNote++;
            break;  // Break from inf loop
          }
          else
          {
            nextNote++;
            continue;  // Check next note
          }
        }
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
        { MatrixOS::LED::SetColor(globalPos, Color(0)); }
        else if (activeNotes.find(note) != activeNotes.end())  // If find the note is currently active. Show it as white
        { MatrixOS::LED::SetColor(globalPos, Color(0xFFFFFF)); }
        else
        {
          uint8_t inScale = InScale(note);  // Check if the note is in scale.
          if (inScale == 0)
          { MatrixOS::LED::SetColor(globalPos, Color(0)); }
          else if (inScale == 1)
          { MatrixOS::LED::SetColor(globalPos, config->color); }
          else if (inScale == 2)
          { MatrixOS::LED::SetColor(globalPos, config->rootColor); }
        }
        index++;
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    uint8_t note = noteMap[xy.y * dimension.x + xy.x];
    if (note == 255)
    { return false; }
    if (keyInfo->state == PRESSED)
    {
      MatrixOS::MIDI::Send(MidiPacket(0, NoteOn, config->channel, note, config->velocitySensitive ? keyInfo->velocity.to7bits() : 0x7F));
      activeNotes[note]++;  // If this key doesn't exist, unordered_map will auto assign it to 0.
    }
    else if (config->velocitySensitive && keyInfo->state == AFTERTOUCH)
    { MatrixOS::MIDI::Send(MidiPacket(0, AfterTouch, config->channel, note, keyInfo->velocity.to7bits())); }
    else if (keyInfo->state == RELEASED)
    {
      MatrixOS::MIDI::Send(MidiPacket(0, NoteOff, config->channel, note, 0));
      if (activeNotes[note]-- <= 1)
      { activeNotes.erase(note); }
    }
    return true;
  }

  NotePad(Dimension dimension, NoteLayoutConfig* config) {
    this->dimension = dimension;
    this->config = config;
    activeNotes.reserve(8);
    GenerateKeymap();
  }
};
