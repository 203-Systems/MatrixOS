#include <stdarg.h>
#include "MidiSpecs.h"
// #include "esp_log.h"

enum EMidiStatus : uint8_t {
  None = 0,
  NoteOff = MIDIv1_NOTE_OFF,
  NoteOn = MIDIv1_NOTE_ON,
  AfterTouch = MIDIv1_AFTER_TOUCH,
  ControlChange = MIDIv1_CONTROL_CHANGE,
  ProgramChange = MIDIv1_PROGRAM_CHANGE,
  ChannelPressure = MIDIv1_CHANNEL_PRESSURE,
  PitchChange = MIDIv1_PITCH_WHEEL,
  SongPosition = MIDIv1_SONG_POSITION_PTR,
  SongSelect = MIDIv1_SONG_SELECT,
  TuneRequest = MIDIv1_TUNE_REQUEST,
  Sync = MIDIv1_CLOCK,
  Tick = MIDIv1_TICK,
  Start = MIDIv1_START,
  Continue = MIDIv1_CONTINUE,
  Stop = MIDIv1_STOP,
  ActiveSense = MIDIv1_ACTIVE_SENSE,
  Reset = MIDIv1_RESET,
  SysExData = MIDIv1_SYSEX_START,
  SysExEnd = MIDIv1_SYSEX_END
};

// Port number can be any of the following class + 0~0xFF
enum EMidiPortID : uint16_t {
  MIDI_PORT_ALL_CLASS = 0x0,  // This is the default midi out mode, it will send midi from first of all output type
  MIDI_PORT_USB = 0x100,
  MIDI_PORT_PHYISCAL = 0x200,
  MIDI_PORT_BLUETOOTH = 0x300,
  MIDI_PORT_WIRELESS = 0x400,
  MIDI_PORT_RTP = 0x500,
  MIDI_PORT_DEVICE_CUSTOM = 0x600,
  MIDI_PORT_SYNTH = 0x8000,
  MIDI_PORT_INVAILD = 0xFFFF
};

struct MidiPacket {
  uint16_t port = MIDI_PORT_INVAILD;
  EMidiStatus status = None;
  uint8_t data[3] = {0, 0, 0};

  MidiPacket() {}  // Place Holder data

  MidiPacket(EMidiStatus status, ...) {
    va_list valst;
    MidiPacket(0, status, valst);
  }
  
  MidiPacket(uint16_t port, EMidiStatus status, ...) {
    this->port = port;
    this->status = status;
    va_list valst;
    va_start(valst, status);
    status = (EMidiStatus)(status & 0xF0);
    switch (status)
    {
      case NoteOn:
      case NoteOff:
      case AfterTouch:
      case ControlChange:
        data[0] = (uint8_t)(status | ((uint8_t)va_arg(valst, int) & 0x0f));
        data[1] = (uint8_t)va_arg(valst, int);
        data[2] = (uint8_t)va_arg(valst, int);
        break;
      case ProgramChange:
      case ChannelPressure:
        data[0] = (uint8_t)(status | ((uint8_t)va_arg(valst, int) & 0x0f));
        data[1] = (uint8_t)va_arg(valst, int);
        break;
      case PitchChange:
      {
        data[0] = (uint8_t)(status | ((uint8_t)va_arg(valst, int) & 0x0f));
        uint16_t pitch = (uint16_t)va_arg(valst, int);
        data[1] = (uint8_t)(pitch & 0x07F);
        data[2] = (uint8_t)((pitch >> 7) & 0x7f);
      }
      break;
      case SongSelect:
        data[0] = SongSelect;
        data[1] = (uint8_t)va_arg(valst, int);
        break;
      case SongPosition:
      {
        data[0] = SongPosition;
        uint16_t position = (uint16_t)va_arg(valst, int);
        data[1] = (uint8_t)(position & 0x07F);
        data[2] = (uint8_t)((position >> 7) & 0x7f);
        break;
      }
      case SysExData:
      case SysExEnd:
        data[0] = (uint8_t)va_arg(valst, int);
        data[1] = (uint8_t)va_arg(valst, int);
        data[2] = (uint8_t)va_arg(valst, int);
        break;
      case TuneRequest:
      case Sync:
      case Start:
      case Continue:
      case Stop:
      case ActiveSense:
      case Reset:
      case None:
      default:
        break;
    }
  }

  MidiPacket(EMidiStatus status, uint16_t length, uint8_t* data)  // I can prob use status to figure out length and
                                                                  // assign it automaticlly
  {
    MidiPacket(0, status, data);
  }

  MidiPacket(uint16_t port, EMidiStatus status, uint16_t length, uint8_t* data)  // I can prob use status to figure out
                                                                                 // length and assign it automaticlly
  {
    this->port = port;
    this->status = status;
    memcpy(this->data, data, length);
  }

  uint8_t channel() {
    switch (status)
    {
      case NoteOn:
      case NoteOff:
      case AfterTouch:
      case ControlChange:
      case ProgramChange:
      case ChannelPressure:
      case PitchChange:
        return data[0] & 0x0F;
      default:
        return 0;
    }
  }

  uint8_t note() {
    switch (status)
    {
      case NoteOn:
      case NoteOff:
      case AfterTouch:
      case ControlChange:  // To be honest this shouldn't be here but close enough
      case ProgramChange:  // To be honest this shouldn't be here but close enough
        return data[1];
      default:
        return 0;
    }
  }

  uint8_t controller()  // Just an alies for note(), specially build for Program Change
  {
    return note();
  }

  uint8_t velocity() {
    switch (status)
    {
      case NoteOn:
      case NoteOff:
      case AfterTouch:
      case ControlChange:  // Close Enough
        return data[2];
      case ChannelPressure:
        return data[1];
      default:
        return 0;
    }
  }

  uint16_t value()  // Get value all type, basiclly a generic getter
  {
    switch (status)
    {
      case NoteOn:  // Close enough
      case NoteOff:
      case AfterTouch:
      case ControlChange:
        return data[2];
      case ProgramChange:
      case ChannelPressure:
        return data[1];
      case PitchChange:
        return ((uint16_t)data[2] << 7) & data[1];
      case SongPosition:
        return ((uint16_t)data[1] << 7) & data[0];
      case SongSelect:
        return data[0];
      default:
        return 0;
    }
  }

  uint8_t Length() {
    switch (status)
    {
      case NoteOn:
      case NoteOff:
      case AfterTouch:
      case ControlChange:
      case PitchChange:
      case SongPosition:
      case SysExData:
        return 3;
      case ProgramChange:
      case ChannelPressure:
        return 2;
      case SongSelect:
        return 1;
      case SysExEnd:
      {
        if(data[0] == 0xF7)
          return 1;
        else if(data[1] == 0xF7)
          return 2;
        else
          return 3;
      }
      case TuneRequest:
      case Sync:
      case Start:
      case Continue:
      case Stop:
      case ActiveSense:
      case Reset:
      case None:
      default:
        return 0;
    }
  }

  bool SysEx()
  {
    return status == SysExData || status == SysExEnd;
  }

  bool SysExStart()
  {
    return status == SysExData && data[0] == 0xF0;
  }

  // bool SysExEnd()
  // {
  //   return status == SysExEnd;
  // }
};