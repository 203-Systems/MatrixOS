#include "DeveloperApp.h"

#include "MIDI/MIDI.h"

namespace
{
constexpr uint8_t PROTOCOL_VERSION = 0x01;
constexpr size_t HID_REPORT_SIZE = 32;
constexpr size_t HID_HEADER_SIZE = 4;
constexpr size_t MAX_SYSEX_SIZE = 256;

constexpr uint8_t COMMAND_PING = 0x00;
constexpr uint8_t COMMAND_SET_INPUT_REPORT = 0x01;
constexpr uint8_t COMMAND_EXIT_APP = 0x02;
constexpr uint8_t COMMAND_LED_WRITE_INDEX = 0x10;
constexpr uint8_t COMMAND_LED_WRITE_XY = 0x11;
constexpr uint8_t COMMAND_LED_WRITE_INDEX_BULK = 0x18;
constexpr uint8_t COMMAND_LED_WRITE_XY_BULK = 0x19;
constexpr uint8_t COMMAND_LED_CANVAS_UPDATE = 0x20;
constexpr uint8_t COMMAND_LED_CLEAR_SCREEN = 0x21;
constexpr uint8_t COMMAND_KEY_READ_INDEX = 0x30;
constexpr uint8_t COMMAND_KEY_READ_XY = 0x31;

constexpr uint8_t REPLY_ACK = 0x80;
constexpr uint8_t REPLY_ERROR = 0x81;
constexpr uint8_t REPLY_INPUT_EVENT = 0x90;

constexpr uint8_t STATUS_OK = 0x00;
constexpr uint8_t ERROR_BAD_VERSION = 0x01;
constexpr uint8_t ERROR_BAD_LENGTH = 0x02;
constexpr uint8_t ERROR_BAD_PAYLOAD = 0x03;
constexpr uint8_t ERROR_UNKNOWN_COMMAND = 0x7F;

constexpr uint8_t INPUT_REPORT_ALL = 0x00;
constexpr uint8_t INPUT_REPORT_KEY_INFO = 0x01;
constexpr uint8_t INPUT_EVENT_KEY_INFO = 0x01;

constexpr uint8_t LED_FLAG_CANVAS = 0x01;
constexpr uint8_t LED_FLAG_RGBW = 0x02;
constexpr uint8_t LED_FLAGS_MASK = LED_FLAG_CANVAS | LED_FLAG_RGBW;
constexpr uint8_t LED_CLEAR_FLAG_REFRESH = 0x01;

enum class Transport : uint8_t {
  HID,
  SysEx,
};

enum class CommandResult : uint8_t {
  OK,
  BadLength,
  BadPayload,
  UnknownCommand,
};

struct CommandReply {
  CommandResult result = CommandResult::OK;
  vector<uint8_t> data;
  bool exitApp = false;
};

uint8_t To7Bit(uint16_t value) {
  return (uint8_t)(value >> 9);
}

uint8_t To8BitFrom7Bit(uint8_t value) {
  value &= 0x7F;
  return (uint8_t)((value << 1) | (value >> 6));
}

uint8_t EncodeInt7(int16_t value) {
  if (value < -64)
  {
    value = -64;
  }
  else if (value > 63)
  {
    value = 63;
  }
  return value < 0 ? (uint8_t)(128 + value) : (uint8_t)value;
}

int8_t DecodeInt7(uint8_t value) {
  value &= 0x7F;
  return value >= 64 ? (int8_t)value - 128 : (int8_t)value;
}

uint16_t DecodeIndex(const uint8_t* payload, Transport transport) {
  if (transport == Transport::HID)
  {
    return ((uint16_t)payload[0] << 8) | payload[1];
  }
  return ((uint16_t)(payload[1] & 0x7F) << 7) | (payload[0] & 0x7F);
}

void AppendIndex(vector<uint8_t>* data, uint16_t index, Transport transport) {
  if (transport == Transport::HID)
  {
    data->push_back((uint8_t)(index >> 8));
    data->push_back((uint8_t)index);
    return;
  }

  data->push_back((uint8_t)(index & 0x7F));
  data->push_back((uint8_t)((index >> 7) & 0x7F));
}

bool Is7BitData(const uint8_t* data, size_t size) {
  for (size_t i = 0; i < size; i++)
  {
    if ((data[i] & 0x80) != 0)
    {
      return false;
    }
  }
  return true;
}

uint8_t ResultToErrorCode(CommandResult result) {
  switch (result)
  {
  case CommandResult::BadLength:
    return ERROR_BAD_LENGTH;
  case CommandResult::BadPayload:
    return ERROR_BAD_PAYLOAD;
  case CommandResult::UnknownCommand:
    return ERROR_UNKNOWN_COMMAND;
  case CommandResult::OK:
  default:
    return STATUS_OK;
  }
}

void SendHIDPacket(uint8_t seq, uint8_t command, const vector<uint8_t>& payload) {
  vector<uint8_t> report;
  report.reserve(HID_REPORT_SIZE);
  report.push_back(PROTOCOL_VERSION);
  report.push_back(seq);
  report.push_back(command);
  report.push_back((uint8_t)payload.size());
  report.insert(report.end(), payload.begin(), payload.end());
  MatrixOS::HID::RawHID::Send(report);
}

void SendHIDAck(uint8_t seq, uint8_t command, const vector<uint8_t>& data = {}) {
  vector<uint8_t> payload;
  payload.reserve(data.size() + 2);
  payload.push_back(command);
  payload.push_back(STATUS_OK);
  payload.insert(payload.end(), data.begin(), data.end());
  SendHIDPacket(seq, REPLY_ACK, payload);
}

void SendHIDError(uint8_t seq, uint8_t command, uint8_t errorCode) {
  SendHIDPacket(seq, REPLY_ERROR, vector<uint8_t>{command, errorCode});
}

bool SendSysExPacket(uint16_t port, uint8_t command, const vector<uint8_t>& payload) {
  vector<uint8_t> message;
  message.reserve(payload.size() + 8);
  message.push_back(MIDIv1_SYSEX_START);
  message.push_back(SYSEX_MFG_ID[0]);
  message.push_back(SYSEX_MFG_ID[1]);
  message.push_back(SYSEX_MFG_ID[2]);
  message.push_back(SYSEX_FAMILY_ID[0]);
  message.push_back(SYSEX_FAMILY_ID[1]);
  message.push_back(command);
  message.insert(message.end(), payload.begin(), payload.end());
  message.push_back(MIDIv1_SYSEX_END);
  return MatrixOS::MIDI::SendSysEx(port, (uint16_t)message.size(), message.data(), false);
}

void SendSysExAck(uint16_t port, uint8_t command, const vector<uint8_t>& data = {}) {
  vector<uint8_t> payload;
  payload.reserve(data.size() + 2);
  payload.push_back(command);
  payload.push_back(STATUS_OK);
  payload.insert(payload.end(), data.begin(), data.end());
  SendSysExPacket(port, REPLY_ACK, payload);
}

void SendSysExError(uint16_t port, uint8_t command, uint8_t errorCode) {
  SendSysExPacket(port, REPLY_ERROR, vector<uint8_t>{command, errorCode});
}

bool ValidateWriteFlags(uint8_t flags) {
  return (flags & ~LED_FLAGS_MASK) == 0;
}

size_t LedEntrySize(bool rgbw) {
  return rgbw ? 6 : 5;
}

Color DecodeColor(const uint8_t* payload, bool rgbw, Transport transport) {
  if (transport == Transport::HID)
  {
    return Color(payload[0], payload[1], payload[2], rgbw ? payload[3] : 0);
  }
  return Color(To8BitFrom7Bit(payload[0]), To8BitFrom7Bit(payload[1]), To8BitFrom7Bit(payload[2]),
               rgbw ? To8BitFrom7Bit(payload[3]) : 0);
}

bool WriteLedEntry(const uint8_t* entry, bool byIndex, bool rgbw, Transport transport) {
  size_t colorOffset = byIndex ? 2 : 2;
  Color color = DecodeColor(entry + colorOffset, rgbw, transport);

  if (byIndex)
  {
    uint16_t index = DecodeIndex(entry, transport);
    if (Device::LED::ID2Index(index) == UINT16_MAX)
    {
      return false;
    }
    MatrixOS::LED::SetColor(index, color);
    return true;
  }

  Point point;
  if (transport == Transport::HID)
  {
    point = Point((int8_t)entry[0], (int8_t)entry[1]);
  }
  else
  {
    point = Point(DecodeInt7(entry[0]), DecodeInt7(entry[1]));
  }

  if (Device::LED::XY2Index(point) == UINT16_MAX)
  {
    return false;
  }

  MatrixOS::LED::SetColor(point, color);
  return true;
}

CommandResult HandleLedWrite(const uint8_t* payload, size_t length, bool byIndex, bool bulk, Transport transport) {
  if (length < (bulk ? 2u : 1u))
  {
    return CommandResult::BadLength;
  }

  uint8_t flags = payload[0];
  if (!ValidateWriteFlags(flags))
  {
    return CommandResult::BadPayload;
  }

  bool rgbw = (flags & LED_FLAG_RGBW) != 0;
  bool canvasOnly = (flags & LED_FLAG_CANVAS) != 0;
  size_t entrySize = LedEntrySize(rgbw);
  size_t offset = bulk ? 2 : 1;
  uint8_t count = bulk ? payload[1] : 1;
  if (count == 0 || length != offset + entrySize * count)
  {
    return CommandResult::BadLength;
  }

  if (transport == Transport::SysEx && !Is7BitData(payload, length))
  {
    return CommandResult::BadPayload;
  }

  for (uint8_t i = 0; i < count; i++)
  {
    if (!WriteLedEntry(payload + offset + entrySize * i, byIndex, rgbw, transport))
    {
      return CommandResult::BadPayload;
    }
  }

  if (!canvasOnly)
  {
    MatrixOS::LED::Update();
  }
  return CommandResult::OK;
}

CommandResult HandleClearScreen(const uint8_t* payload, size_t length) {
  if (length > 1)
  {
    return CommandResult::BadLength;
  }

  uint8_t flags = length == 0 ? 0 : payload[0];
  if ((flags & ~LED_CLEAR_FLAG_REFRESH) != 0)
  {
    return CommandResult::BadPayload;
  }

  MatrixOS::LED::Fill(Color::Black);
  if ((flags & LED_CLEAR_FLAG_REFRESH) != 0)
  {
    MatrixOS::LED::Update();
  }
  return CommandResult::OK;
}

bool GetInputByIndex(uint16_t index, InputId* id) {
  if (index >= MatrixOS::LED::GetLEDCount())
  {
    return false;
  }

  Point point = Device::LED::Index2XY(index);
  if (!point)
  {
    return false;
  }

  const InputCluster* grid = MatrixOS::Input::GetPrimaryGridCluster();
  if (grid == nullptr)
  {
    return false;
  }

  return MatrixOS::Input::GetInputAt(grid->clusterId, point, id);
}

bool GetInputByXY(Point point, InputId* id) {
  const InputCluster* grid = MatrixOS::Input::GetPrimaryGridCluster();
  if (grid == nullptr)
  {
    return false;
  }
  return MatrixOS::Input::GetInputAt(grid->clusterId, point, id);
}

bool EncodeKeyInfo(InputId id, Transport transport, vector<uint8_t>* data) {
  InputSnapshot snapshot;
  if (!MatrixOS::Input::GetState(id, &snapshot) || snapshot.inputClass != InputClass::Keypad)
  {
    return false;
  }

  Point point = Point::Invalid();
  MatrixOS::Input::GetPosition(id, &point);

  data->push_back(INPUT_EVENT_KEY_INFO);
  data->push_back(id.clusterId);
  if (transport == Transport::HID)
  {
    data->push_back((uint8_t)(id.memberId >> 8));
    data->push_back((uint8_t)id.memberId);
    data->push_back((uint8_t)(int8_t)(point ? point.x : 0));
    data->push_back((uint8_t)(int8_t)(point ? point.y : 0));
    data->push_back((uint8_t)snapshot.keypad.state);
    data->push_back((uint8_t)(snapshot.keypad.pressure.value >> 8));
    data->push_back((uint8_t)snapshot.keypad.pressure.value);
    data->push_back((uint8_t)(snapshot.keypad.velocity.value >> 8));
    data->push_back((uint8_t)snapshot.keypad.velocity.value);
    return true;
  }

  data->push_back((uint8_t)(id.memberId & 0x7F));
  data->push_back((uint8_t)((id.memberId >> 7) & 0x7F));
  data->push_back(EncodeInt7(point ? point.x : 0));
  data->push_back(EncodeInt7(point ? point.y : 0));
  data->push_back((uint8_t)snapshot.keypad.state & 0x7F);
  data->push_back(To7Bit(snapshot.keypad.pressure.value));
  data->push_back(To7Bit(snapshot.keypad.velocity.value));
  return true;
}

CommandReply ExecuteCommand(uint8_t command, const uint8_t* payload, size_t length, Transport transport, bool* inputReportEnabled) {
  CommandReply reply;

  switch (command)
  {
  case COMMAND_PING: {
    if (length != 0)
    {
      reply.result = CommandResult::BadLength;
      return reply;
    }

    reply.data.push_back(PROTOCOL_VERSION);
    reply.data.push_back(Device::xSize);
    reply.data.push_back(Device::ySize);
    AppendIndex(&reply.data, (uint16_t)MatrixOS::LED::GetLEDCount(), transport);
    return reply;
  }
  case COMMAND_SET_INPUT_REPORT: {
    if (length != 2)
    {
      reply.result = CommandResult::BadLength;
      return reply;
    }

    uint8_t type = payload[0];
    if (type != INPUT_REPORT_ALL && type != INPUT_REPORT_KEY_INFO)
    {
      reply.result = CommandResult::BadPayload;
      return reply;
    }

    *inputReportEnabled = payload[1] != 0;
    return reply;
  }
  case COMMAND_EXIT_APP: {
    if (length != 0)
    {
      reply.result = CommandResult::BadLength;
      return reply;
    }
    reply.exitApp = true;
    return reply;
  }
  case COMMAND_LED_WRITE_INDEX:
    reply.result = HandleLedWrite(payload, length, true, false, transport);
    return reply;
  case COMMAND_LED_WRITE_XY:
    reply.result = HandleLedWrite(payload, length, false, false, transport);
    return reply;
  case COMMAND_LED_WRITE_INDEX_BULK:
    reply.result = HandleLedWrite(payload, length, true, true, transport);
    return reply;
  case COMMAND_LED_WRITE_XY_BULK:
    reply.result = HandleLedWrite(payload, length, false, true, transport);
    return reply;
  case COMMAND_LED_CANVAS_UPDATE: {
    if (length != 0)
    {
      reply.result = CommandResult::BadLength;
      return reply;
    }
    MatrixOS::LED::Update();
    return reply;
  }
  case COMMAND_LED_CLEAR_SCREEN:
    reply.result = HandleClearScreen(payload, length);
    return reply;
  case COMMAND_KEY_READ_INDEX: {
    if (length != 2)
    {
      reply.result = CommandResult::BadLength;
      return reply;
    }

    InputId id = InputId::Invalid();
    if (!GetInputByIndex(DecodeIndex(payload, transport), &id) || !EncodeKeyInfo(id, transport, &reply.data))
    {
      reply.result = CommandResult::BadPayload;
    }
    return reply;
  }
  case COMMAND_KEY_READ_XY: {
    if (length != 2)
    {
      reply.result = CommandResult::BadLength;
      return reply;
    }

    Point point;
    if (transport == Transport::HID)
    {
      point = Point((int8_t)payload[0], (int8_t)payload[1]);
    }
    else
    {
      if (!Is7BitData(payload, length))
      {
        reply.result = CommandResult::BadPayload;
        return reply;
      }
      point = Point(DecodeInt7(payload[0]), DecodeInt7(payload[1]));
    }

    InputId id = InputId::Invalid();
    if (!GetInputByXY(point, &id) || !EncodeKeyInfo(id, transport, &reply.data))
    {
      reply.result = CommandResult::BadPayload;
    }
    return reply;
  }
  default:
    reply.result = CommandResult::UnknownCommand;
    return reply;
  }
}

void SendHIDInputEvent(const InputEvent& event) {
  vector<uint8_t> data;
  if (EncodeKeyInfo(event.id, Transport::HID, &data))
  {
    SendHIDPacket(0, REPLY_INPUT_EVENT, data);
  }
}

void SendSysExInputEvent(uint16_t port, const InputEvent& event) {
  vector<uint8_t> data;
  if (EncodeKeyInfo(event.id, Transport::SysEx, &data))
  {
    SendSysExPacket(port, REPLY_INPUT_EVENT, data);
  }
}

bool InputEventToLedIndex(const InputEvent& event, uint8_t* index) {
  Point point = Point::Invalid();
  if (!MatrixOS::Input::GetPosition(event.id, &point))
  {
    return false;
  }

  uint16_t ledIndex = Device::LED::XY2Index(point);
  if (ledIndex == UINT16_MAX || ledIndex > 127)
  {
    return false;
  }

  *index = (uint8_t)ledIndex;
  return true;
}

void SendMIDIInputEvent(const InputEvent& event) {
  if (event.inputClass != InputClass::Keypad)
  {
    return;
  }

  uint8_t note = 0;
  if (!InputEventToLedIndex(event, &note))
  {
    return;
  }

  uint8_t velocity = To7Bit(event.keypad.velocity.value);
  uint8_t pressure = To7Bit(event.keypad.pressure.value);
  switch (event.keypad.state)
  {
  case KeypadState::Activated:
  case KeypadState::Pressed:
    MatrixOS::MIDI::Send(MidiPacket::NoteOn(0, note, velocity));
    break;
  case KeypadState::Hold:
  case KeypadState::Aftertouch:
    MatrixOS::MIDI::Send(MidiPacket::AfterTouch(0, note, pressure));
    break;
  case KeypadState::Released:
    MatrixOS::MIDI::Send(MidiPacket::NoteOff(0, note, velocity));
    break;
  default:
    break;
  }
}

void WriteMIDINoteLed(const MidiPacket& packet) {
  uint8_t note = packet.Note();
  if (note >= MatrixOS::LED::GetLEDCount())
  {
    return;
  }

  if (packet.Status() == EMidiStatus::NoteOff || packet.Velocity() == 0)
  {
    MatrixOS::LED::SetColor(note, Color::Black);
    MatrixOS::LED::Update();
    return;
  }

  uint8_t level = To8BitFrom7Bit(packet.Velocity());
  uint8_t channel = packet.Channel();
  Color color = channel == 0 ? Color(level, level, level)
                             : Color::HsvToRgb((float)(channel - 1) / 15.0f, 1.0f, (float)packet.Velocity() / 127.0f);
  MatrixOS::LED::SetColor(note, color);
  MatrixOS::LED::Update();
}

void HandleMIDIControlChange(const MidiPacket& packet) {
  if (packet.Channel() == 0 && packet.Controller() == 120)
  {
    MatrixOS::LED::Fill(Color::Black);
    MatrixOS::LED::Update();
  }
}
} // namespace

