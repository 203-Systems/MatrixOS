#include "CoprocessorLinkHost.h"

#include <inttypes.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

namespace CoprocessorLink
{

namespace
{

constexpr char kTag[] = "CoprocessorLink";

uint64_t nowMs() {
  return (uint64_t)(esp_timer_get_time() / 1000ULL);
}

uint16_t crc16Begin() {
  return 0xFFFFU;
}

uint16_t crc16Update(uint16_t crc, const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; ++i)
  {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t bit = 0; bit < 8; ++bit)
    {
      crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
    }
  }
  return crc;
}

uint16_t crc16Finish(uint16_t crc) {
  return crc;
}

uint16_t crc16(const uint8_t* data, size_t len) {
  return crc16Finish(crc16Update(crc16Begin(), data, len));
}

} // namespace

Host::Host(Transport& transport) : transport_(transport) {}

bool Host::begin() {
  if (!transport_.begin())
  {
    setError("transport begin failed");
    return false;
  }

  transport_.clearRx();
  setError("ok", kStatusOk);
  ESP_LOGI(kTag, "host begin ok");
  return true;
}

uint8_t Host::nextSeq() {
  return nextSeq_++;
}

void Host::setError(const char* error, uint8_t status) {
  lastError_ = error;
  lastStatus_ = status;
}

bool Host::sendFrame(uint8_t cmd, uint8_t seq, const void* payload, uint16_t payloadLen) {
  if (payloadLen > kMaxPayload)
  {
    setError("payload too large", kStatusErrLen);
    return false;
  }

  uint8_t frame[2 + 5 + kMaxPayload + COPROCESSOR_FRAME_CRC_SIZE];
  size_t cursor = 0;
  frame[cursor++] = (uint8_t)(kFrameMagic & 0xFFU);
  frame[cursor++] = (uint8_t)(kFrameMagic >> 8);
  frame[cursor++] = cmd;
  frame[cursor++] = (uint8_t)(cmd ^ 0xFFU);
  frame[cursor++] = seq;
  frame[cursor++] = (uint8_t)(payloadLen & 0xFFU);
  frame[cursor++] = (uint8_t)(payloadLen >> 8);

  if ((payload != nullptr) && (payloadLen > 0U))
  {
    memcpy(&frame[cursor], payload, payloadLen);
    cursor += payloadLen;
  }

  const uint16_t frameCrc = crc16(frame, cursor);
  frame[cursor++] = (uint8_t)(frameCrc & 0xFFU);
  frame[cursor++] = (uint8_t)((frameCrc >> 8) & 0xFFU);

  ESP_LOGD(kTag, "send frame: cmd=0x%02X seq=%u payload=%u crc=0x%04X", cmd, seq, payloadLen, frameCrc);

  if (!transport_.write(frame, cursor))
  {
    setError("transport write failed");
    return false;
  }

  return true;
}

