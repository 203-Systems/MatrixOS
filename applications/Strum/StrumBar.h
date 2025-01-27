#include "MatrixOS.h"
#include "chord.h"
#include "ui/UI.h"

class StrumBar : public UIComponent {
 public:
  Color color;
  EChord* chord;
  uint8_t* root;
  uint8_t* octave;
  uint8_t* midi_channel;
  uint8_t keycache = 0;
  uint8_t activeNote = 255;
  EChord currentChord = (EChord)0;
  uint8_t currentRoot = 0;
  uint8_t currentOctave = 0;
  uint8_t noteMap[15];
  uint32_t releaseMillis = 0;

  const uint8_t sectionWidth[15][15] = {
      {15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
      {7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
      {5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {4, 4, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  
      {3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
      {2, 3, 2, 3, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {2, 2, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0},  
      {2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0}, 
      {2, 2, 1, 2, 2, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0},
      {1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 0, 0, 0, 0, 0},  
      {1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 0, 0, 0, 0}, 
      {1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0},
      {1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 0, 0},  
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0}, 
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };

  // Recursive function to build the pattern
  // void buildPattern(int level, const vector<int>& counts, const vector<int>& remainders, vector<int>& pattern) {
  //     if (level == -1) {
  //         pattern.push_back(0); // Append a rest
  //     } else if (level == -2) {
  //         pattern.push_back(1); // Append a pulse
  //     } else {
  //         for (int i = 0; i < counts[level]; ++i) {
  //             buildPattern(level - 1, counts, remainders, pattern);
  //         }
  //         if (remainders[level] != 0) {
  //             buildPattern(level - 2, counts, remainders, pattern);
  //         }
  //     }
  // }

  // // Function to calculate section widths
  // vector<int> calculateSectionWidth(int space, int segments) {
  //     if (segments > space) {
  //         MatrixOS::SYS::ErrorHandler("Segments cannot be greater than space");
  //     }

  //     // Initialize variables
  //     vector<int> pattern;
  //     vector<int> counts;
  //     vector<int> remainders;
  //     int divisor = space - segments;

  //     remainders.push_back(segments);
  //     int level = 0;

  //     // Build the counts and remainders
  //     while (true) {
  //         counts.push_back(divisor / remainders[level]);
  //         remainders.push_back(divisor % remainders[level]);
  //         divisor = remainders[level];
  //         level++;
  //         if (remainders[level] <= 1) {
  //             break;
  //         }
  //     }
  //     counts.push_back(divisor);

  //     // Build the pattern using the recursive function
  //     buildPattern(level, counts, remainders, pattern);

  //     // Rotate the pattern to start with a pulse (1)
  //     auto it = find(pattern.begin(), pattern.end(), 1);
  //     std::rotate(pattern.begin(), it, pattern.end());

  //     // Ensure the pattern size matches the space
  //     if (pattern.size() > space) {
  //         pattern.resize(space);
  //     }

  //     // Calculate distances between segments
  //     vector<int> distances;
  //     int pulse_start = -1;
  //     int pulse_end = 0;

  //     for (int i = 0; i < pattern.size(); ++i) {
  //         if (pattern[i] == 1) {
  //             if (pulse_start == -1) {
  //                 pulse_start = i;
  //             } else {
  //                 distances.push_back(i - pulse_end);
  //             }
  //             pulse_end = i;
  //         }
  //     }

  //     // Wrap around from the last pulse to the first pulse
  //     if (pulse_start != -1 && pulse_end != pulse_start) {
  //         distances.push_back(space - pulse_end + pulse_start);
  //     }

  //     return distances;
  // }

  void GenerateNoteMap()
  {
    uint8_t base_note = (*octave) * 12 + *root;
    uint8_t bitcounts = __builtin_popcount(*chord);
    uint8_t processed_bits = 0;
    uint8_t populated_sections = 0;
    // vector<int> section_widths = calculateSectionWidth(15, bitcounts);
    for (uint8_t i = 0; i < 32; i++)
    {
      if ((*chord & (1 << i)) == 0)
      {
        continue;
      }

      uint8_t note = base_note + i;

      MLOGD("StrumBar", "Index %d Note %d Section Width %d", processed_bits, note, sectionWidth[bitcounts - 1][processed_bits]);

      for (uint8_t j = 0; j < sectionWidth[bitcounts - 1][processed_bits]; j++)
      {
        noteMap[populated_sections] = note;
        populated_sections++;
      }

      processed_bits++;

      if(processed_bits == bitcounts)
      {
        break;
      }
    }

    MLOGD("StrumBar", "Generated note map - %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", noteMap[0], noteMap[1], noteMap[2], noteMap[3], noteMap[4], noteMap[5], noteMap[6], noteMap[7], noteMap[8], noteMap[9], noteMap[10], noteMap[11], noteMap[12], noteMap[13], noteMap[14]);

  }

  StrumBar(EChord* chord, uint8_t* root, uint8_t* octave, uint8_t* midi_channel, Color color = Color(0x00FFFF)) {
    this->chord = chord;
    this->root = root;
    this->octave = octave;
    this->midi_channel = midi_channel;
    this->color = color;
  }

  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return Dimension(1, 8); }

  virtual bool Render(Point origin) {
    if(*chord != currentChord || 
        *root != currentRoot || 
        *octave != currentOctave)
      {
        currentChord = *chord;
        currentRoot = *root;
        currentOctave = *octave;
        GenerateNoteMap();
      }

    uint8_t new_key_bitmap = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = origin + Point(0, i);
      bool key_state = MatrixOS::KEYPAD::GetKey(xy)->active();
      if (key_state)
      {
        new_key_bitmap |= 1 << i;
        MatrixOS::LED::SetColor(xy, color);
      }
    }

    if(new_key_bitmap == keycache)
    {
      if(keycache == 0 && activeNote != 255 && MatrixOS::SYS::Millis() - releaseMillis > 100)
      {
        MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, NoteOff, *midi_channel, activeNote, 0));
        MLOGD("StrumBar", "Note off %d", activeNote);
        activeNote = 255;
      }
      return true;
    }

    keycache = new_key_bitmap;

    uint8_t start = 0;
    uint8_t size = 0;
    uint8_t current_start = 0;
    uint8_t current_size = 0;
    bool prev = false;
    for(uint8_t i = 0; i < 9; i++)
    {
      bool key_state = i == 8 ? 0 : new_key_bitmap & (1 << i);
      if(key_state)
      {
        if(!prev)
        {
          current_start = i;
          current_size = 1;
        }
        else
        {
          current_size++;
        }
      }
      else
      {
        if(prev)
        {
          if(current_size > size)
          {
            start = current_start;
            size = current_size;
          }
        }
      }
      prev = key_state;
    }

    if(size == 0)
    {
      MLOGW("StrumBar", "No active keys");
      releaseMillis = MatrixOS::SYS::Millis();
      // if (activeNote != 255)
      // {
      //   MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, NoteOff, midi_channel, activeNote, 0));
      //   MLOGD("StrumBar", "Note off %d", activeNote);
      //   activeNote = 255;
      // }
      return true;
    }

    uint8_t pos = start * 2 + (size - 1);
    pos = 14 - pos;

    MLOGD("StrumBar", "Start %d Size %d Pos %d", start, size, pos);

    uint8_t note = noteMap[pos];

    if (activeNote != note)
    {
      if (activeNote != 255)
      {
        MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, NoteOff, midi_channel, activeNote, 0));
        MLOGD("StrumBar", "Note off %d", activeNote);
      }
      MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, NoteOn, midi_channel, note, 0x7F));
      MLOGD("StrumBar", "Note on %d", note);
      activeNote = note;
    }


    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) { return true; };

};
