#include "MidiPacket.h"

// Constructor implementations
MidiPacket::MidiPacket()  // Place Holder data
{
   this->port = MIDI_PORT_INVALID;
   this->status = EMidiStatus::None;
}

MidiPacket::MidiPacket(EMidiStatus status, ...) {
  this->port = MIDI_PORT_INVALID;
  this->status = status;
  va_list valst;
  va_start(valst, status);
  switch (status)
  {
    case EMidiStatus::NoteOn:
    case EMidiStatus::NoteOff:
    case EMidiStatus::AfterTouch:
    case EMidiStatus::ControlChange:
      data[0] = (uint8_t)((status & 0xF0) | ((uint8_t)va_arg(valst, int) & 0x0f));
      data[1] = (uint8_t)va_arg(valst, int);
      data[2] = (uint8_t)va_arg(valst, int);
      break;
    case EMidiStatus::ProgramChange:
    case EMidiStatus::ChannelPressure:
      data[0] = (uint8_t)((status & 0xF0) | ((uint8_t)va_arg(valst, int) & 0x0f));
      data[1] = (uint8_t)va_arg(valst, int);
      break;
    case EMidiStatus::PitchChange:
    {
      data[0] = (uint8_t)((status & 0xF0) | ((uint8_t)va_arg(valst, int) & 0x0f));
      uint16_t value = (uint16_t)va_arg(valst, int);
      data[1] = (uint8_t)(value & 0x7F);
      data[2] = (uint8_t)((value >> 7) & 0x7F);
      break;
    }
    case EMidiStatus::MTCQuarterFrame:
    case EMidiStatus::SongSelect:
      data[0] = status;
      data[1] = (uint8_t)va_arg(valst, int);
      break;
    case EMidiStatus::SongPosition:
    {
      data[0] = status;
      uint16_t value = (uint16_t)va_arg(valst, int);
      data[1] = (uint8_t)(value & 0x7F);
      data[2] = (uint8_t)((value >> 7) & 0x7F);
      break;
    }
    case EMidiStatus::TuneRequest:
    case EMidiStatus::Sync:
    case EMidiStatus::Tick:
    case EMidiStatus::Start:
    case EMidiStatus::Continue:
    case EMidiStatus::Stop:
    case EMidiStatus::ActiveSense:
    case EMidiStatus::Reset:
    case EMidiStatus::SysExData:
    case EMidiStatus::SysExEnd:
      data[0] = status;
      break;
    default:
      data[0] = status;
      break;
  }
  va_end(valst);
}

// Static factory methods for channel messages
MidiPacket MidiPacket::NoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
  return MidiPacket(EMidiStatus::NoteOn, channel, note, velocity);
}

MidiPacket MidiPacket::NoteOff(uint8_t channel, uint8_t note, uint8_t velocity)
{
  return MidiPacket(EMidiStatus::NoteOff, channel, note, velocity);
}

MidiPacket MidiPacket::AfterTouch(uint8_t channel, uint8_t note, uint8_t pressure)
{
  return MidiPacket(EMidiStatus::AfterTouch, channel, note, pressure);
}

MidiPacket MidiPacket::ControlChange(uint8_t channel, uint8_t controller, uint8_t value)
{
  return MidiPacket(EMidiStatus::ControlChange, channel, controller, value);
}

MidiPacket MidiPacket::ProgramChange(uint8_t channel, uint8_t program)
{
  return MidiPacket(EMidiStatus::ProgramChange, channel, program);
}

MidiPacket MidiPacket::ChannelPressure(uint8_t channel, uint8_t pressure)
{
  return MidiPacket(EMidiStatus::ChannelPressure, channel, pressure);
}

MidiPacket MidiPacket::PitchBend(uint8_t channel, uint16_t value)
{
  return MidiPacket(EMidiStatus::PitchChange, channel, value);
}

// Static factory methods for system messages
MidiPacket MidiPacket::MTCQuarterFrame(uint8_t value)
{
  return MidiPacket(EMidiStatus::MTCQuarterFrame, value);
}

