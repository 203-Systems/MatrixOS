#include "Device.h"

#include <inttypes.h>

#include "CoprocessorLink/CoprocessorLinkHost.h"
#include "CoprocessorLink/CoprocessorLinkTransport.h"
#include "CoprocessorLink/Firmware/mystrix2_mpe_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Variants/Mystrix2/UltraProto2/Config.h"

namespace Device::KeyPad::MPE
{
  namespace
  {
    constexpr char kTag[] = "Mystrix2-MPE";
    constexpr uart_port_t kMpeUartPort = UART_NUM_1;
    constexpr uint32_t kMpeBaud = 2500000U;
    constexpr uint32_t kQueryTimeoutMs = 150U;
    constexpr uint32_t kQueryPollIntervalMs = 50U;
    constexpr uint32_t kQueryMaxAttempts = 40U;
    constexpr uint16_t kOtaChunkSize = 128U;
    constexpr uint32_t kOtaTimeoutMs = 2000U;

    CoprocessorLink::UartTransport mpe_transport(kMpeUartPort, UPT2::KEYPAD_TX_PIN, UPT2::KEYPAD_RX_PIN, kMpeBaud);
    CoprocessorLink::Host mpe_host(mpe_transport);
    bool host_initialized = false;
    bool probe_started = false;

    void ProbeTask(void*) {
      auto queryWithRetries = [&](const char* phase, CoprocessorLink::QueryResponse& response) -> bool {
        uint32_t attempts = 0U;
        ESP_LOGI(kTag, "starting %s query loop", phase);
        while ((attempts < kQueryMaxAttempts) && !mpe_host.query(response, kQueryTimeoutMs)) {
          ESP_LOGW(kTag,
                   "%s query attempt %u/%u failed: error=%s status=%u",
                   phase,
                   (unsigned)(attempts + 1U),
                   (unsigned)kQueryMaxAttempts,
                   mpe_host.lastError(),
                   mpe_host.lastStatus());
          uint8_t raw_rx[64];
          const size_t raw_len = mpe_transport.readAvailable(raw_rx, sizeof(raw_rx));
          if (raw_len > 0U) {
            ESP_LOGW(kTag,
                     "%s query retry stray data: len=%u first=%02X %02X %02X %02X",
                     phase,
                     (unsigned)raw_len,
                     raw_rx[0],
                     raw_len > 1 ? raw_rx[1] : 0,
                     raw_len > 2 ? raw_rx[2] : 0,
                     raw_len > 3 ? raw_rx[3] : 0);
          }
          attempts++;
          vTaskDelay(pdMS_TO_TICKS(kQueryPollIntervalMs));
        }

        if (attempts >= kQueryMaxAttempts) {
          ESP_LOGW(kTag, "%s query failed: %s (status=%u)", phase, mpe_host.lastError(), mpe_host.lastStatus());
          return false;
        }

        ESP_LOGD(kTag, "%s query ok after %u attempt(s)", phase, (unsigned)(attempts + 1U));
        return true;
      };

      ESP_LOGI(kTag, "ProbeTask entered");
      CoprocessorLink::QueryResponse query = {};
      if (!queryWithRetries("initial", query)) {
        probe_started = false;
        vTaskDelete(nullptr);
        return;
      }

      ESP_LOGI(kTag,
               "query ok: status=%u proto=%u endpoint=%u flags=0x%02X bl_crc=0x%08" PRIX32 " app_crc=0x%08" PRIX32
               " app_size=%" PRIu32 " app_max=%" PRIu32,
               query.status,
               query.protoVersion,
               query.endpoint,
               query.flags,
               query.bootloaderCrc32,
               query.appCrc32,
               query.appSize,
               query.appMaxSize);
      ESP_LOGD(kTag,
               "mcu status: endpoint=%u extended=%u crc=0x%08" PRIX32 " size=%" PRIu32,
               query.endpoint,
               query.hasExtendedStatus ? 1U : 0U,
               query.appCrc32,
               query.appSize);

      if (query.endpoint == CoprocessorLink::COPROCESSOR_ENDPOINT_APP) {
        uint8_t enter_status = 0xFFU;
        ESP_LOGI(kTag, "coprocessor app detected, requesting enter bootloader");
        if (!mpe_host.enterBootloader(&enter_status, kQueryTimeoutMs)) {
          ESP_LOGE(kTag, "enter bootloader failed: %s (status=%u)", mpe_host.lastError(), enter_status);
          probe_started = false;
          vTaskDelete(nullptr);
          return;
        }

        ESP_LOGI(kTag, "enter bootloader acked, waiting for bootloader");
        mpe_transport.clearRx();
        vTaskDelay(pdMS_TO_TICKS(200));
        if (!mpe_host.begin()) {
          ESP_LOGE(kTag, "host re-begin failed after enter bootloader: %s", mpe_host.lastError());
          probe_started = false;
          vTaskDelete(nullptr);
          return;
        }
        mpe_transport.clearRx();

        if (!queryWithRetries("bootloader", query)) {
          ESP_LOGE(kTag, "bootloader did not come back after enter bootloader");
          probe_started = false;
          vTaskDelete(nullptr);
          return;
        }

        ESP_LOGI(kTag,
                 "bootloader query: status=%u proto=%u endpoint=%u flags=0x%02X bl_crc=0x%08" PRIX32
                 " app_crc=0x%08" PRIX32 " app_size=%" PRIu32 " app_max=%" PRIu32,
                 query.status,
                 query.protoVersion,
                 query.endpoint,
                 query.flags,
                 query.bootloaderCrc32,
                 query.appCrc32,
                 query.appSize,
                 query.appMaxSize);
      }

      if ((query.endpoint != CoprocessorLink::COPROCESSOR_ENDPOINT_BOOTLOADER) || !query.hasExtendedStatus) {
        ESP_LOGE(kTag,
                 "invalid bootloader status: endpoint=%u extended=%u",
                 query.endpoint,
                 query.hasExtendedStatus ? 1U : 0U);
        probe_started = false;
        vTaskDelete(nullptr);
        return;
      }

      if (!mpe_host.programImage(query,
                                 mystrix2_mpe_driver_image,
                                 mystrix2_mpe_driver_image_size,
                                 mystrix2_mpe_driver_image_crc32,
                                 kOtaChunkSize,
                                 kOtaTimeoutMs)) {
        ESP_LOGE(kTag, "OTA failed: %s (status=%u)", mpe_host.lastError(), mpe_host.lastStatus());
        probe_started = false;
        vTaskDelete(nullptr);
        return;
      }
      ESP_LOGD(kTag, "firmware check/ota ok");

      CoprocessorLink::QueryResponse after = {};
      if (mpe_host.query(after, kQueryTimeoutMs)) {
        ESP_LOGI(kTag,
                 "post-ota query: status=%u proto=%u endpoint=%u flags=0x%02X bl_crc=0x%08" PRIX32
                 " app_crc=0x%08" PRIX32 " app_size=%" PRIu32 " app_max=%" PRIu32,
                 after.status,
                 after.protoVersion,
                 after.endpoint,
                 after.flags,
                 after.bootloaderCrc32,
                 after.appCrc32,
                 after.appSize,
                 after.appMaxSize);
        ESP_LOGD(kTag, "post-ota mcu firmware crc=0x%08" PRIX32 " size=%" PRIu32, after.appCrc32, after.appSize);
      }

      uint8_t jump_status = 0xFFU;
      if (!mpe_host.appJump(&jump_status, kQueryTimeoutMs)) {
        ESP_LOGE(kTag, "app jump failed: %s (status=%u)", mpe_host.lastError(), jump_status);
        probe_started = false;
        vTaskDelete(nullptr);
        return;
      }
      ESP_LOGD(kTag, "app jump ok");

      vTaskDelay(pdMS_TO_TICKS(50));

      CoprocessorLink::QueryResponse app_query = {};
      bool app_query_ok = false;
      for (uint32_t app_attempt = 0; app_attempt < 5U; ++app_attempt) {
        if (mpe_host.query(app_query, kOtaTimeoutMs)) {
          app_query_ok = true;
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
      }

      if (app_query_ok) {
        ESP_LOGI(kTag,
                 "post-jump query: status=%u proto=%u endpoint=%u flags=0x%02X bl_crc=0x%08" PRIX32
                 " app_crc=0x%08" PRIX32 " app_size=%" PRIu32 " app_max=%" PRIu32,
                 app_query.status,
                 app_query.protoVersion,
                 app_query.endpoint,
                 app_query.flags,
                 app_query.bootloaderCrc32,
                 app_query.appCrc32,
                 app_query.appSize,
                 app_query.appMaxSize);
      } else {
        ESP_LOGW(kTag, "post-jump query failed: %s (status=%u)", mpe_host.lastError(), mpe_host.lastStatus());
      }

      ESP_LOGI(kTag, "MPE init complete: firmware verified, OTA checked, app started");
      probe_started = false;
      vTaskDelete(nullptr);
    }
  }