bool Host::receiveFrame(Frame& frame, uint32_t timeoutMs) {
  const uint8_t magic0 = (uint8_t)(kFrameMagic & 0xFFU);
  const uint8_t magic1 = (uint8_t)(kFrameMagic >> 8);
  const uint64_t deadlineMs = nowMs() + timeoutMs;
  uint8_t byte = 0;

  while (nowMs() < deadlineMs)
  {
    const uint32_t remainMs = (uint32_t)(deadlineMs - nowMs());
    if (!transport_.read(&byte, 1, remainMs))
    {
      setError("receive timeout");
      ESP_LOGD(kTag, "receive frame: timeout waiting for magic0");
      return false;
    }

    if (byte != magic0)
    {
      continue;
    }

    if (!transport_.read(&byte, 1, remainMs) || (byte != magic1))
    {
      ESP_LOGD(kTag, "receive frame: magic1 mismatch");
      continue;
    }

    uint8_t hdrTail[5];
    if (!transport_.read(hdrTail, sizeof(hdrTail), remainMs))
    {
      setError("header timeout");
      ESP_LOGD(kTag, "receive frame: header timeout");
      return false;
    }

    frame.cmd = hdrTail[0];
    frame.seq = hdrTail[2];
    frame.length = (uint16_t)hdrTail[3] | ((uint16_t)hdrTail[4] << 8);

    if ((hdrTail[1] != (uint8_t)(frame.cmd ^ 0xFFU)) || (frame.length > kMaxPayload))
    {
      ESP_LOGD(kTag, "receive frame: invalid header cmd=0x%02X cmd_inv=0x%02X len=%u", frame.cmd, hdrTail[1], frame.length);
      continue;
    }

    if ((frame.length > 0U) && !transport_.read(frame.payload, frame.length, remainMs))
    {
      setError("payload timeout");
      ESP_LOGD(kTag, "receive frame: payload timeout len=%u", frame.length);
      return false;
    }

    uint8_t crcBytes[COPROCESSOR_FRAME_CRC_SIZE];
    if (!transport_.read(crcBytes, sizeof(crcBytes), remainMs))
    {
      setError("crc timeout");
      ESP_LOGD(kTag, "receive frame: crc timeout");
      return false;
    }

    uint8_t raw[2 + 5 + kMaxPayload];
    size_t cursor = 0;
    raw[cursor++] = magic0;
    raw[cursor++] = magic1;
    memcpy(&raw[cursor], hdrTail, sizeof(hdrTail));
    cursor += sizeof(hdrTail);
    memcpy(&raw[cursor], frame.payload, frame.length);
    cursor += frame.length;

    const uint16_t expected = crc16(raw, cursor);
    const uint16_t received = (uint16_t)crcBytes[0] | ((uint16_t)crcBytes[1] << 8);
    if (expected != received)
    {
      ESP_LOGD(kTag, "receive frame: crc mismatch expected=0x%04X received=0x%04X", expected, received);
      continue;
    }

    setError("ok", kStatusOk);
    ESP_LOGD(kTag, "receive frame ok: cmd=0x%02X seq=%u len=%u", frame.cmd, frame.seq, frame.length);
    return true;
  }

  setError("receive timeout");
  return false;
}

bool Host::transact(uint8_t cmd, const void* payload, uint16_t payloadLen, Frame& response, uint32_t timeoutMs) {
  const uint8_t seq = nextSeq();
  const uint8_t expectedResponseCmd = MakeResponseCommand(cmd);
  ESP_LOGD(kTag, "transact start: cmd=0x%02X seq=%u timeout=%" PRIu32, cmd, seq, timeoutMs);
  if (!sendFrame(cmd, seq, payload, payloadLen))
  {
    return false;
  }

  if (!receiveFrame(response, timeoutMs))
  {
    return false;
  }

  if ((response.cmd != expectedResponseCmd) || (response.seq != seq))
  {
    setError("unexpected response");
    ESP_LOGD(kTag, "transact unexpected response: cmd=0x%02X expected=0x%02X seq=%u expected_seq=%u", response.cmd, expectedResponseCmd,
             response.seq, seq);
    return false;
  }

  return true;
}

bool Host::query(QueryResponse& response, uint32_t timeoutMs) {
  Frame frame;
  if (!transact(kCmdGetStatus, nullptr, 0, frame, timeoutMs))
  {
    return false;
  }

  if (frame.length < sizeof(StatusMinResponse))
  {
    setError("query response length", kStatusErrLen);
    return false;
  }

  memset(&response, 0, sizeof(response));

  StatusMinResponse minResponse = {};
  memcpy(&minResponse, frame.payload, sizeof(minResponse));

  response.status = minResponse.status;
  response.protoVersion = minResponse.protoVersion;
  response.endpoint = minResponse.endpoint;

  if (frame.length >= sizeof(coprocessor_get_status_rsp_t))
  {
    coprocessor_get_status_rsp_t fullResponse = {};
    memcpy(&fullResponse, frame.payload, sizeof(fullResponse));
    response.flags = fullResponse.flags;
    response.bootloaderCrc32 = fullResponse.bootloaderCrc32;
    response.appCrc32 = fullResponse.appCrc32;
    response.appSize = fullResponse.appSize;
    response.appMaxSize = fullResponse.appMaxSize;
    response.hasExtendedStatus = true;
  }

  lastStatus_ = response.status;
  return response.status == kStatusOk;
}