void DeveloperApp::Setup(const vector<string>& args) {
  (void)args;
  hidKeyReportEnabled = false;
  midiKeyReportEnabled = false;
  sysExKeyReportEnabled = false;
  sysExReportPort = MIDI_PORT_INVALID;
  sysExBuffer.reserve(MAX_SYSEX_SIZE);
}

void DeveloperApp::Loop() {
  DrainInput();
  DrainHID();
  DrainMIDI();
  MatrixOS::SYS::DelayMs(1);
}

void DeveloperApp::End() {
  sysExBuffer.clear();
  activeSysExPort = MIDI_PORT_INVALID;
  sysExReportPort = MIDI_PORT_INVALID;
}

void DeveloperApp::DrainInput() {
  InputEvent event;
  while (MatrixOS::Input::Get(&event, 0))
  {
    HandleInputEvent(event);
  }
}

void DeveloperApp::DrainHID() {
  uint8_t* report = nullptr;
  size_t size = 0;
  while ((size = MatrixOS::HID::RawHID::Get(&report, 0)) > 0)
  {
    HandleHIDReport(report, size);
  }
}

void DeveloperApp::DrainMIDI() {
  MidiPacket packet;
  while (MatrixOS::MIDI::Get(&packet, 0))
  {
    HandleMIDIPacket(packet);
  }
}

