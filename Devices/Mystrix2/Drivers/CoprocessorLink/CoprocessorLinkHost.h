#pragma once

#include <stddef.h>
#include <stdint.h>

#include "CoprocessorLinkProtocol.h"
#include "CoprocessorLinkTransport.h"

namespace CoprocessorLink {

class Host {
 public:
  explicit Host(Transport& transport);

  bool begin();
  bool query(QueryResponse& response, uint32_t timeoutMs = 800U);
  bool appJump(uint8_t* status = nullptr, uint32_t timeoutMs = 800U);
  bool enterBootloader(uint8_t* status = nullptr, uint32_t timeoutMs = 800U);
  bool otaBegin(uint32_t imageSize, uint32_t imageCrc32, uint32_t* nextOffset = nullptr, uint32_t timeoutMs = 800U);
  bool otaData(uint32_t offset,
               const uint8_t* data,
               uint16_t dataLen,
               uint32_t* nextOffset = nullptr,
               uint32_t timeoutMs = 800U);
  bool otaEnd(uint8_t* status = nullptr, uint32_t timeoutMs = 800U);
  bool appTransact(uint8_t commandId,
                   const void* payload,
                   uint16_t payloadLen,
                   Frame& response,
                   uint32_t timeoutMs = 800U);
  bool programImage(const QueryResponse& query,
                    const uint8_t* image,
                    uint32_t imageSize,
                    uint32_t imageCrc32,
                    uint16_t chunkSize = 240U,
                    uint32_t timeoutMs = 800U);

  uint8_t lastStatus() const { return lastStatus_; }
  const char* lastError() const { return lastError_; }

 private:
  bool transact(uint8_t cmd, const void* payload, uint16_t payloadLen, Frame& response, uint32_t timeoutMs);
  bool sendFrame(uint8_t cmd, uint8_t seq, const void* payload, uint16_t payloadLen);
  bool receiveFrame(Frame& frame, uint32_t timeoutMs);
  uint8_t nextSeq();
  void setError(const char* error, uint8_t status = 0xFFU);

  Transport& transport_;
  uint8_t nextSeq_ = 1U;
  uint8_t lastStatus_ = 0xFFU;
  const char* lastError_ = "uninitialized";
};

}  // namespace CoprocessorLink
