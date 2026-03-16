#include "CoprocessorLinkTransport.h"

#include <inttypes.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace CoprocessorLink {

namespace {

constexpr char kTag[] = "CoprocessorUART";

uint64_t nowMs() {
  return (uint64_t)(esp_timer_get_time() / 1000ULL);
}

}  // namespace

UartTransport::UartTransport(uart_port_t uartNum,
                             int txPin,
                             int rxPin,
                             uint32_t baudRate,
                             int rxBufferSize,
                             int txBufferSize)
    : uartNum_(uartNum),
      txPin_(txPin),
      rxPin_(rxPin),
      baudRate_(baudRate),
      rxBufferSize_(rxBufferSize),
      txBufferSize_(txBufferSize) {}

bool UartTransport::begin() {
  if (started_) {
    return true;
  }

  ESP_LOGI(kTag,
           "uart begin: port=%d tx=%d rx=%d baud=%" PRIu32 " rx_buf=%d tx_buf=%d",
           (int)uartNum_,
           txPin_,
           rxPin_,
           baudRate_,
           rxBufferSize_,
           txBufferSize_);

  uart_config_t config = {};
  config.baud_rate = (int)baudRate_;
  config.data_bits = UART_DATA_8_BITS;
  config.parity = UART_PARITY_DISABLE;
  config.stop_bits = UART_STOP_BITS_1;
  config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  config.rx_flow_ctrl_thresh = 0;
  config.source_clk = UART_SCLK_DEFAULT;

  if (uart_driver_install(uartNum_, rxBufferSize_, txBufferSize_, 0, nullptr, 0) != ESP_OK) {
    ESP_LOGE(kTag, "uart_driver_install failed");
    return false;
  }

  if (uart_param_config(uartNum_, &config) != ESP_OK) {
    ESP_LOGE(kTag, "uart_param_config failed");
    return false;
  }

  if (uart_set_pin(uartNum_, txPin_, rxPin_, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
    ESP_LOGE(kTag, "uart_set_pin failed");
    return false;
  }

  if (uart_set_line_inverse(uartNum_, 0U) != ESP_OK) {
    ESP_LOGE(kTag, "uart_set_line_inverse failed");
    return false;
  }

  uart_flush_input(uartNum_);
  started_ = true;
  ESP_LOGI(kTag, "uart begin ok");
  return true;
}

bool UartTransport::write(const uint8_t* data, size_t len) {
  if (!started_ && !begin()) {
    return false;
  }

  if (uart_write_bytes(uartNum_, data, len) < 0) {
    ESP_LOGE(kTag, "uart_write_bytes failed len=%u", (unsigned)len);
    return false;
  }

  ESP_LOGD(kTag,
           "uart tx len=%u first=%02X %02X %02X %02X",
           (unsigned)len,
           len > 0 ? data[0] : 0,
           len > 1 ? data[1] : 0,
           len > 2 ? data[2] : 0,
           len > 3 ? data[3] : 0);

  const bool success = uart_wait_tx_done(uartNum_, pdMS_TO_TICKS(1000)) == ESP_OK;
  if (!success) {
    ESP_LOGE(kTag, "uart_wait_tx_done timeout");
  }
  return success;
}

bool UartTransport::read(uint8_t* data, size_t len, uint32_t timeoutMs) {
  if (!started_ && !begin()) {
    return false;
  }

  size_t received = 0;
  const uint64_t deadlineMs = nowMs() + timeoutMs;

  while (received < len) {
    if (nowMs() >= deadlineMs) {
      ESP_LOGD(kTag, "uart rx timeout: need=%u got=%u timeout=%" PRIu32, (unsigned)len, (unsigned)received, timeoutMs);
      return false;
    }

    const uint32_t remainMs = (uint32_t)(deadlineMs - nowMs());
    const int got = uart_read_bytes(
        uartNum_, data + received, len - received, pdMS_TO_TICKS(remainMs > 10U ? 10U : remainMs));
    if (got > 0) {
      received += (size_t)got;
    } else {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }

  ESP_LOGD(kTag,
           "uart rx len=%u first=%02X %02X %02X %02X",
           (unsigned)len,
           len > 0 ? data[0] : 0,
           len > 1 ? data[1] : 0,
           len > 2 ? data[2] : 0,
           len > 3 ? data[3] : 0);
  return true;
}

void UartTransport::clearRx() {
  if (started_) {
    uart_flush_input(uartNum_);
  }
}

size_t UartTransport::readAvailable(uint8_t* data, size_t maxLen) {
  if ((data == nullptr) || (maxLen == 0U)) {
    return 0U;
  }

  if (!started_ && !begin()) {
    return 0U;
  }

  size_t buffered = 0U;
  if (uart_get_buffered_data_len(uartNum_, &buffered) != ESP_OK) {
    return 0U;
  }
  if (buffered == 0U) {
    return 0U;
  }

  const size_t toRead = (buffered > maxLen) ? maxLen : buffered;
  const int got = uart_read_bytes(uartNum_, data, toRead, 0);
  if (got > 0) {
    ESP_LOGW(kTag,
             "uart stray rx len=%u first=%02X %02X %02X %02X",
             (unsigned)got,
             got > 0 ? data[0] : 0,
             got > 1 ? data[1] : 0,
             got > 2 ? data[2] : 0,
             got > 3 ? data[3] : 0);
  }
  return (got > 0) ? (size_t)got : 0U;
}

}  // namespace CoprocessorLink