void DeveloperApp::HandleInputEvent(const InputEvent& event) {
  if (event.inputClass != InputClass::Keypad)
  {
    return;
  }

  if (hidKeyReportEnabled)
  {
    SendHIDInputEvent(event);
  }

  if (midiKeyReportEnabled)
  {
    SendMIDIInputEvent(event);
  }

  if (sysExKeyReportEnabled && sysExReportPort != MIDI_PORT_INVALID)
  {
    SendSysExInputEvent(sysExReportPort, event);
  }
}

void DeveloperApp::HandleHIDReport(const uint8_t* report, size_t size) {
  if (report == nullptr || size < HID_HEADER_SIZE)
  {
    SendHIDError(0, 0, ERROR_BAD_LENGTH);
    return;
  }

  uint8_t version = report[0];
  uint8_t seq = report[1];
  uint8_t command = report[2];
  uint8_t length = report[3];
  if (version != PROTOCOL_VERSION)
  {
    SendHIDError(seq, command, ERROR_BAD_VERSION);
    return;
  }

  if (length > size - HID_HEADER_SIZE || length > HID_REPORT_SIZE - HID_HEADER_SIZE)
  {
    SendHIDError(seq, command, ERROR_BAD_LENGTH);
    return;
  }

  CommandReply reply = ExecuteCommand(command, report + HID_HEADER_SIZE, length, Transport::HID, &hidKeyReportEnabled);
  if (reply.result == CommandResult::OK)
  {
    SendHIDAck(seq, command, reply.data);
    if (reply.exitApp)
    {
      Exit();
    }
    return;
  }

  SendHIDError(seq, command, ResultToErrorCode(reply.result));
}

