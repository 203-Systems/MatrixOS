
#pragma once

#include "MatrixOS.h"
#include "Scales.h"
#include "ui/UI.h"

enum NoteLayoutMode : uint8_t {
  OCTAVE_LAYOUT,
  OFFSET_LAYOUT,
  PIANO_LAYOUT,
};

enum NoteType : uint8_t {
  ROOT_NOTE,
  SCALE_NOTE,
  OFF_SCALE_NOTE,
};

struct NoteLayoutConfig {
  uint8_t rootKey = 0;
  uint16_t scale = NATURAL_MINOR;
  int8_t octave = 0;
  uint8_t channel = 0;
  NoteLayoutMode mode = OCTAVE_LAYOUT;
  bool inKeyNoteOnly = false;
  union {
    struct // Octave Mode
    {
      uint8_t unknown = 0;
    };
    struct // Offset Mode
    {
      uint8_t x_offset : 4;     // X offset for the note pad
      uint8_t y_offset : 4;     // Y offset for the note pad
    };
    struct // Piano Mode
    {
      uint8_t unknown2;
    };
  };
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
  uint16_t c_aligned_scale_map;

  virtual Color GetColor() { return config->rootColor; }
  virtual Dimension GetSize() { return dimension; }

  NoteType InScale(uint8_t note) {
    note %= 12;

    if (note == config->rootKey)
      return ROOT_NOTE;  // It is a root key
    return bitRead(c_aligned_scale_map, note) ? SCALE_NOTE : OFF_SCALE_NOTE;
  }

  uint8_t GetNextInScaleNote(uint8_t note) {
    for (int8_t i = 0; i < 12; i++)
    {
      note++;
      if (InScale(note) == SCALE_NOTE || InScale(note) == ROOT_NOTE)
      {
        return note;
      }
    }
    return UINT8_MAX;
  }

  // void GenerateOffsetKeymap() {
  //   noteMap.reserve(dimension.Area());
  //   uint8_t root = 12 * config->octave + config->rootKey;
  //   uint8_t nextNote = root;
  //   for (int8_t y = 0; y < dimension.y; y++)
  //   {
  //     int8_t ui_y = dimension.y - y - 1;

  //     if(config->overlap && config->overlap < dimension.x)
  //     { 
  //       if(y != 0) nextNote = noteMap[(ui_y + 1) * dimension.x + config->overlap]; 
  //     }

  //     for (int8_t x = 0; x < dimension.x; x++)
  //     {
  //       uint8_t id = ui_y * dimension.x + x;
  //       if (nextNote > 127) // If next note is out of range, fill with 255
  //       {
  //         noteMap[id] = 255;
  //       }
  //       else if(!config->enforceScale) // If enforce scale is false, just add the next note
  //       {
  //         noteMap[id] = nextNote;  // Add to map
  //         nextNote++;
  //       }
  //       else // If enforce scale is true, find the next note that is in scale
  //       {
  //         while (true)  // Find next key that we should put in
  //         {
  //           uint8_t inScale = InScale(nextNote);
  //           if (inScale == SCALE_NOTE || inScale == ROOT_NOTE)
  //           {
  //             noteMap[id] = nextNote;  // Add to map
  //             nextNote++;
  //             break;  // Break from inf loop
  //           }
  //           else if (inScale == OFF_SCALE_NOTE)
  //           {
  //             nextNote++;
  //             continue;  // Check next note
  //           }
  //         }
  //       }
  //     }
  //   }
  // }

  void GenerateOctaveKeymap() {
    noteMap.reserve(dimension.Area());
      uint8_t root = 12 * config->octave + config->rootKey;
      uint8_t nextNote = root;
      uint8_t rootCount = 0;
      for (int8_t y = 0; y < dimension.y; y++)
      {
        int8_t ui_y = dimension.y - y - 1;
        
        if (rootCount >= 2)
        {
          root += 12;
          rootCount = 0;
          nextNote = root;
        }

        for (int8_t x = 0; x < dimension.x; x++)
        {
          uint8_t id = ui_y * dimension.x + x;
          if (nextNote > 127) // If next note is out of range, fill with 255
          {
            noteMap[id] = 255;
          }
          else if(!config->inKeyNoteOnly) // If enforce scale is false, just add the next note
          {
            noteMap[id] = nextNote;  // Add to map
            nextNote++;
          }
          else // If enforce scale is true, find the next note that is in scale
          {
            while (true)  // Find next key that we should put in
            {
              uint8_t inScale = InScale(nextNote);
              if (inScale == ROOT_NOTE)  { rootCount++; }
              if (inScale == SCALE_NOTE || inScale == ROOT_NOTE)
              {
                noteMap[id] = nextNote;  // Add to map
                nextNote++;
                break;  // Break from inf loop
              }
              else if (inScale == OFF_SCALE_NOTE)
              {
                nextNote++;
                continue;  // Check next note
              }
            }
          }
        }
      }

  }