bool Host::appJump(uint8_t* status, uint32_t timeoutMs) {
  Frame frame;
  if (!transact(kCmdAppJump, nullptr, 0, frame, timeoutMs))
  {
    return false;
  }
  if (frame.length != sizeof(StatusResponse))
  {
    setError("app jump response length", kStatusErrLen);
    return false;
  }

  StatusResponse response;
  memcpy(&response, frame.payload, sizeof(response));
  lastStatus_ = response.status;
  if (status != nullptr)
  {
    *status = response.status;
  }
  return response.status == kStatusOk;
}

bool Host::enterBootloader(uint8_t* status, uint32_t timeoutMs) {
  Frame frame;
  if (!transact(kCmdEnterBootloader, nullptr, 0, frame, timeoutMs))
  {
    return false;
  }
  if (frame.length != sizeof(StatusResponse))
  {
    setError("enter bootloader response length", kStatusErrLen);
    return false;
  }

  StatusResponse response;
  memcpy(&response, frame.payload, sizeof(response));
  lastStatus_ = response.status;
  if (status != nullptr)
  {
    *status = response.status;
  }
  return response.status == kStatusOk;
}

bool Host::otaBegin(uint32_t imageSize, uint32_t imageCrc32, uint32_t* nextOffset, uint32_t timeoutMs) {
  OtaBeginRequest request = {};
  request.imageSize = imageSize;
  request.imageCrc32 = imageCrc32;

  Frame frame;
  if (!transact(kCmdOtaBegin, &request, sizeof(request), frame, timeoutMs))
  {
    return false;
  }
  if (frame.length != sizeof(OtaBeginResponse))
  {
    setError("ota begin response length", kStatusErrLen);
    return false;
  }

  OtaBeginResponse response;
  memcpy(&response, frame.payload, sizeof(response));
  lastStatus_ = response.status;
  if (nextOffset != nullptr)
  {
    *nextOffset = response.nextOffset;
  }
  return response.status == kStatusOk;
}

bool Host::otaData(uint32_t offset, const uint8_t* data, uint16_t dataLen, uint32_t* nextOffset, uint32_t timeoutMs) {
  if ((data == nullptr) || (dataLen == 0U) || ((dataLen + sizeof(uint32_t)) > kMaxPayload))
  {
    setError("ota data invalid args", kStatusErrLen);
    return false;
  }

  uint8_t payload[sizeof(uint32_t) + kMaxPayload];
  payload[0] = (uint8_t)(offset & 0xFFU);
  payload[1] = (uint8_t)((offset >> 8) & 0xFFU);
  payload[2] = (uint8_t)((offset >> 16) & 0xFFU);
  payload[3] = (uint8_t)((offset >> 24) & 0xFFU);
  memcpy(&payload[4], data, dataLen);

  Frame frame;
  if (!transact(kCmdOtaData, payload, (uint16_t)(sizeof(uint32_t) + dataLen), frame, timeoutMs))
  {
    return false;
  }
  if (frame.length != sizeof(OtaDataResponse))
  {
    setError("ota data response length", kStatusErrLen);
    return false;
  }

  OtaDataResponse response;
  memcpy(&response, frame.payload, sizeof(response));
  lastStatus_ = response.status;
  if (nextOffset != nullptr)
  {
    *nextOffset = response.nextOffset;
  }
  return response.status == kStatusOk;
}