MidiPacket MidiPacket::SongPosition(uint16_t position)
{
  return MidiPacket(EMidiStatus::SongPosition, position);
}

MidiPacket MidiPacket::SongSelect(uint8_t song)
{
  return MidiPacket(EMidiStatus::SongSelect, song);
}

MidiPacket MidiPacket::TuneRequest()
{
  return MidiPacket(EMidiStatus::TuneRequest);
}

// Static factory methods for real-time messages
MidiPacket MidiPacket::Sync()
{
  return MidiPacket(EMidiStatus::Sync);
}

MidiPacket MidiPacket::Tick()
{
  return MidiPacket(EMidiStatus::Tick);
}

MidiPacket MidiPacket::Start()
{
  return MidiPacket(EMidiStatus::Start);
}

MidiPacket MidiPacket::Continue()
{
  return MidiPacket(EMidiStatus::Continue);
}

MidiPacket MidiPacket::Stop()
{
  return MidiPacket(EMidiStatus::Stop);
}

MidiPacket MidiPacket::ActiveSense()
{
  return MidiPacket(EMidiStatus::ActiveSense);
}

MidiPacket MidiPacket::Reset()
{
  return MidiPacket(EMidiStatus::Reset);
}

// Status methods
EMidiStatus MidiPacket::Status() const {
  if((uint8_t)status < EMidiStatus::SysExData)
  {
    return (EMidiStatus)data[0];
  }
  else
  {
    return (EMidiStatus)data[0];
  }
}

bool MidiPacket::SetStatus(EMidiStatus status)
{
  if((uint8_t)status < EMidiStatus::NoteOff)
  {
    return false;
  }

  this->status = status;
  if((uint8_t)status < EMidiStatus::SysExData)
  {
    data[0] = (data[0] & 0x0F) + status;
  }
  else
  {
    data[0] = status;
  }
  return true;
}

// Port methods
uint16_t MidiPacket::Port() const
{
  return port;
}

void MidiPacket::SetPort(uint16_t port_id)
{
  port = port_id;
}

// Channel methods
uint8_t MidiPacket::Channel() const {
  switch (status)
  {
    case EMidiStatus::NoteOn:
    case EMidiStatus::NoteOff:
    case EMidiStatus::AfterTouch:
    case EMidiStatus::ControlChange:
    case EMidiStatus::ProgramChange:
    case EMidiStatus::ChannelPressure:
    case EMidiStatus::PitchChange:
      return data[0] & 0x0F;
    default:
      return UINT8_MAX;
  }
}

bool MidiPacket::SetChannel(uint8_t channel)
{
  switch (status)
  {
    case EMidiStatus::NoteOn:
    case EMidiStatus::NoteOff:
    case EMidiStatus::AfterTouch:
    case EMidiStatus::ControlChange:
    case EMidiStatus::ProgramChange:
    case EMidiStatus::ChannelPressure:
    case EMidiStatus::PitchChange:
      // Update channel in data[0] while preserving status
      data[0] = (data[0] & 0xF0) | (channel & 0x0F);
      return true;
    default:
      return false;
  }
}

// Note methods
uint8_t MidiPacket::Note() const {
  switch (status)
  {
    case EMidiStatus::NoteOn:
    case EMidiStatus::NoteOff:
    case EMidiStatus::AfterTouch:
    case EMidiStatus::ControlChange:  // To be honest this shouldn't be here but close enough
    case EMidiStatus::ProgramChange:  // To be honest this shouldn't be here but close enough
      return data[1];
    default:
      return UINT8_MAX;
  }
}

bool MidiPacket::SetNote(uint8_t note)
{
  switch (status)
  {
    case EMidiStatus::NoteOn:
    case EMidiStatus::NoteOff:
    case EMidiStatus::AfterTouch:
    case EMidiStatus::ControlChange:
    case EMidiStatus::ProgramChange:
      data[1] = note & 0x7F;
      return true;
    default:
      return false;
  }
}

// Controller methods
uint8_t MidiPacket::Controller() const  // Just an alias for Note(), specially build for Program Change
{
  return Note();
}