void DeveloperApp::HandleMIDIPacket(const MidiPacket& packet) {
  switch (packet.Status())
  {
  case EMidiStatus::Start:
  case EMidiStatus::Continue:
    midiKeyReportEnabled = true;
    break;
  case EMidiStatus::NoteOn:
  case EMidiStatus::NoteOff:
    WriteMIDINoteLed(packet);
    break;
  case EMidiStatus::ControlChange:
    HandleMIDIControlChange(packet);
    break;
  case EMidiStatus::SysExData:
  case EMidiStatus::SysExEnd:
    HandleSysExPacket(packet);
    break;
  default:
    break;
  }
}

void DeveloperApp::HandleSysExPacket(const MidiPacket& packet) {
  if (packet.SysExStart())
  {
    sysExBuffer.clear();
    activeSysExPort = packet.Port();
  }
  else if (activeSysExPort != packet.Port())
  {
    return;
  }

  uint8_t length = packet.Length();
  if (sysExBuffer.size() + length > MAX_SYSEX_SIZE)
  {
    sysExBuffer.clear();
    activeSysExPort = MIDI_PORT_INVALID;
    return;
  }

  sysExBuffer.insert(sysExBuffer.end(), packet.data, packet.data + length);
  if (packet.Status() == EMidiStatus::SysExEnd)
  {
    HandleSysExMessage(packet.Port(), sysExBuffer);
    sysExBuffer.clear();
    activeSysExPort = MIDI_PORT_INVALID;
  }
}

