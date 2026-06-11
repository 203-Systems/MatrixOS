#include "CommandHandler.h"
#include "CommandSpecs.h"

#include "../../Applications/Application.h"
#include "../System/System.h"

namespace MatrixOS::Command
{
static constexpr uint8_t DEFAULT_LAYER = 255;
static constexpr uint32_t ACTION_ACK_DELAY_MS = 20;

static bool SendReply(const vector<uint8_t>& reply, size_t maxReplyLength, ReplyCallback replyCallback, void* replyContext);

static uint8_t ResponseCommand(uint8_t command, Encoding encoding) {
  if (encoding == Encoding::HID)
  {
    return command | 0x80;
  }
  return command;
}

static bool SendCommandAck(uint8_t command, Encoding encoding, size_t maxReplyLength, ReplyCallback replyCallback, void* replyContext) {
  return SendReply(vector<uint8_t>{ResponseCommand(command, encoding)}, maxReplyLength, replyCallback, replyContext);
}

static bool SendCommandAckAndDelay(uint8_t command, Encoding encoding, size_t maxReplyLength, ReplyCallback replyCallback, void* replyContext) {
  if (!SendCommandAck(command, encoding, maxReplyLength, replyCallback, replyContext))
  {
    return false;
  }
  SYS::DelayMs(ACTION_ACK_DELAY_MS);
  return true;
}

static void AppendString(vector<uint8_t>* reply, const string& value) {
  for (char character : value)
  {
    uint8_t byte = (uint8_t)character;
    reply->push_back(byte <= 0x7F ? byte : (uint8_t)'?');
  }
  reply->push_back(0);
}

static void AppendUInt16(vector<uint8_t>* reply, uint16_t value, Encoding encoding) {
  if (encoding == Encoding::HID)
  {
    reply->push_back((uint8_t)(value >> 8));
    reply->push_back((uint8_t)value);
    return;
  }

  reply->push_back((uint8_t)((value >> 14) & 0x7F));
  reply->push_back((uint8_t)((value >> 7) & 0x7F));
  reply->push_back((uint8_t)(value & 0x7F));
}

static void AppendUInt32(vector<uint8_t>* reply, uint32_t value, Encoding encoding) {
  if (encoding == Encoding::HID)
  {
    reply->push_back((uint8_t)(value >> 24));
    reply->push_back((uint8_t)(value >> 16));
    reply->push_back((uint8_t)(value >> 8));
    reply->push_back((uint8_t)value);
    return;
  }

  reply->push_back((uint8_t)((value >> 25) & 0x7F));
  reply->push_back((uint8_t)((value >> 18) & 0x7F));
  reply->push_back((uint8_t)((value >> 11) & 0x7F));
  reply->push_back((uint8_t)((value >> 4) & 0x7F));
  reply->push_back((uint8_t)((value << 3) & 0x7F));
}

static bool Is7BitData(const uint8_t* data, size_t size) {
  for (size_t i = 0; i < size; i++)
  {
    if ((data[i] & 0x80) != 0)
    {
      return false;
    }
  }
  return true;
}

static uint8_t SysExReleaseVersion() {
#if MATRIXOS_BUILD_VER == 0
  return 0;
#elif (MATRIXOS_BUILD_VER == 4)
  return 0x31;
#elif (MATRIXOS_RELEASE_VER < 32)
  return (MATRIXOS_BUILD_VER << 5) + MATRIXOS_RELEASE_VER;
#else
  return (MATRIXOS_BUILD_VER << 5) + 0x1F;
#endif
}

static bool DecodeUInt16(const uint8_t* request, size_t size, size_t offset, Encoding encoding, uint16_t* value, size_t* nextOffset) {
  if (encoding == Encoding::HID)
  {
    if (size < offset + 2)
    {
      return false;
    }

    *value = ((uint16_t)request[offset] << 8) | request[offset + 1];
    *nextOffset = offset + 2;
    return true;
  }

  if (size < offset + 3 || !Is7BitData(request + offset, 3))
  {
    return false;
  }

  *value = ((uint16_t)request[offset] << 14) | ((uint16_t)request[offset + 1] << 7) | request[offset + 2];
  *nextOffset = offset + 3;
  return true;
}

static bool DecodeUInt32(const uint8_t* request, size_t size, size_t offset, Encoding encoding, uint32_t* value, size_t* nextOffset) {
  if (encoding == Encoding::HID)
  {
    if (size < offset + 4)
    {
      return false;
    }

    *value = ((uint32_t)request[offset] << 24) + ((uint32_t)request[offset + 1] << 16) + ((uint32_t)request[offset + 2] << 8) +
             (uint32_t)request[offset + 3];
    *nextOffset = offset + 4;
    return true;
  }

  if (size < offset + 5 || !Is7BitData(request + offset, 5))
  {
    return false;
  }

  *value = ((uint32_t)request[offset] << 25);
  *value += ((uint32_t)request[offset + 1] << 18);
  *value += ((uint32_t)request[offset + 2] << 11);
  *value += ((uint32_t)request[offset + 3] << 4);
  *value += ((uint32_t)request[offset + 4] >> 3);
  *nextOffset = offset + 5;
  return true;
}

static bool DecodeColor(const uint8_t* request, size_t size, size_t offset, Encoding encoding, Color* color, size_t* nextOffset) {
  uint32_t rawColor = 0;
  if (!DecodeUInt32(request, size, offset, encoding, &rawColor, nextOffset))
  {
    return false;
  }

  *color = Color(rawColor);
  return true;
}

static bool DecodeAppId(const uint8_t* request, size_t size, Encoding encoding, uint32_t* appId) {
  size_t nextOffset = 0;
  if (!DecodeUInt32(request, size, 1, encoding, appId, &nextOffset))
  {
    return false;
  }
  return encoding == Encoding::HID ? size == 9 : size == 6;
}

static vector<uint8_t> EncodeAppId(uint8_t command, uint32_t appId, Encoding encoding) {
  vector<uint8_t> reply = {ResponseCommand(command, encoding)};
  AppendUInt32(&reply, appId, encoding);
  return reply;
}

static vector<uint8_t> EncodeVersion(uint8_t command, Encoding encoding) {
  if (encoding == Encoding::HID)
  {
    return vector<uint8_t>{ResponseCommand(command, encoding),
                           MATRIXOS_MAJOR_VER,
                           MATRIXOS_MINOR_VER,
                           MATRIXOS_PATCH_VER,
                           MATRIXOS_BUILD_VER,
                           MATRIXOS_RELEASE_VER};
  }

  return vector<uint8_t>{ResponseCommand(command, encoding), MATRIXOS_MAJOR_VER, MATRIXOS_MINOR_VER, MATRIXOS_PATCH_VER,
                         SysExReleaseVersion()};
}

static vector<uint8_t> EncodeString(uint8_t command, const string& value, Encoding encoding) {
  vector<uint8_t> reply = {ResponseCommand(command, encoding)};
  AppendString(&reply, value);
  return reply;
}

static vector<uint8_t> EncodeUInt16Reply(uint8_t command, uint16_t value, Encoding encoding) {
  vector<uint8_t> reply = {ResponseCommand(command, encoding)};
  AppendUInt16(&reply, value, encoding);
  return reply;
}

static vector<uint8_t> EncodeUInt32Reply(uint8_t command, uint32_t value, Encoding encoding) {
  vector<uint8_t> reply = {ResponseCommand(command, encoding)};
  AppendUInt32(&reply, value, encoding);
  return reply;
}

static bool GetLayerArg(const uint8_t* request, size_t size, size_t offset, uint8_t* layer) {
  *layer = size > offset ? request[offset] : DEFAULT_LAYER;
  return *layer == DEFAULT_LAYER || *layer < MAX_LED_LAYERS;
}

static bool HandleLedSetColorXY(const uint8_t* request, size_t size, Encoding encoding) {
  size_t colorOffset = 0;
  Point point;

  bool hasLongCoordinates = (encoding == Encoding::HID && size >= 9) || (encoding == Encoding::SysEx7Bit && size >= 12);
  if (hasLongCoordinates)
  {
    uint16_t x = 0;
    uint16_t y = 0;
    size_t nextOffset = 0;
    if (!DecodeUInt16(request, size, 1, encoding, &x, &nextOffset) || !DecodeUInt16(request, size, nextOffset, encoding, &y, &colorOffset))
    {
      return false;
    }
    point = Point((int16_t)x, (int16_t)y);
  }
  else if (size >= 4)
  {
    if (encoding == Encoding::SysEx7Bit && !Is7BitData(request + 1, 2))
    {
      return false;
    }
    point = Point((int16_t)request[1], (int16_t)request[2]);
    colorOffset = 3;
  }
  else
  {
    return false;
  }

  Color color;
  size_t layerOffset = 0;
  uint8_t layer = DEFAULT_LAYER;
  if (!DecodeColor(request, size, colorOffset, encoding, &color, &layerOffset) || !GetLayerArg(request, size, layerOffset, &layer))
  {
    return false;
  }

  MatrixOS::LED::SetColor(point, color, layer);
  return true;
}

static bool HandleLedSetColorID(const uint8_t* request, size_t size, Encoding encoding) {
  uint16_t ledId = 0;
  size_t colorOffset = 0;
  Color color;
  size_t layerOffset = 0;
  uint8_t layer = DEFAULT_LAYER;

  if (!DecodeUInt16(request, size, 1, encoding, &ledId, &colorOffset) ||
      !DecodeColor(request, size, colorOffset, encoding, &color, &layerOffset) || !GetLayerArg(request, size, layerOffset, &layer))
  {
    return false;
  }

  MatrixOS::LED::SetColor(ledId, color, layer);
  return true;
}

static bool HandleLedFill(const uint8_t* request, size_t size, Encoding encoding) {
  Color color;
  size_t layerOffset = 0;
  uint8_t layer = DEFAULT_LAYER;

  if (!DecodeColor(request, size, 1, encoding, &color, &layerOffset) || !GetLayerArg(request, size, layerOffset, &layer))
  {
    return false;
  }

  MatrixOS::LED::Fill(color, layer);
  return true;
}

static bool HandleLedFillPartition(const uint8_t* request, size_t size, Encoding encoding) {
  if (size < 2 || (encoding == Encoding::SysEx7Bit && (request[1] & 0x80) != 0))
  {
    return false;
  }

  uint8_t partitionIndex = request[1];
  if (partitionIndex >= Device::LED::partitions.size())
  {
    return false;
  }

  Color color;
  size_t layerOffset = 0;
  uint8_t layer = DEFAULT_LAYER;
  if (!DecodeColor(request, size, 2, encoding, &color, &layerOffset) || !GetLayerArg(request, size, layerOffset, &layer))
  {
    return false;
  }

  return MatrixOS::LED::FillPartition(Device::LED::partitions[partitionIndex].name, color, layer);
}

static bool HandleKeypadGetKeyXY(uint8_t command, const uint8_t* request, size_t size, Encoding encoding, size_t maxReplyLength,
                                 ReplyCallback replyCallback, void* replyContext) {
  if (size < 3 || (encoding == Encoding::SysEx7Bit && !Is7BitData(request + 1, 2)))
  {
    return false;
  }

  const InputCluster* grid = MatrixOS::Input::GetPrimaryGridCluster();
  if (grid == nullptr)
  {
    return SendReply(vector<uint8_t>{ResponseCommand(command, encoding), 0}, maxReplyLength, replyCallback, replyContext);
  }

  InputId id = InputId::Invalid();
  bool found = MatrixOS::Input::GetInputAt(grid->clusterId, Point(request[1], request[2]), &id);
  vector<uint8_t> reply = {ResponseCommand(command, encoding), (uint8_t)found};
  if (found)
  {
    reply.push_back(id.clusterId);
    AppendUInt16(&reply, id.memberId, encoding);
  }

  return SendReply(reply, maxReplyLength, replyCallback, replyContext);
}

static bool HandleKeypadGetKeyID(uint8_t command, const uint8_t* request, size_t size, Encoding encoding, size_t maxReplyLength,
                                 ReplyCallback replyCallback, void* replyContext) {
  if (size < 2 || (encoding == Encoding::SysEx7Bit && (request[1] & 0x80) != 0))
  {
    return false;
  }

  uint16_t memberId = 0;
  size_t nextOffset = 0;
  if (!DecodeUInt16(request, size, 2, encoding, &memberId, &nextOffset))
  {
    return false;
  }

  InputId id = {request[1], memberId};
  Point point;
  InputSnapshot snapshot;
  bool hasPosition = MatrixOS::Input::GetPosition(id, &point);
  bool hasState = MatrixOS::Input::GetState(id, &snapshot) && snapshot.inputClass == InputClass::Keypad;

  vector<uint8_t> reply = {ResponseCommand(command, encoding), (uint8_t)(hasPosition || hasState)};
  if (hasPosition || hasState)
  {
    reply.push_back(id.clusterId);
    AppendUInt16(&reply, id.memberId, encoding);
    AppendUInt16(&reply, hasPosition ? (uint16_t)point.x : 0, encoding);
    AppendUInt16(&reply, hasPosition ? (uint16_t)point.y : 0, encoding);
    reply.push_back(hasState ? (uint8_t)snapshot.keypad.state : (uint8_t)KeypadState::Invalid);
    AppendUInt16(&reply, hasState ? snapshot.keypad.pressure.value : 0, encoding);
    AppendUInt16(&reply, hasState ? snapshot.keypad.velocity.value : 0, encoding);
  }

  return SendReply(reply, maxReplyLength, replyCallback, replyContext);
}

static bool SendReply(const vector<uint8_t>& reply, size_t maxReplyLength, ReplyCallback replyCallback, void* replyContext) {
  if (replyCallback == nullptr || maxReplyLength == 0)
  {
    return false;
  }

  if (reply.empty())
  {
    return replyCallback(reply, true, replyContext);
  }

  size_t offset = 0;
  while (offset < reply.size())
  {
    size_t remaining = reply.size() - offset;
    size_t chunkLength = maxReplyLength < remaining ? maxReplyLength : remaining;
    vector<uint8_t> chunk(reply.begin() + offset, reply.begin() + offset + chunkLength);
    offset += chunkLength;

    if (!replyCallback(chunk, offset == reply.size(), replyContext))
    {
      return false;
    }
  }

  return true;
}

bool Handle(const uint8_t* request, size_t size, Encoding encoding, size_t maxReplyLength, ReplyCallback replyCallback, void* replyContext) {
  if (request == nullptr || size == 0)
  {
    return false;
  }

  uint8_t command = request[0];

  switch (command)
  {
  case MATRIXOS_COMMAND_GET_OS_VERSION: {
    return SendReply(EncodeVersion(command, encoding), maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_GET_DEVICE_NAME: {
    return SendReply(EncodeString(command, Device::name, encoding), maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_GET_DEVICE_MODEL_ID: {
    return SendReply(EncodeString(command, Device::model, encoding), maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_GET_DEVICE_SERIAL: {
    return SendReply(EncodeString(command, Device::GetSerial(), encoding), maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_GET_DEVICE_ID: {
    return SendReply(EncodeUInt16Reply(command, MatrixOS::UserVar::deviceId, encoding), maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_GET_DEVICE_SIZE: {
    vector<uint8_t> reply = {ResponseCommand(command, encoding), Device::xSize, Device::ySize};
    return SendReply(reply, maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_GET_DEVICE_LED_COUNT: {
    return SendReply(EncodeUInt32Reply(command, MatrixOS::LED::GetLEDCount(), encoding), maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_SET_DEVICE_ID: {
    uint16_t deviceId = 0;
    size_t nextOffset = 0;
    if (!DecodeUInt16(request, size, 1, encoding, &deviceId, &nextOffset))
    {
      return false;
    }
    MatrixOS::UserVar::deviceId.Set(deviceId);
    return true;
  }
  case MATRIXOS_COMMAND_GET_APP_ID: {
    return SendReply(EncodeAppId(command, SYS::activeAppId, encoding), maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_GET_APP_NAME: {
    string name = SYS::activeAppInfo == nullptr ? "" : SYS::activeAppInfo->name;
    return SendReply(EncodeString(command, name, encoding), maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_GET_APP_AUTHOR: {
    string author = SYS::activeAppInfo == nullptr ? "" : SYS::activeAppInfo->author;
    return SendReply(EncodeString(command, author, encoding), maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_GET_APP_VERSION: {
    uint32_t version = SYS::activeAppInfo == nullptr ? 0 : SYS::activeAppInfo->version;
    return SendReply(EncodeUInt32Reply(command, version, encoding), maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_ENTER_APP_VIA_ID: {
    uint32_t appId = 0;
    if (!DecodeAppId(request, size, encoding, &appId))
    {
      return false;
    }

    if (!SendCommandAckAndDelay(command, encoding, maxReplyLength, replyCallback, replyContext))
    {
      return false;
    }
    SYS::ExecuteAPP(appId);
    return true;
  }
  case MATRIXOS_COMMAND_QUIT_APP: {
    if (!SendCommandAckAndDelay(command, encoding, maxReplyLength, replyCallback, replyContext))
    {
      return false;
    }
    SYS::ExitAPP();
    return true;
  }
  case MATRIXOS_COMMAND_BOOTLOADER: {
    if (!SendCommandAckAndDelay(command, encoding, maxReplyLength, replyCallback, replyContext))
    {
      return false;
    }
    SYS::Bootloader();
    return true;
  }
  case MATRIXOS_COMMAND_REBOOT: {
    if (!SendCommandAckAndDelay(command, encoding, maxReplyLength, replyCallback, replyContext))
    {
      return false;
    }
    SYS::Reboot();
    return true;
  }
  case MATRIXOS_COMMAND_SLEEP: {
    return SendCommandAckAndDelay(command, encoding, maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_OPEN_SETTINGS: {
    if (!SendCommandAckAndDelay(command, encoding, maxReplyLength, replyCallback, replyContext))
    {
      return false;
    }
    SYS::OpenSetting();
    return true;
  }
  case MATRIXOS_COMMAND_OPEN_DEVELOPER_APP: {
    if (!SendCommandAckAndDelay(command, encoding, maxReplyLength, replyCallback, replyContext))
    {
      return false;
    }
    SYS::ExecuteAPP("203 Systems", "DeveloperApp");
    return true;
  }
  case MATRIXOS_COMMAND_LED_SET_COLOR_XY: {
    return HandleLedSetColorXY(request, size, encoding);
  }
  case MATRIXOS_COMMAND_LED_SET_COLOR_ID: {
    return HandleLedSetColorID(request, size, encoding);
  }
  case MATRIXOS_COMMAND_LED_FILL: {
    return HandleLedFill(request, size, encoding);
  }
  case MATRIXOS_COMMAND_LED_FILL_PARTITION: {
    return HandleLedFillPartition(request, size, encoding);
  }
  case MATRIXOS_COMMAND_LED_UPDATE: {
    uint8_t layer = DEFAULT_LAYER;
    if (!GetLayerArg(request, size, 1, &layer))
    {
      return false;
    }
    MatrixOS::LED::Update(layer);
    return true;
  }
  case MATRIXOS_COMMAND_LED_CREATELAYER: {
    int8_t layer = 0;
    if (size == 1)
    {
      layer = MatrixOS::LED::CreateLayer();
    }
    else
    {
      uint16_t crossfade = 0;
      size_t nextOffset = 0;
      if (!DecodeUInt16(request, size, 1, encoding, &crossfade, &nextOffset))
      {
        return false;
      }
      layer = MatrixOS::LED::CreateLayer(crossfade);
    }
    return SendReply(vector<uint8_t>{ResponseCommand(command, encoding), (uint8_t)layer}, maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_LED_COPYLAYER: {
    if (size < 3)
    {
      return false;
    }
    int8_t currentLayer = MatrixOS::LED::CurrentLayer();
    if (request[1] > currentLayer || request[2] > currentLayer)
    {
      return false;
    }
    MatrixOS::LED::CopyLayer(request[2], request[1]);
    return true;
  }
  case MATRIXOS_COMMAND_LED_DESTROYLAYER: {
    bool destroyed = false;
    if (size == 1)
    {
      destroyed = MatrixOS::LED::DestroyLayer();
    }
    else
    {
      uint16_t crossfade = 0;
      size_t nextOffset = 0;
      if (!DecodeUInt16(request, size, 1, encoding, &crossfade, &nextOffset))
      {
        return false;
      }
      destroyed = MatrixOS::LED::DestroyLayer(crossfade);
    }
    return SendReply(vector<uint8_t>{ResponseCommand(command, encoding), (uint8_t)destroyed}, maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_LED_SET_BRIGHTNESS: {
    if (size < 2 || (encoding == Encoding::SysEx7Bit && (request[1] & 0x80) != 0))
    {
      return false;
    }
    MatrixOS::LED::SetBrightness(request[1]);
    return true;
  }
  case MATRIXOS_COMMAND_LED_FADE: {
    if (size == 1)
    {
      MatrixOS::LED::Fade();
      return true;
    }

    uint16_t crossfade = 0;
    size_t nextOffset = 0;
    if (!DecodeUInt16(request, size, 1, encoding, &crossfade, &nextOffset))
    {
      return false;
    }
    MatrixOS::LED::Fade(crossfade);
    return true;
  }
  case MATRIXOS_COMMAND_GET_LED_CURRENT_LAYER: {
    return SendReply(vector<uint8_t>{ResponseCommand(command, encoding), (uint8_t)MatrixOS::LED::CurrentLayer()}, maxReplyLength,
                     replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_GET_LED_BRIGHTNESS: {
    return SendReply(vector<uint8_t>{ResponseCommand(command, encoding), MatrixOS::UserVar::brightness}, maxReplyLength, replyCallback,
                     replyContext);
  }
  case MATRIXOS_COMMAND_KEYPAD_GET_KEY_XY: {
    return HandleKeypadGetKeyXY(command, request, size, encoding, maxReplyLength, replyCallback, replyContext);
  }
  case MATRIXOS_COMMAND_KEYPAD_GET_KEY_ID: {
    return HandleKeypadGetKeyID(command, request, size, encoding, maxReplyLength, replyCallback, replyContext);
  }
  default: {
    return false;
  }
  }
}
} // namespace MatrixOS::Command