  void GenerateOffsetKeymap() {
    noteMap.reserve(dimension.Area());
    uint8_t root = 12 * config->octave + config->rootKey;
    if (!config->inKeyNoteOnly)
    {
      for (int8_t y = 0; y < dimension.y; y++)
      {
        int8_t ui_y = dimension.y - y - 1;
        for (int8_t x = 0; x < dimension.x; x++)
        {
          uint8_t note = root + config->x_offset * x + config->y_offset * y;
          noteMap[ui_y * dimension.x + x] = note;
        }
      }
    }
    else
    {
      for (uint8_t y = 0; y < dimension.y; y++)
      {
        int8_t ui_y = dimension.y - y - 1;
        uint8_t note = root;

        for (uint8_t x = 0; x < dimension.x; x++)
        {
          noteMap[ui_y * dimension.x + x] = note;
          for (uint8_t i = 0; i < config->x_offset; i++)
          {
            note = GetNextInScaleNote(note);
          }
        }

        for (uint8_t i = 0; i < config->y_offset; i++)
        {
          root = GetNextInScaleNote(root);
        }
      }
    }
  }    

  void GeneratePianoKeymap() {
    noteMap.reserve(dimension.Area());
    const int8_t blackKeys[7] = {-1, 1,  3, -1, 6, 8, 10};
    const int8_t whiteKeys[7] = {0,  2,  4,  5, 7, 9, 11};

    for (int8_t y = 0; y < dimension.y; y++)
    {
      int8_t ui_y = dimension.y - y - 1;
      uint8_t octave = config->octave + (y / 2);

      if(y % 2 == 0)  // Bottom row
      {
        for (int8_t x = 0; x < dimension.x; x++)
        {
          uint8_t id = ui_y * dimension.x + x;
          noteMap[id] = (octave + (x / 7)) * 12 + whiteKeys[x % 7];
        }
      }
      else // Top row
      {
        for (int8_t x = 0; x < dimension.x; x++)
        {
          uint8_t id = ui_y * dimension.x + x;
          int8_t offset = blackKeys[x % 7];
          if(offset == -1) 
          { noteMap[id] = 255; }
          else 
          { noteMap[id] = (octave + (x / 7)) * 12 + offset; }
        }
      }
    }
  }


  
  void GenerateKeymap() {
    c_aligned_scale_map = ((config->scale << config->rootKey) + ((config->scale & 0xFFF) >> (12 - config->rootKey % 12))) & 0xFFF;
    switch (config->mode)
    {
      case OCTAVE_LAYOUT:
        GenerateOctaveKeymap();
        break;
      case OFFSET_LAYOUT:
        GenerateOffsetKeymap();
        break;
      case PIANO_LAYOUT:
        GeneratePianoKeymap();
        break;
    }
  }
  virtual bool Render(Point origin) {
    uint8_t index = 0;
    Color color_dim = config->color.Dim(32);
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
          if (inScale == OFF_SCALE_NOTE)
          { MatrixOS::LED::SetColor(globalPos, color_dim); }
          else if (inScale == SCALE_NOTE)
          { MatrixOS::LED::SetColor(globalPos, config->color); }
          else if (inScale == ROOT_NOTE)
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
    { return true; }
    if (keyInfo->state == PRESSED)
    {
      MatrixOS::MIDI::Send(MidiPacket::NoteOn(config->channel, note, config->velocitySensitive ? keyInfo->velocity.to7bits() : 0x7F));
      activeNotes[note]++;  // If this key doesn't exist, unordered_map will auto assign it to 0.
    }
    else if (config->velocitySensitive && keyInfo->state == AFTERTOUCH)
    { MatrixOS::MIDI::Send(MidiPacket::AfterTouch(config->channel, note, keyInfo->velocity.to7bits())); }
    else if (keyInfo->state == RELEASED)
    {
      MatrixOS::MIDI::Send(MidiPacket::NoteOff(config->channel, note, 0));
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

  ~NotePad() {
    MatrixOS::MIDI::Send(MidiPacket::ControlChange(config->channel, 123, 0)); // All notes off
  }
};
