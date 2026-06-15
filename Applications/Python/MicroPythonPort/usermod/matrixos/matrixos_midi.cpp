// Exposes MatrixOS MIDI packet construction, send, and receive APIs to MicroPython.
#include "matrixos_common.h"
#include "matrixos_modules.h"

extern "C" {
#include "py/objstr.h"
#include "py/runtime.h"
}

using namespace MatrixOSPython;

extern const mp_obj_type_t matrixos_midi_packet_type;

typedef struct _matrixos_midi_packet_obj_t {
  mp_obj_base_t base;
  MidiPacket packet;
} matrixos_midi_packet_obj_t;

static matrixos_midi_packet_obj_t* ToMidiPacket(mp_obj_t packetObj) {
  if (!mp_obj_is_type(packetObj, &matrixos_midi_packet_type))
  {
    mp_raise_TypeError(MP_ERROR_TEXT("expected MidiPacket"));
  }
  return (matrixos_midi_packet_obj_t*)MP_OBJ_TO_PTR(packetObj);
}

static uint8_t ObjectToU7(mp_obj_t valueObj) {
  int value = mp_obj_get_int(valueObj);
  if (value < 0 || value > 127)
  {
    mp_raise_ValueError(MP_ERROR_TEXT("value must be 0..127"));
  }
  return (uint8_t)value;
}

static uint8_t ObjectToChannel(mp_obj_t channelObj) {
  int channel = mp_obj_get_int(channelObj);
  if (channel < 0 || channel > 15)
  {
    mp_raise_ValueError(MP_ERROR_TEXT("channel must be 0..15"));
  }
  return (uint8_t)channel;
}

static uint16_t ObjectToU14(mp_obj_t valueObj) {
  int value = mp_obj_get_int(valueObj);
  if (value < 0 || value > 16383)
  {
    mp_raise_ValueError(MP_ERROR_TEXT("value must be 0..16383"));
  }
  return (uint16_t)value;
}

static mp_obj_t MakeMidiPacket(const MidiPacket& packet) {
  matrixos_midi_packet_obj_t* self = mp_obj_malloc(matrixos_midi_packet_obj_t, &matrixos_midi_packet_type);
  self->packet = packet;
  return MP_OBJ_FROM_PTR(self);
}

static uint8_t PacketLength(const MidiPacket& packet) {
  switch (packet.Status())
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
  case EMidiStatus::MTCQuarterFrame:
  case EMidiStatus::SongSelect:
    return 2;
  case EMidiStatus::TuneRequest:
  case EMidiStatus::Clock:
  case EMidiStatus::Tick:
  case EMidiStatus::Start:
  case EMidiStatus::Continue:
  case EMidiStatus::Stop:
  case EMidiStatus::ActiveSense:
  case EMidiStatus::Reset:
    return 1;
  case EMidiStatus::SysExEnd:
    if (packet.data[0] == MIDIv1_SYSEX_END)
      return 1;
    if (packet.data[1] == MIDIv1_SYSEX_END)
      return 2;
    return 3;
  case EMidiStatus::None:
  default:
    return 0;
  }
}

mp_obj_t midi_packet_make_new(const mp_obj_type_t* type, size_t argc, size_t n_kw, const mp_obj_t* args) {
  (void)n_kw;

  matrixos_midi_packet_obj_t* self = mp_obj_malloc(matrixos_midi_packet_obj_t, type);
  if (argc == 0)
  {
    self->packet = MidiPacket();
  }
  else if (argc == 1)
  {
    self->packet = ToMidiPacket(args[0])->packet;
  }
  else
  {
    self->packet = MidiPacket((EMidiStatus)mp_obj_get_int(args[0]), argc > 1 ? mp_obj_get_int(args[1]) : 0, argc > 2 ? mp_obj_get_int(args[2]) : 0,
                              argc > 3 ? mp_obj_get_int(args[3]) : 0);
  }
  return MP_OBJ_FROM_PTR(self);
}

void midi_packet_print(const mp_print_t* print, mp_obj_t selfObj, mp_print_kind_t kind) {
  (void)kind;
  MidiPacket& packet = ToMidiPacket(selfObj)->packet;
  mp_printf(print, "MidiPacket(status=0x%02x, channel=%d, data=(%d, %d, %d), port=0x%04x)", (uint8_t)packet.Status(), packet.Channel(),
            packet.data[0], packet.data[1], packet.data[2], packet.Port());
}