void DeveloperApp::HandleSysExMessage(uint16_t port, const vector<uint8_t>& message) {
  if (message.size() < 8 || message[0] != MIDIv1_SYSEX_START || message.back() != MIDIv1_SYSEX_END || message[1] != SYSEX_MFG_ID[0] ||
      message[2] != SYSEX_MFG_ID[1] || message[3] != SYSEX_MFG_ID[2] || message[4] != SYSEX_FAMILY_ID[0] || message[5] != SYSEX_FAMILY_ID[1])
  {
    return;
  }

  uint8_t command = message[6];
  const uint8_t* payload = message.data() + 7;
  size_t length = message.size() - 8;
  if (!Is7BitData(payload, length))
  {
    SendSysExError(port, command, ERROR_BAD_PAYLOAD);
    return;
  }

  CommandReply reply = ExecuteCommand(command, payload, length, Transport::SysEx, &sysExKeyReportEnabled);
  if (reply.result == CommandResult::OK)
  {
    if (command == COMMAND_SET_INPUT_REPORT)
    {
      sysExReportPort = sysExKeyReportEnabled ? port : MIDI_PORT_INVALID;
    }
    SendSysExAck(port, command, reply.data);
    if (reply.exitApp)
    {
      Exit();
    }
    return;
  }

  SendSysExError(port, command, ResultToErrorCode(reply.result));
}