  void Init() {
    if (host_initialized) {
      return;
    }

    if (!mpe_host.begin()) {
      ESP_LOGE(kTag, "host init failed: %s", mpe_host.lastError());
      return;
    }
    ESP_LOGD(kTag, "host begin ok");

    ESP_LOGI(kTag,
             "driver image ready: size=%" PRIu32 " crc=0x%08" PRIX32,
             mystrix2_mpe_driver_image_size,
             mystrix2_mpe_driver_image_crc32);
    ESP_LOGI(kTag,
             "coprocessor uart config: port=%d tx=%d rx=%d baud=%" PRIu32,
             (int)kMpeUartPort,
             (int)UPT2::KEYPAD_TX_PIN,
             (int)UPT2::KEYPAD_RX_PIN,
             kMpeBaud);

    host_initialized = true;
    ESP_LOGI(kTag, "MPE transport initialized");
  }

  void Start() {
    ESP_LOGI(kTag, "Start entered: host_initialized=%u probe_started=%u", host_initialized ? 1U : 0U, probe_started ? 1U : 0U);
    if (!host_initialized || probe_started) {
      ESP_LOGI(kTag, "Start skipped");
      return;
    }

    probe_started = true;
    BaseType_t ok = xTaskCreate([](void* arg) {
                                  (void)arg;
                                  ProbeTask(nullptr);
                                },
                                "mpe_probe",
                                4096,
                                nullptr,
                                1,
                                nullptr);
    if (ok != pdPASS) {
      probe_started = false;
      ESP_LOGE(kTag, "failed to create MPE probe task");
      return;
    }

    ESP_LOGI(kTag, "MPE probe task started");
  }

  IRAM_ATTR bool Scan() {
    return false;
  }
}