mp_obj_t midi_packet_status(mp_obj_t selfObj) {
  return mp_obj_new_int((uint8_t)ToMidiPacket(selfObj)->packet.Status());
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_packet_status_obj, midi_packet_status);

mp_obj_t midi_packet_set_status(mp_obj_t selfObj, mp_obj_t statusObj) {
  return mp_obj_new_bool(ToMidiPacket(selfObj)->packet.SetStatus((EMidiStatus)mp_obj_get_int(statusObj)));
}
MP_DEFINE_CONST_FUN_OBJ_2(midi_packet_set_status_obj, midi_packet_set_status);

mp_obj_t midi_packet_port(mp_obj_t selfObj) {
  return mp_obj_new_int_from_uint(ToMidiPacket(selfObj)->packet.Port());
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_packet_port_obj, midi_packet_port);

mp_obj_t midi_packet_set_port(mp_obj_t selfObj, mp_obj_t portObj) {
  ToMidiPacket(selfObj)->packet.SetPort((uint16_t)mp_obj_get_int(portObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(midi_packet_set_port_obj, midi_packet_set_port);

mp_obj_t midi_packet_channel(mp_obj_t selfObj) {
  uint8_t channel = ToMidiPacket(selfObj)->packet.Channel();
  return channel == UINT8_MAX ? mp_const_none : mp_obj_new_int_from_uint(channel);
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_packet_channel_obj, midi_packet_channel);

mp_obj_t midi_packet_set_channel(mp_obj_t selfObj, mp_obj_t channelObj) {
  return mp_obj_new_bool(ToMidiPacket(selfObj)->packet.SetChannel(ObjectToChannel(channelObj)));
}
MP_DEFINE_CONST_FUN_OBJ_2(midi_packet_set_channel_obj, midi_packet_set_channel);

mp_obj_t midi_packet_note(mp_obj_t selfObj) {
  uint8_t note = ToMidiPacket(selfObj)->packet.Note();
  return note == UINT8_MAX ? mp_const_none : mp_obj_new_int_from_uint(note);
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_packet_note_obj, midi_packet_note);

mp_obj_t midi_packet_set_note(mp_obj_t selfObj, mp_obj_t noteObj) {
  return mp_obj_new_bool(ToMidiPacket(selfObj)->packet.SetNote(ObjectToU7(noteObj)));
}
MP_DEFINE_CONST_FUN_OBJ_2(midi_packet_set_note_obj, midi_packet_set_note);

mp_obj_t midi_packet_controller(mp_obj_t selfObj) {
  uint8_t controller = ToMidiPacket(selfObj)->packet.Controller();
  return controller == UINT8_MAX ? mp_const_none : mp_obj_new_int_from_uint(controller);
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_packet_controller_obj, midi_packet_controller);

mp_obj_t midi_packet_set_controller(mp_obj_t selfObj, mp_obj_t controllerObj) {
  return mp_obj_new_bool(ToMidiPacket(selfObj)->packet.SetController(ObjectToU7(controllerObj)));
}
MP_DEFINE_CONST_FUN_OBJ_2(midi_packet_set_controller_obj, midi_packet_set_controller);

mp_obj_t midi_packet_velocity(mp_obj_t selfObj) {
  uint8_t velocity = ToMidiPacket(selfObj)->packet.Velocity();
  return velocity == UINT8_MAX ? mp_const_none : mp_obj_new_int_from_uint(velocity);
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_packet_velocity_obj, midi_packet_velocity);

mp_obj_t midi_packet_set_velocity(mp_obj_t selfObj, mp_obj_t velocityObj) {
  return mp_obj_new_bool(ToMidiPacket(selfObj)->packet.SetVelocity(ObjectToU7(velocityObj)));
}
MP_DEFINE_CONST_FUN_OBJ_2(midi_packet_set_velocity_obj, midi_packet_set_velocity);

mp_obj_t midi_packet_value(mp_obj_t selfObj) {
  uint16_t value = ToMidiPacket(selfObj)->packet.Value();
  return value == UINT16_MAX ? mp_const_none : mp_obj_new_int_from_uint(value);
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_packet_value_obj, midi_packet_value);

mp_obj_t midi_packet_set_value(mp_obj_t selfObj, mp_obj_t valueObj) {
  return mp_obj_new_bool(ToMidiPacket(selfObj)->packet.SetValue(ObjectToU14(valueObj)));
}
MP_DEFINE_CONST_FUN_OBJ_2(midi_packet_set_value_obj, midi_packet_set_value);

mp_obj_t midi_packet_length(mp_obj_t selfObj) {
  return mp_obj_new_int_from_uint(PacketLength(ToMidiPacket(selfObj)->packet));
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_packet_length_obj, midi_packet_length);

mp_obj_t midi_packet_is_sysex(mp_obj_t selfObj) {
  return mp_obj_new_bool(ToMidiPacket(selfObj)->packet.SysEx());
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_packet_is_sysex_obj, midi_packet_is_sysex);

mp_obj_t midi_packet_is_sysex_start(mp_obj_t selfObj) {
  return mp_obj_new_bool(ToMidiPacket(selfObj)->packet.SysExStart());
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_packet_is_sysex_start_obj, midi_packet_is_sysex_start);

mp_obj_t midi_packet_data(mp_obj_t selfObj) {
  MidiPacket& packet = ToMidiPacket(selfObj)->packet;
  return mp_obj_new_tuple(3, (mp_obj_t[]){mp_obj_new_int_from_uint(packet.data[0]), mp_obj_new_int_from_uint(packet.data[1]),
                                          mp_obj_new_int_from_uint(packet.data[2])});
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_packet_data_obj, midi_packet_data);

static const mp_rom_map_elem_t midi_packet_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&midi_packet_status_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_status), MP_ROM_PTR(&midi_packet_set_status_obj)},
    {MP_ROM_QSTR(MP_QSTR_port), MP_ROM_PTR(&midi_packet_port_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_port), MP_ROM_PTR(&midi_packet_set_port_obj)},
    {MP_ROM_QSTR(MP_QSTR_channel), MP_ROM_PTR(&midi_packet_channel_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_channel), MP_ROM_PTR(&midi_packet_set_channel_obj)},
    {MP_ROM_QSTR(MP_QSTR_note), MP_ROM_PTR(&midi_packet_note_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_note), MP_ROM_PTR(&midi_packet_set_note_obj)},
    {MP_ROM_QSTR(MP_QSTR_controller), MP_ROM_PTR(&midi_packet_controller_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_controller), MP_ROM_PTR(&midi_packet_set_controller_obj)},
    {MP_ROM_QSTR(MP_QSTR_velocity), MP_ROM_PTR(&midi_packet_velocity_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_velocity), MP_ROM_PTR(&midi_packet_set_velocity_obj)},
    {MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&midi_packet_value_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_value), MP_ROM_PTR(&midi_packet_set_value_obj)},
    {MP_ROM_QSTR(MP_QSTR_length), MP_ROM_PTR(&midi_packet_length_obj)},
    {MP_ROM_QSTR(MP_QSTR_is_sysex), MP_ROM_PTR(&midi_packet_is_sysex_obj)},
    {MP_ROM_QSTR(MP_QSTR_is_sysex_start), MP_ROM_PTR(&midi_packet_is_sysex_start_obj)},
    {MP_ROM_QSTR(MP_QSTR_data), MP_ROM_PTR(&midi_packet_data_obj)},
};
MP_DEFINE_CONST_DICT(midi_packet_locals, midi_packet_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    matrixos_midi_packet_type,
    MP_QSTR_MidiPacket,
    MP_TYPE_FLAG_NONE,
    make_new, (const void*)midi_packet_make_new,
    print, (const void*)midi_packet_print,
    locals_dict, &midi_packet_locals
);