bool MidiPacket::SetController(uint8_t controller)
{
  switch (status)
  {
    case EMidiStatus::NoteOn:
    case EMidiStatus::NoteOff:
    case EMidiStatus::AfterTouch:
    case EMidiStatus::ControlChange:
    case EMidiStatus::ProgramChange:
      data[1] = controller & 0x7F;
      return true;
    default:
      return false;
  }
}

// Velocity methods
uint8_t MidiPacket::Velocity() const {
  switch (status)
  {
    case EMidiStatus::NoteOn:
    case EMidiStatus::NoteOff:
    case EMidiStatus::AfterTouch:
    case EMidiStatus::ControlChange:  // Close Enough
      return data[2];
    case EMidiStatus::ChannelPressure:
      return data[1];
    default:
      return UINT8_MAX;
  }
}

bool MidiPacket::SetVelocity(uint8_t velocity)
{
  switch (status)
  {
    case EMidiStatus::NoteOn:
    case EMidiStatus::NoteOff:
    case EMidiStatus::AfterTouch:
    case EMidiStatus::ControlChange:
      data[2] = velocity & 0x7F;
      return true;
    case EMidiStatus::ChannelPressure:
      data[1] = velocity & 0x7F;
      return true;
    default:
      return false;
  }
}

// Value methods
uint16_t MidiPacket::Value() const  // Get value all type, basically a generic getter
{
  switch (status)
  {
    case EMidiStatus::NoteOn:  // Close enough
    case EMidiStatus::NoteOff:
    case EMidiStatus::AfterTouch:
    case EMidiStatus::ControlChange:
      return data[2];
    case EMidiStatus::ProgramChange:
    case EMidiStatus::ChannelPressure:
      return data[1];
    case EMidiStatus::PitchChange:
      return ((uint16_t)data[2] << 7) | data[1];
    case EMidiStatus::SongPosition:
      return ((uint16_t)data[1] << 7) | data[0];
    case EMidiStatus::SongSelect:
      return data[0];
    default:
      return UINT16_MAX;
  }
}

bool MidiPacket::SetValue(uint16_t value)
{
  switch (status)
  {
    case EMidiStatus::ControlChange:
      data[2] = value & 0x7F;
      return true;
    case EMidiStatus::ProgramChange:
    case EMidiStatus::ChannelPressure:
      data[1] = value & 0x7F;
      return true;
    case EMidiStatus::PitchChange:
    case EMidiStatus::SongPosition:
      data[1] = value & 0x7F;
      data[2] = (value >> 7) & 0x7F;
      return true;
    case EMidiStatus::SongSelect:
      data[1] = value & 0x7F;
      return true;
    default:
      return false;
  }
}

// Helper methods
uint8_t MidiPacket::Length() const {
  switch (status)
  {
    case EMidiStatus::NoteOn:
    case EMidiStatus::NoteOff:
    case EMidiStatus::AfterTouch:
    case EMidiStatus::ControlChange:
    case EMidiStatus::PitchChange:
    case EMidiStatus::SongPosition:
    case EMidiStatus::SysExData:
      return 3;
    case EMidiStatus::ProgramChange:
    case EMidiStatus::ChannelPressure:
    case EMidiStatus::SongSelect:
      return 2;
    case EMidiStatus::TuneRequest:
    case EMidiStatus::Sync:
    case EMidiStatus::Tick:
    case EMidiStatus::Start:
    case EMidiStatus::Continue:
    case EMidiStatus::Stop:
    case EMidiStatus::ActiveSense:
    case EMidiStatus::Reset:
      return 1;
    case EMidiStatus::SysExEnd:
    {
      if(data[0] == 0xF7)
        return 1;
      else if(data[1] == 0xF7)
        return 2;
      else
        return 3;
    }

    case EMidiStatus::None:
    default:
      return 0;
  }
}

bool MidiPacket::SysEx() const // Terrible name
{
  return status == EMidiStatus::SysExData || status == EMidiStatus::SysExEnd;
}

bool MidiPacket::SysExStart() const // Terrible name
{
  return status == EMidiStatus::SysExData && data[0] == MIDIv1_SYSEX_START;
}
