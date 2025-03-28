#pragma once

#include "MatrixOS.h"
#include "Chord.h"
#include "ui/UI.h"

struct StrumBarConfig
{
  Color color = Color(0x00FFFF);
  EChord chord = EChord::MAJOR_TRIAD;
  uint8_t root = 0;
  uint8_t octave = 3;
  uint8_t midi_channel = 0;
  uint16_t note_length = 200;
  bool half_step = true;
};

class StrumBar : public UIComponent {
 public:
  StrumBarConfig* config;
  uint8_t keycache = 0;
  uint8_t activeNote = 255;
  queue<pair<uint32_t, uint8_t>> note_queue;

  uint8_t calculate_note_offset(EChord chord, uint8_t pos)
  {
    uint8_t notes_in_chord = __builtin_popcount(chord);
    uint8_t offset = 12 * (pos / notes_in_chord);
    
    uint8_t remaining_bits = pos % notes_in_chord + 1;
    for (uint8_t i = 0; i < 12; i++)
    {
      if (chord & (1 << i))
      {
        remaining_bits--;
        if (remaining_bits == 0)
        {
          return i + offset;
        }
      }
    }
    return 255; // Really should never happen
  }

  StrumBar(StrumBarConfig* config)
  {
    this->config = config;
  }

  virtual Color GetColor() { return config->color; }
  virtual Dimension GetSize() { return Dimension(1, 8); }

  void checkForExpiredNotes()
  {
    while (!note_queue.empty())
    {
      if (MatrixOS::SYS::Millis() - note_queue.front().first > config->note_length)
      {
        MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, NoteOff, config->midi_channel, note_queue.front().second, 0));
        note_queue.pop();
      }
      else
      {
        break;
      }
    }
  }

  virtual bool Render(Point origin) {
    uint8_t new_key_bitmap = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = origin + Point(0, i);
      bool key_state = MatrixOS::KEYPAD::GetKey(xy)->active();
      if (key_state)
      {
        new_key_bitmap |= 1 << i;
        MatrixOS::LED::SetColor(xy, config->color);
      }
    }

    checkForExpiredNotes();

    if(new_key_bitmap == keycache)
    {
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
      return true;
    }

    uint8_t pos = start * 2 + (size - 1);
    pos = 14 - pos;

    MLOGD("StrumBar", "Start %d Size %d Pos %d", start, size, pos);

    uint8_t note = config->root + 12 * (config->octave) + calculate_note_offset(config->chord, pos);

    if ((activeNote != note) && (note < 128))
    {
      MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, NoteOn, config->midi_channel, note, 0x7F));
      note_queue.push(std::make_pair(MatrixOS::SYS::Millis(), note));
      MLOGD("StrumBar", "Note on %d", note);
      activeNote = note;
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) { return true; };

};