mp_obj_t midi_get(size_t argc, const mp_obj_t* args) {
  uint16_t timeoutMs = argc > 0 ? (uint16_t)mp_obj_get_int(args[0]) : 0;
  MidiPacket packet;
  if (!MatrixOS::MIDI::Get(&packet, timeoutMs))
  {
    return mp_const_none;
  }
  return MakeMidiPacket(packet);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(midi_get_obj, 0, 1, midi_get);

mp_obj_t midi_send(size_t argc, const mp_obj_t* args) {
  MidiPacket packet = ToMidiPacket(args[0])->packet;
  uint16_t targetPort = argc > 1 ? (uint16_t)mp_obj_get_int(args[1]) : MIDI_PORT_EACH_CLASS;
  uint16_t timeoutMs = argc > 2 ? (uint16_t)mp_obj_get_int(args[2]) : 0;
  return mp_obj_new_bool(MatrixOS::MIDI::Send(packet, targetPort, timeoutMs));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(midi_send_obj, 1, 3, midi_send);

mp_obj_t midi_send_sysex(size_t argc, const mp_obj_t* args) {
  uint16_t port = (uint16_t)mp_obj_get_int(args[0]);
  mp_buffer_info_t buffer;
  mp_get_buffer_raise(args[1], &buffer, MP_BUFFER_READ);
  bool includeMeta = argc > 2 ? mp_obj_is_true(args[2]) : true;
  return mp_obj_new_bool(MatrixOS::MIDI::SendSysEx(port, (uint16_t)buffer.len, (uint8_t*)buffer.buf, includeMeta));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(midi_send_sysex_obj, 2, 3, midi_send_sysex);

mp_obj_t midi_note_on(mp_obj_t channelObj, mp_obj_t noteObj, mp_obj_t velocityObj) {
  return MakeMidiPacket(MidiPacket::NoteOn(ObjectToChannel(channelObj), ObjectToU7(noteObj), ObjectToU7(velocityObj)));
}
MP_DEFINE_CONST_FUN_OBJ_3(midi_note_on_obj, midi_note_on);

mp_obj_t midi_note_off(mp_obj_t channelObj, mp_obj_t noteObj, mp_obj_t velocityObj) {
  return MakeMidiPacket(MidiPacket::NoteOff(ObjectToChannel(channelObj), ObjectToU7(noteObj), ObjectToU7(velocityObj)));
}
MP_DEFINE_CONST_FUN_OBJ_3(midi_note_off_obj, midi_note_off);

mp_obj_t midi_is_note_on(mp_obj_t packetObj) {
  MidiPacket& packet = ToMidiPacket(packetObj)->packet;
  return mp_obj_new_bool(packet.Status() == EMidiStatus::NoteOn && packet.Velocity() > 0);
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_is_note_on_obj, midi_is_note_on);

mp_obj_t midi_is_note_off(mp_obj_t packetObj) {
  MidiPacket& packet = ToMidiPacket(packetObj)->packet;
  return mp_obj_new_bool(packet.Status() == EMidiStatus::NoteOff || (packet.Status() == EMidiStatus::NoteOn && packet.Velocity() == 0));
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_is_note_off_obj, midi_is_note_off);

mp_obj_t midi_aftertouch(mp_obj_t channelObj, mp_obj_t noteObj, mp_obj_t pressureObj) {
  return MakeMidiPacket(MidiPacket::AfterTouch(ObjectToChannel(channelObj), ObjectToU7(noteObj), ObjectToU7(pressureObj)));
}
MP_DEFINE_CONST_FUN_OBJ_3(midi_aftertouch_obj, midi_aftertouch);

mp_obj_t midi_control_change(mp_obj_t channelObj, mp_obj_t controllerObj, mp_obj_t valueObj) {
  return MakeMidiPacket(MidiPacket::ControlChange(ObjectToChannel(channelObj), ObjectToU7(controllerObj), ObjectToU7(valueObj)));
}
MP_DEFINE_CONST_FUN_OBJ_3(midi_control_change_obj, midi_control_change);

mp_obj_t midi_program_change(mp_obj_t channelObj, mp_obj_t programObj) {
  return MakeMidiPacket(MidiPacket::ProgramChange(ObjectToChannel(channelObj), ObjectToU7(programObj)));
}
MP_DEFINE_CONST_FUN_OBJ_2(midi_program_change_obj, midi_program_change);

mp_obj_t midi_channel_pressure(mp_obj_t channelObj, mp_obj_t pressureObj) {
  return MakeMidiPacket(MidiPacket::ChannelPressure(ObjectToChannel(channelObj), ObjectToU7(pressureObj)));
}
MP_DEFINE_CONST_FUN_OBJ_2(midi_channel_pressure_obj, midi_channel_pressure);

mp_obj_t midi_pitch_bend(mp_obj_t channelObj, mp_obj_t valueObj) {
  return MakeMidiPacket(MidiPacket::PitchBend(ObjectToChannel(channelObj), ObjectToU14(valueObj)));
}
MP_DEFINE_CONST_FUN_OBJ_2(midi_pitch_bend_obj, midi_pitch_bend);

mp_obj_t midi_song_position(mp_obj_t positionObj) {
  return MakeMidiPacket(MidiPacket::SongPosition(ObjectToU14(positionObj)));
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_song_position_obj, midi_song_position);

mp_obj_t midi_song_select(mp_obj_t songObj) {
  return MakeMidiPacket(MidiPacket::SongSelect(ObjectToU7(songObj)));
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_song_select_obj, midi_song_select);

mp_obj_t midi_mtc_quarter_frame(mp_obj_t valueObj) {
  return MakeMidiPacket(MidiPacket::MTCQuarterFrame(ObjectToU7(valueObj)));
}
MP_DEFINE_CONST_FUN_OBJ_1(midi_mtc_quarter_frame_obj, midi_mtc_quarter_frame);

#define DEFINE_MIDI_ZERO_ARG_FACTORY(function_name, object_name, factory_expr) \
  mp_obj_t function_name() { return MakeMidiPacket(factory_expr); } \
  MP_DEFINE_CONST_FUN_OBJ_0(object_name, function_name)

DEFINE_MIDI_ZERO_ARG_FACTORY(midi_tune_request, midi_tune_request_obj, MidiPacket::TuneRequest());
DEFINE_MIDI_ZERO_ARG_FACTORY(midi_clock, midi_clock_obj, MidiPacket::Clock());
DEFINE_MIDI_ZERO_ARG_FACTORY(midi_tick, midi_tick_obj, MidiPacket::Tick());
DEFINE_MIDI_ZERO_ARG_FACTORY(midi_start, midi_start_obj, MidiPacket::Start());
DEFINE_MIDI_ZERO_ARG_FACTORY(midi_continue, midi_continue_obj, MidiPacket::Continue());
DEFINE_MIDI_ZERO_ARG_FACTORY(midi_stop, midi_stop_obj, MidiPacket::Stop());
DEFINE_MIDI_ZERO_ARG_FACTORY(midi_active_sense, midi_active_sense_obj, MidiPacket::ActiveSense());
DEFINE_MIDI_ZERO_ARG_FACTORY(midi_reset, midi_reset_obj, MidiPacket::Reset());

static const mp_rom_map_elem_t midi_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_MIDI)},
    {MP_ROM_QSTR(MP_QSTR_MidiPacket), MP_ROM_PTR(&matrixos_midi_packet_type)},
    {MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&midi_get_obj)},
    {MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&midi_send_obj)},
    {MP_ROM_QSTR(MP_QSTR_send_sysex), MP_ROM_PTR(&midi_send_sysex_obj)},
    {MP_ROM_QSTR(MP_QSTR_note_on), MP_ROM_PTR(&midi_note_on_obj)},
    {MP_ROM_QSTR(MP_QSTR_note_off), MP_ROM_PTR(&midi_note_off_obj)},
    {MP_ROM_QSTR(MP_QSTR_is_note_on), MP_ROM_PTR(&midi_is_note_on_obj)},
    {MP_ROM_QSTR(MP_QSTR_is_note_off), MP_ROM_PTR(&midi_is_note_off_obj)},
    {MP_ROM_QSTR(MP_QSTR_aftertouch), MP_ROM_PTR(&midi_aftertouch_obj)},
    {MP_ROM_QSTR(MP_QSTR_control_change), MP_ROM_PTR(&midi_control_change_obj)},
    {MP_ROM_QSTR(MP_QSTR_program_change), MP_ROM_PTR(&midi_program_change_obj)},
    {MP_ROM_QSTR(MP_QSTR_channel_pressure), MP_ROM_PTR(&midi_channel_pressure_obj)},
    {MP_ROM_QSTR(MP_QSTR_pitch_bend), MP_ROM_PTR(&midi_pitch_bend_obj)},
    {MP_ROM_QSTR(MP_QSTR_song_position), MP_ROM_PTR(&midi_song_position_obj)},
    {MP_ROM_QSTR(MP_QSTR_song_select), MP_ROM_PTR(&midi_song_select_obj)},
    {MP_ROM_QSTR(MP_QSTR_mtc_quarter_frame), MP_ROM_PTR(&midi_mtc_quarter_frame_obj)},
    {MP_ROM_QSTR(MP_QSTR_tune_request), MP_ROM_PTR(&midi_tune_request_obj)},
    {MP_ROM_QSTR(MP_QSTR_clock), MP_ROM_PTR(&midi_clock_obj)},
    {MP_ROM_QSTR(MP_QSTR_tick), MP_ROM_PTR(&midi_tick_obj)},
    {MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&midi_start_obj)},
    {MP_ROM_QSTR(MP_QSTR_continue_), MP_ROM_PTR(&midi_continue_obj)},
    {MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&midi_stop_obj)},
    {MP_ROM_QSTR(MP_QSTR_active_sense), MP_ROM_PTR(&midi_active_sense_obj)},
    {MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&midi_reset_obj)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_NONE), MP_ROM_INT(EMidiStatus::None)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_NOTE_OFF), MP_ROM_INT(EMidiStatus::NoteOff)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_NOTE_ON), MP_ROM_INT(EMidiStatus::NoteOn)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_AFTERTOUCH), MP_ROM_INT(EMidiStatus::AfterTouch)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_CONTROL_CHANGE), MP_ROM_INT(EMidiStatus::ControlChange)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_PROGRAM_CHANGE), MP_ROM_INT(EMidiStatus::ProgramChange)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_CHANNEL_PRESSURE), MP_ROM_INT(EMidiStatus::ChannelPressure)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_PITCH_BEND), MP_ROM_INT(EMidiStatus::PitchChange)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_MTC_QUARTER_FRAME), MP_ROM_INT(EMidiStatus::MTCQuarterFrame)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_SONG_POSITION), MP_ROM_INT(EMidiStatus::SongPosition)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_SONG_SELECT), MP_ROM_INT(EMidiStatus::SongSelect)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_TUNE_REQUEST), MP_ROM_INT(EMidiStatus::TuneRequest)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_CLOCK), MP_ROM_INT(EMidiStatus::Clock)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_TICK), MP_ROM_INT(EMidiStatus::Tick)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_START), MP_ROM_INT(EMidiStatus::Start)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_CONTINUE), MP_ROM_INT(EMidiStatus::Continue)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_STOP), MP_ROM_INT(EMidiStatus::Stop)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_ACTIVE_SENSE), MP_ROM_INT(EMidiStatus::ActiveSense)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_RESET), MP_ROM_INT(EMidiStatus::Reset)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_SYSEX_DATA), MP_ROM_INT(EMidiStatus::SysExData)},
    {MP_ROM_QSTR(MP_QSTR_STATUS_SYSEX_END), MP_ROM_INT(EMidiStatus::SysExEnd)},
    {MP_ROM_QSTR(MP_QSTR_PORT_EACH_CLASS), MP_ROM_INT(MIDI_PORT_EACH_CLASS)},
    {MP_ROM_QSTR(MP_QSTR_PORT_ALL), MP_ROM_INT(MIDI_PORT_ALL)},
    {MP_ROM_QSTR(MP_QSTR_PORT_USB), MP_ROM_INT(MIDI_PORT_USB)},
    {MP_ROM_QSTR(MP_QSTR_PORT_PHYSICAL), MP_ROM_INT(MIDI_PORT_PHYSICAL)},
    {MP_ROM_QSTR(MP_QSTR_PORT_BLUETOOTH), MP_ROM_INT(MIDI_PORT_BLUETOOTH)},
    {MP_ROM_QSTR(MP_QSTR_PORT_WIRELESS), MP_ROM_INT(MIDI_PORT_WIRELESS)},
    {MP_ROM_QSTR(MP_QSTR_PORT_RTP), MP_ROM_INT(MIDI_PORT_RTP)},
    {MP_ROM_QSTR(MP_QSTR_PORT_DEVICE_CUSTOM), MP_ROM_INT(MIDI_PORT_DEVICE_CUSTOM)},
    {MP_ROM_QSTR(MP_QSTR_PORT_SYNTH), MP_ROM_INT(MIDI_PORT_SYNTH)},
    {MP_ROM_QSTR(MP_QSTR_PORT_OS), MP_ROM_INT(MIDI_PORT_OS)},
    {MP_ROM_QSTR(MP_QSTR_PORT_INVALID), MP_ROM_INT(MIDI_PORT_INVALID)},
};
MP_DEFINE_CONST_DICT(midi_globals, midi_globals_table);

const mp_obj_module_t matrixos_midi_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&midi_globals,
};