bool Host::otaEnd(uint8_t* status, uint32_t timeoutMs) {
  Frame frame;
  if (!transact(kCmdOtaEnd, nullptr, 0, frame, timeoutMs))
  {
    return false;
  }
  if (frame.length != sizeof(StatusResponse))
  {
    setError("ota end response length", kStatusErrLen);
    return false;
  }

  StatusResponse response;
  memcpy(&response, frame.payload, sizeof(response));
  lastStatus_ = response.status;
  if (status != nullptr)
  {
    *status = response.status;
  }
  return response.status == kStatusOk;
}

bool Host::appTransact(uint8_t commandId, const void* payload, uint16_t payloadLen, Frame& response, uint32_t timeoutMs) {
  const uint8_t cmd = COPROCESSOR_MAKE_CMD(COPROCESSOR_CMD_DIR_REQUEST, COPROCESSOR_CMD_SCOPE_APP, commandId);
  return transact(cmd, payload, payloadLen, response, timeoutMs);
}

bool Host::programImage(const QueryResponse& query, const uint8_t* image, uint32_t imageSize, uint32_t imageCrc32, uint16_t chunkSize,
                        uint32_t timeoutMs) {
  if ((image == nullptr) || (imageSize == 0U))
  {
    setError("image missing");
    return false;
  }

  if (!query.hasExtendedStatus)
  {
    setError("bootloader status required", kStatusErrState);
    return false;
  }

  if (query.endpoint != COPROCESSOR_ENDPOINT_BOOTLOADER)
  {
    setError("not in bootloader", kStatusErrState);
    return false;
  }

  if (imageSize > query.appMaxSize)
  {
    setError("image too large", kStatusErrLen);
    return false;
  }

  if ((query.appSize == imageSize) && (query.appCrc32 == imageCrc32))
  {
    ESP_LOGI(kTag, "OTA skipped: image already matches size=%" PRIu32 " crc=0x%08" PRIX32, imageSize, imageCrc32);
    setError("image already matches", kStatusOk);
    return true;
  }

  uint32_t offset = 0U;
  ESP_LOGI(kTag, "OTA begin: image_size=%" PRIu32 " crc=0x%08" PRIX32 " chunk=%u timeout=%" PRIu32, imageSize, imageCrc32, chunkSize,
           timeoutMs);
  if (!otaBegin(imageSize, imageCrc32, &offset, timeoutMs))
  {
    ESP_LOGE(kTag, "OTA begin failed: %s (status=%u)", lastError(), lastStatus());
    return false;
  }

  constexpr uint32_t kOtaDataMaxRetries = 4U;
  while (offset < imageSize)
  {
    const uint32_t startOffset = offset;
    const uint16_t currentChunk = (uint16_t)(((imageSize - offset) > chunkSize) ? chunkSize : (imageSize - offset));
    uint32_t nextOffset = startOffset;
    bool chunkOk = false;

    for (uint32_t attempt = 0U; attempt < kOtaDataMaxRetries; ++attempt)
    {
      nextOffset = startOffset;
      if (otaData(startOffset, &image[startOffset], currentChunk, &nextOffset, timeoutMs))
      {
        offset = nextOffset;
        chunkOk = true;
        break;
      }

      ESP_LOGW(kTag, "OTA data retry %" PRIu32 "/%" PRIu32 ": offset=%" PRIu32 " chunk=%u error=%s status=%u", attempt + 1U,
               kOtaDataMaxRetries, startOffset, currentChunk, lastError(), lastStatus());
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (!chunkOk)
    {
      ESP_LOGE(kTag, "OTA data failed: offset=%" PRIu32 " chunk=%u error=%s status=%u", startOffset, currentChunk, lastError(),
               lastStatus());
      return false;
    }
  }

  uint8_t status = 0xFFU;
  if (!otaEnd(&status, timeoutMs))
  {
    ESP_LOGE(kTag, "OTA end failed: %s (status=%u)", lastError(), lastStatus());
    return false;
  }

  return status == kStatusOk;
}

} // namespace CoprocessorLink
