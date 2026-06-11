#include "Developer.h"

#include "MIDI/MIDI.h"

namespace
{
constexpr uint8_t PROTOCOL_VERSION = 0x01;
constexpr size_t HID_REPORT_SIZE = 63;
constexpr uint8_t MAX_HID_REPORTS_PER_LOOP = 4;
constexpr size_t MAX_SYSEX_SIZE = 256;

constexpr uint8_t COMMAND_PING = 0x00;
constexpr uint8_t COMMAND_SET_INPUT_REPORT = 0x01;
constexpr uint8_t COMMAND_EXIT_APP = 0x02;
constexpr uint8_t COMMAND_LED_WRITE_INDEX = 0x10;
constexpr uint8_t COMMAND_LED_WRITE_XY = 0x11;
constexpr uint8_t COMMAND_LED_WRITE_INDEX_RANGE = 0x12;
constexpr uint8_t COMMAND_LED_CANVAS_UPDATE = 0x20;
constexpr uint8_t COMMAND_LED_CLEAR_SCREEN = 0x21;
constexpr uint8_t COMMAND_KEY_READ_INDEX = 0x30;
constexpr uint8_t COMMAND_KEY_READ_XY = 0x31;

constexpr uint8_t REPLY_ACK = 0x80;
constexpr uint8_t REPLY_ERROR = 0x81;
constexpr uint8_t REPLY_INPUT_EVENT = 0x90;

constexpr uint8_t STATUS_OK = 0x00;
constexpr uint8_t ERROR_BAD_LENGTH = 0x02;
constexpr uint8_t ERROR_BAD_PAYLOAD = 0x03;
constexpr uint8_t ERROR_UNKNOWN_COMMAND = 0x7F;

constexpr uint8_t INPUT_REPORT_ALL = 0x00;
constexpr uint8_t INPUT_REPORT_KEY_INFO = 0x01;
constexpr uint8_t INPUT_EVENT_KEY_INFO = 0x01;

constexpr uint8_t LED_FLAG_CANVAS = 0x01;
constexpr uint8_t LED_FLAG_COLOR_MODE_SHIFT = 1;
constexpr uint8_t LED_FLAG_COLOR_MODE_MASK = 0x0E;
constexpr uint8_t LED_FLAGS_MASK = LED_FLAG_CANVAS | LED_FLAG_COLOR_MODE_MASK;
constexpr uint8_t LED_CLEAR_FLAG_REFRESH = 0x01;

constexpr uint8_t LED_COLOR_RGB24 = 0x00;
constexpr uint8_t LED_COLOR_RGBW32 = 0x01;
constexpr uint8_t LED_COLOR_RGB565 = 0x02;

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
  bool skipAck = false;
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

void SendHIDPacket(uint8_t command, const vector<uint8_t>& payload) {
  vector<uint8_t> report;
  report.reserve(HID_REPORT_SIZE);
  report.push_back(command);
  report.insert(report.end(), payload.begin(), payload.end());
  MatrixOS::HID::RawHID::Send(report);
}

void SendHIDReply(uint8_t command, uint8_t status, const vector<uint8_t>& data = {}) {
  vector<uint8_t> payload;
  payload.reserve(data.size() + 1);
  payload.push_back(status);
  payload.insert(payload.end(), data.begin(), data.end());
  SendHIDPacket(command | 0x80, payload);
}

void SendHIDAck(uint8_t command, const vector<uint8_t>& data = {}) {
  SendHIDReply(command, STATUS_OK, data);
}

void SendHIDError(uint8_t command, uint8_t errorCode) {
  SendHIDReply(command, errorCode);
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

uint8_t LedColorMode(uint8_t flags) {
  return (flags & LED_FLAG_COLOR_MODE_MASK) >> LED_FLAG_COLOR_MODE_SHIFT;
}

size_t LedColorSize(uint8_t colorMode, Transport transport) {
  switch (colorMode)
  {
  case LED_COLOR_RGB24:
    return 3;
  case LED_COLOR_RGBW32:
    return 4;
  case LED_COLOR_RGB565:
    return transport == Transport::HID ? 2 : 0;
  default:
    return 0;
  }
}

Color DecodeRgb565(uint16_t value) {
  uint8_t r5 = (value >> 11) & 0x1F;
  uint8_t g6 = (value >> 5) & 0x3F;
  uint8_t b5 = value & 0x1F;
  return Color((r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2));
}

Color DecodeColor(const uint8_t* payload, uint8_t colorMode, Transport transport) {
  switch (colorMode)
  {
  case LED_COLOR_RGB24:
    if (transport == Transport::HID)
    {
      return Color(payload[0], payload[1], payload[2]);
    }
    return Color(To8BitFrom7Bit(payload[0]), To8BitFrom7Bit(payload[1]), To8BitFrom7Bit(payload[2]));
  case LED_COLOR_RGBW32:
    if (transport == Transport::HID)
    {
      return Color(payload[0], payload[1], payload[2], payload[3]);
    }
    return Color(To8BitFrom7Bit(payload[0]), To8BitFrom7Bit(payload[1]), To8BitFrom7Bit(payload[2]), To8BitFrom7Bit(payload[3]));
  case LED_COLOR_RGB565:
    return DecodeRgb565(((uint16_t)payload[0] << 8) | payload[1]);
  default:
    return Color::Black;
  }
}

bool IsLedInPartition(uint16_t index, uint8_t partition) {
  if (partition >= Device::LED::partitions.size())
  {
    return false;
  }

  const LEDPartition& ledPartition = Device::LED::partitions[partition];
  return index >= ledPartition.start && index < ledPartition.start + ledPartition.size;
}

CommandResult PrepareLedWrite(const uint8_t* payload, size_t length, Transport transport, uint8_t* flags, uint8_t* partition, uint8_t* count,
                              size_t* colorSize) {
  if (length < 3)
  {
    return CommandResult::BadLength;
  }

  *flags = payload[0];
  *partition = payload[1];
  *count = payload[2];
  if (!ValidateWriteFlags(*flags))
  {
    return CommandResult::BadPayload;
  }
  if (*partition >= Device::LED::partitions.size())
  {
    return CommandResult::BadPayload;
  }

  *colorSize = LedColorSize(LedColorMode(*flags), transport);
  if (*colorSize == 0)
  {
    return CommandResult::BadPayload;
  }
  if (*count == 0)
  {
    return CommandResult::BadPayload;
  }
  return CommandResult::OK;
}

CommandResult HandleLedWriteIndex(const uint8_t* payload, size_t length, Transport transport) {
  uint8_t flags = 0;
  uint8_t partition = 0;
  uint8_t count = 0;
  size_t colorSize = 0;
  CommandResult result = PrepareLedWrite(payload, length, transport, &flags, &partition, &count, &colorSize);
  if (result != CommandResult::OK)
  {
    return result;
  }

  size_t entrySize = 2 + colorSize;
  size_t requiredLength = 3 + entrySize * count;
  if (length < requiredLength || (transport == Transport::SysEx && length != requiredLength))
  {
    return CommandResult::BadLength;
  }
  if (transport == Transport::SysEx && !Is7BitData(payload, length))
  {
    return CommandResult::BadPayload;
  }

  uint8_t colorMode = LedColorMode(flags);
  for (size_t i = 0; i < count; i++)
  {
    const uint8_t* entry = payload + 3 + entrySize * i;
    uint16_t index = DecodeIndex(entry, transport);
    if (Device::LED::ID2Index(index) == UINT16_MAX || !IsLedInPartition(index, partition))
    {
      return CommandResult::BadPayload;
    }
    MatrixOS::LED::SetColor(index, DecodeColor(entry + 2, colorMode, transport));
  }

  if ((flags & LED_FLAG_CANVAS) == 0)
  {
    MatrixOS::LED::Update();
  }
  return CommandResult::OK;
}

CommandResult HandleLedWriteXY(const uint8_t* payload, size_t length, Transport transport) {
  uint8_t flags = 0;
  uint8_t partition = 0;
  uint8_t count = 0;
  size_t colorSize = 0;
  CommandResult result = PrepareLedWrite(payload, length, transport, &flags, &partition, &count, &colorSize);
  if (result != CommandResult::OK)
  {
    return result;
  }

  size_t entrySize = 2 + colorSize;
  size_t requiredLength = 3 + entrySize * count;
  if (length < requiredLength || (transport == Transport::SysEx && length != requiredLength))
  {
    return CommandResult::BadLength;
  }
  if (transport == Transport::SysEx && !Is7BitData(payload, length))
  {
    return CommandResult::BadPayload;
  }

  uint8_t colorMode = LedColorMode(flags);
  for (size_t i = 0; i < count; i++)
  {
    const uint8_t* entry = payload + 3 + entrySize * i;
    Point point = transport == Transport::HID ? Point((int8_t)entry[0], (int8_t)entry[1]) : Point(DecodeInt7(entry[0]), DecodeInt7(entry[1]));
    uint16_t index = Device::LED::XY2Index(point);
    if (index == UINT16_MAX || !IsLedInPartition(index, partition))
    {
      return CommandResult::BadPayload;
    }
    MatrixOS::LED::SetColor(point, DecodeColor(entry + 2, colorMode, transport));
  }

  if ((flags & LED_FLAG_CANVAS) == 0)
  {
    MatrixOS::LED::Update();
  }
  return CommandResult::OK;
}

CommandResult HandleLedWriteIndexRange(const uint8_t* payload, size_t length, Transport transport) {
  uint8_t flags = 0;
  uint8_t partition = 0;
  uint8_t count = 0;
  if (length < 5)
  {
    return CommandResult::BadLength;
  }

  flags = payload[0];
  partition = payload[1];
  if (!ValidateWriteFlags(flags) || partition >= Device::LED::partitions.size())
  {
    return CommandResult::BadPayload;
  }

  uint16_t startIndex = DecodeIndex(payload + 2, transport);
  count = payload[4];
  if (count == 0)
  {
    return CommandResult::BadPayload;
  }

  size_t colorSize = 0;
  colorSize = LedColorSize(LedColorMode(flags), transport);
  if (colorSize == 0)
  {
    return CommandResult::BadPayload;
  }

  size_t requiredLength = 5 + colorSize * count;
  if (length < requiredLength || (transport == Transport::SysEx && length != requiredLength))
  {
    return CommandResult::BadLength;
  }
  if (transport == Transport::SysEx && !Is7BitData(payload, length))
  {
    return CommandResult::BadPayload;
  }

  uint16_t ledCount = MatrixOS::LED::GetLEDCount();
  if (startIndex >= ledCount || (uint32_t)startIndex + count > ledCount)
  {
    return CommandResult::BadPayload;
  }
  if (!IsLedInPartition(startIndex, partition) || !IsLedInPartition((uint16_t)(startIndex + count - 1), partition))
  {
    return CommandResult::BadPayload;
  }

  uint8_t colorMode = LedColorMode(flags);
  const uint8_t* colors = payload + 5;
  for (size_t i = 0; i < count; i++)
  {
    MatrixOS::LED::SetColor((uint16_t)(startIndex + i), DecodeColor(colors + colorSize * i, colorMode, transport));
  }

  if ((flags & LED_FLAG_CANVAS) == 0)
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

bool GetHIDPayloadLength(uint8_t command, const uint8_t* payload, size_t size, size_t* length) {
  switch (command)
  {
  case COMMAND_PING:
  case COMMAND_EXIT_APP:
  case COMMAND_LED_CANVAS_UPDATE:
    *length = 0;
    return true;
  case COMMAND_SET_INPUT_REPORT:
  case COMMAND_KEY_READ_INDEX:
  case COMMAND_KEY_READ_XY:
    if (size < 2)
    {
      return false;
    }
    *length = 2;
    return true;
  case COMMAND_LED_CLEAR_SCREEN:
    *length = size == 0 ? 0 : 1;
    return true;
  case COMMAND_LED_WRITE_INDEX:
  case COMMAND_LED_WRITE_XY: {
    if (size < 3)
    {
      return false;
    }
    size_t colorSize = LedColorSize(LedColorMode(payload[0]), Transport::HID);
    if (colorSize == 0 || payload[2] == 0)
    {
      *length = 3;
      return true;
    }
    size_t requiredLength = 3 + (2 + colorSize) * payload[2];
    if (size < requiredLength)
    {
      return false;
    }
    *length = requiredLength;
    return true;
  }
  case COMMAND_LED_WRITE_INDEX_RANGE: {
    if (size < 5)
    {
      return false;
    }
    size_t colorSize = LedColorSize(LedColorMode(payload[0]), Transport::HID);
    if (colorSize == 0 || payload[4] == 0)
    {
      *length = 5;
      return true;
    }
    size_t requiredLength = 5 + colorSize * payload[4];
    if (size < requiredLength)
    {
      return false;
    }
    *length = requiredLength;
    return true;
  }
  default:
    *length = size;
    return true;
  }
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

bool EncodeKeyInfo(InputId id, const KeypadInfo& keypad, Transport transport, vector<uint8_t>* data) {
  const InputCluster* grid = MatrixOS::Input::GetPrimaryGridCluster();
  bool isFunctionKey = id == InputId::FunctionKey();
  bool isGridKey = grid != nullptr && id.clusterId == grid->clusterId;
  if (!isFunctionKey && !isGridKey)
  {
    return false;
  }

  Point point = Point::Invalid();
  if (isGridKey && (!MatrixOS::Input::GetPosition(id, &point) || !point))
  {
    return false;
  }
  if (isFunctionKey)
  {
    point = Point::Origin();
  }

  data->push_back(INPUT_EVENT_KEY_INFO);
  data->push_back(id.clusterId);
  if (transport == Transport::HID)
  {
    data->push_back((uint8_t)(id.memberId >> 8));
    data->push_back((uint8_t)id.memberId);
    data->push_back((uint8_t)(int8_t)point.x);
    data->push_back((uint8_t)(int8_t)point.y);
    data->push_back((uint8_t)keypad.state);
    data->push_back((uint8_t)(keypad.pressure.value >> 8));
    data->push_back((uint8_t)keypad.pressure.value);
    data->push_back((uint8_t)(keypad.velocity.value >> 8));
    data->push_back((uint8_t)keypad.velocity.value);
    return true;
  }

  data->push_back((uint8_t)(id.memberId & 0x7F));
  data->push_back((uint8_t)((id.memberId >> 7) & 0x7F));
  data->push_back(EncodeInt7(point.x));
  data->push_back(EncodeInt7(point.y));
  data->push_back((uint8_t)keypad.state & 0x7F);
  data->push_back(To7Bit(keypad.pressure.value));
  data->push_back(To7Bit(keypad.velocity.value));
  return true;
}

bool EncodeKeyInfo(InputId id, Transport transport, vector<uint8_t>* data) {
  InputSnapshot snapshot;
  if (!MatrixOS::Input::GetState(id, &snapshot) || snapshot.inputClass != InputClass::Keypad)
  {
    return false;
  }

  return EncodeKeyInfo(id, snapshot.keypad, transport, data);
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
    reply.result = HandleLedWriteIndex(payload, length, transport);
    reply.skipAck = reply.result == CommandResult::OK;
    return reply;
  case COMMAND_LED_WRITE_XY:
    reply.result = HandleLedWriteXY(payload, length, transport);
    reply.skipAck = reply.result == CommandResult::OK;
    return reply;
  case COMMAND_LED_WRITE_INDEX_RANGE:
    reply.result = HandleLedWriteIndexRange(payload, length, transport);
    reply.skipAck = reply.result == CommandResult::OK;
    return reply;
  case COMMAND_LED_CANVAS_UPDATE: {
    if (length != 0)
    {
      reply.result = CommandResult::BadLength;
      return reply;
    }
    MatrixOS::LED::Update();
    reply.skipAck = true;
    return reply;
  }
  case COMMAND_LED_CLEAR_SCREEN:
    reply.result = HandleClearScreen(payload, length);
    reply.skipAck = reply.result == CommandResult::OK;
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
  if (EncodeKeyInfo(event.id, event.keypad, Transport::HID, &data))
  {
    SendHIDPacket(REPLY_INPUT_EVENT, data);
  }
}

void SendSysExInputEvent(uint16_t port, const InputEvent& event) {
  vector<uint8_t> data;
  if (EncodeKeyInfo(event.id, event.keypad, Transport::SysEx, &data))
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

void Developer::Setup(const vector<string>& args) {
  (void)args;
  hidKeyReportEnabled = false;
  midiKeyReportEnabled = false;
  sysExKeyReportEnabled = false;
  sysExReportPort = MIDI_PORT_INVALID;
  sysExBuffer.reserve(MAX_SYSEX_SIZE);
}

void Developer::Loop() {
  DrainInput();
  DrainHID();
  DrainMIDI();
  MatrixOS::SYS::DelayMs(1);
}

void Developer::End() {
  sysExBuffer.clear();
  activeSysExPort = MIDI_PORT_INVALID;
  sysExReportPort = MIDI_PORT_INVALID;
}

void Developer::DrainInput() {
  InputEvent event;
  while (MatrixOS::Input::Get(&event, 0))
  {
    HandleInputEvent(event);
  }
}

void Developer::DrainHID() {
  uint8_t* report = nullptr;
  size_t size = 0;
  uint8_t reportCount = 0;
  while (reportCount < MAX_HID_REPORTS_PER_LOOP && (size = MatrixOS::HID::RawHID::Get(&report, 0)) > 0)
  {
    HandleHIDReport(report, size);
    reportCount++;
  }
}

void Developer::DrainMIDI() {
  MidiPacket packet;
  while (MatrixOS::MIDI::Get(&packet, 0))
  {
    HandleMIDIPacket(packet);
  }
}

void Developer::HandleInputEvent(const InputEvent& event) {
  if (event.inputClass != InputClass::Keypad)
  {
    return;
  }
  if (event.keypad.state == KeypadState::Hold)
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

void Developer::HandleHIDReport(const uint8_t* report, size_t size) {
  if (report == nullptr || size == 0)
  {
    SendHIDError(0, ERROR_BAD_LENGTH);
    return;
  }

  uint8_t command = report[0];
  size_t payloadLength = 0;
  if (!GetHIDPayloadLength(command, report + 1, size - 1, &payloadLength))
  {
    SendHIDError(command, ERROR_BAD_LENGTH);
    return;
  }

  CommandReply reply = ExecuteCommand(command, report + 1, payloadLength, Transport::HID, &hidKeyReportEnabled);
  if (reply.result == CommandResult::OK)
  {
    if (!reply.skipAck)
    {
      SendHIDAck(command, reply.data);
    }
    if (reply.exitApp)
    {
      Exit();
    }
    return;
  }

  SendHIDError(command, ResultToErrorCode(reply.result));
}

void Developer::HandleMIDIPacket(const MidiPacket& packet) {
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

void Developer::HandleSysExPacket(const MidiPacket& packet) {
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

void Developer::HandleSysExMessage(uint16_t port, const vector<uint8_t>& message) {
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
    if (!reply.skipAck)
    {
      SendSysExAck(port, command, reply.data);
    }
    if (reply.exitApp)
    {
      Exit();
    }
    return;
  }

  SendSysExError(port, command, ResultToErrorCode(reply.result));
}
