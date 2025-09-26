#pragma once

#include <stdarg.h>
#include "MidiSpecs.h"

enum EMidiStatus : uint8_t {
  None = 0,
  NoteOff = MIDIv1_NOTE_OFF,
  NoteOn = MIDIv1_NOTE_ON,
  AfterTouch = MIDIv1_AFTER_TOUCH,
  ControlChange = MIDIv1_CONTROL_CHANGE,
  ProgramChange = MIDIv1_PROGRAM_CHANGE,
  ChannelPressure = MIDIv1_CHANNEL_PRESSURE,
  PitchChange = MIDIv1_PITCH_WHEEL,
  MTCQuarterFrame = MIDIv1_MTC_QUARTER_FRAME,
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
  MIDI_PORT_EACH_CLASS = 0x0,  // This is the default midi out mode, it will send midi from first of all output type
  MIDI_PORT_ALL = 0x01, // Send to all ports
  MIDI_PORT_USB = 0x100,
  MIDI_PORT_PHYSICAL = 0x200,
  MIDI_PORT_BLUETOOTH = 0x300,
  MIDI_PORT_WIRELESS = 0x400,
  MIDI_PORT_RTP = 0x500,
  MIDI_PORT_DEVICE_CUSTOM = 0x600,
  MIDI_PORT_SYNTH = 0x8000,
  MIDI_PORT_OS = 0xF000,
  MIDI_PORT_INVALID = 0xFFFF
};

struct MidiPacket {
  uint16_t port = MIDI_PORT_INVALID; // Where the packet is coming from
  EMidiStatus status = None;  // We need this just in case we are in SysEx transfer. We can't tell if the payload is SysEx or message purely though data[0]
  uint8_t data[3] = {0, 0, 0};

  // Constructors
  MidiPacket();  // Place Holder data
  MidiPacket(EMidiStatus status, ...);

  // Static factory methods for channel messages
  static MidiPacket NoteOn(uint8_t channel, uint8_t note, uint8_t velocity = 127);
  static MidiPacket NoteOff(uint8_t channel, uint8_t note, uint8_t velocity = 0);
  static MidiPacket AfterTouch(uint8_t channel, uint8_t note, uint8_t pressure);
  static MidiPacket ControlChange(uint8_t channel, uint8_t controller, uint8_t value);
  static MidiPacket ProgramChange(uint8_t channel, uint8_t program);
  static MidiPacket ChannelPressure(uint8_t channel, uint8_t pressure);
  static MidiPacket PitchBend(uint8_t channel, uint16_t value);

  // Static factory methods for system messages
  static MidiPacket MTCQuarterFrame(uint8_t value);
  static MidiPacket SongPosition(uint16_t position);
  static MidiPacket SongSelect(uint8_t song);
  static MidiPacket TuneRequest();

  // Static factory methods for real-time messages
  static MidiPacket Sync();
  static MidiPacket Tick();
  static MidiPacket Start();
  static MidiPacket Continue();
  static MidiPacket Stop();
  static MidiPacket ActiveSense();
  static MidiPacket Reset();

  // Status methods
  EMidiStatus Status() const;
  bool SetStatus(EMidiStatus status);

  // Port methods
  uint16_t Port() const;
  void SetPort(uint16_t port_id);

  // Channel methods
  uint8_t Channel() const;
  bool SetChannel(uint8_t channel);

  // Note methods
  uint8_t Note() const;
  bool SetNote(uint8_t note);

  // Controller methods
  uint8_t Controller() const;  // Just an alias for Note(), specially build for Program Change
  bool SetController(uint8_t controller);

  // Velocity methods
  uint8_t Velocity() const;
  bool SetVelocity(uint8_t velocity);

  // Value methods
  uint16_t Value() const;  // Get value all type, basically a generic getter
  bool SetValue(uint16_t value);

  // Helper methods
  uint8_t Length() const;
  bool SysEx() const; // Checks if packet is part of SysEx transfer - Terrible name
  bool SysExStart() const; // Checks if packet is start of SysEx transfer - Terrible name as well
};