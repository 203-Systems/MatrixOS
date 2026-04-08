#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/uart.h"

namespace CoprocessorLink
{

class Transport {
public:
  virtual ~Transport() = default;

  virtual bool begin() = 0;
  virtual bool write(const uint8_t* data, size_t len) = 0;
  virtual bool read(uint8_t* data, size_t len, uint32_t timeoutMs) = 0;
  virtual void clearRx() = 0;
};

class UartTransport final : public Transport {
public:
  UartTransport(uart_port_t uartNum, int txPin, int rxPin, uint32_t baudRate, int rxBufferSize = 2048, int txBufferSize = 0);

  bool begin() override;
  bool write(const uint8_t* data, size_t len) override;
  bool read(uint8_t* data, size_t len, uint32_t timeoutMs) override;
  void clearRx() override;
  size_t readAvailable(uint8_t* data, size_t maxLen);

private:
  uart_port_t uartNum_;
  int txPin_;
  int rxPin_;
  uint32_t baudRate_;
  int rxBufferSize_;
  int txBufferSize_;
  bool started_ = false;
};

} // namespace CoprocessorLink
