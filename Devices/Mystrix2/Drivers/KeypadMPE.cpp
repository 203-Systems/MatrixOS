#include "Device.h"

#include <cstdlib>
#include <cstring>
#include <inttypes.h>

#include "CoprocessorLink/CoprocessorLinkHost.h"
#include "CoprocessorLink/Mystrix2MPEProtocol.h"
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
    constexpr uint32_t kAppCommandTimeoutMs = 150U;
    constexpr uint32_t kScanTaskIntervalMs = 5U;
    constexpr uint8_t kMpeMatrixXSize = 8U;
    constexpr uint8_t kMpeMatrixYSize = 8U;
    constexpr uint8_t kMpeSectorCount = 8U;

    struct KeyMatrixScanFrame {
      uint16_t reading[kMpeMatrixXSize][kMpeMatrixYSize][kMpeSectorCount];
    };

    static_assert(X_SIZE == kMpeMatrixXSize, "Mystrix2 MPE matrix width mismatch");
    static_assert(Y_SIZE == kMpeMatrixYSize, "Mystrix2 MPE matrix height mismatch");

    CoprocessorLink::UartTransport mpe_transport(kMpeUartPort, UPT2::KEYPAD_TX_PIN, UPT2::KEYPAD_RX_PIN, kMpeBaud);
    CoprocessorLink::Host mpe_host(mpe_transport);
    KeyMatrixScanFrame* scan_frame = nullptr;
    bool host_initialized = false;
    bool probe_started = false;
    bool scan_task_started = false;
    bool app_ready = false;
    bool cached_changes_active = false;
    uint8_t cached_change_count = 0U;
    uint8_t cached_change_cursor = 0U;

    void ResetCachedChanges() {
      cached_changes_active = false;
      cached_change_count = 0U;
      cached_change_cursor = 0U;
    }

    bool CacheChanges() {
      CoprocessorLink::Frame response = {};
      mystrix2_mpe_cache_changes_rsp_t cache_response = {};

      if (!mpe_host.appTransact(MYSTRIX2_MPE_APP_CMD_CACHE_CHANGES, nullptr, 0U, response, kAppCommandTimeoutMs)) {
        mpe_transport.clearRx();
        ESP_LOGW(kTag, "cache changes failed: %s (status=%u)", mpe_host.lastError(), mpe_host.lastStatus());
        return false;
      }

      if (response.length != sizeof(cache_response)) {
        mpe_transport.clearRx();
        ESP_LOGW(kTag, "cache changes response length invalid: %u", response.length);
        return false;
      }

      memcpy(&cache_response, response.payload, sizeof(cache_response));
      if (cache_response.status != CoprocessorLink::kStatusOk) {
        ESP_LOGW(kTag, "cache changes rejected: status=%u", cache_response.status);
        return false;
      }

      cached_change_count = cache_response.cachedCount;
      cached_change_cursor = 0U;
      cached_changes_active = (cached_change_count > 0U);
      if (cached_change_count > 0U) {
        // ESP_LOGI(kTag, "cache changes ok: cached=%u", cached_change_count);
      }
      return true;
    }

    bool FetchChanges() {
      const uint8_t remaining = (cached_change_count > cached_change_cursor)
                                    ? (uint8_t)(cached_change_count - cached_change_cursor)
                                    : 0U;
      const uint8_t request_count =
          (remaining < MYSTRIX2_MPE_FETCH_MAX_ENTRIES) ? remaining : MYSTRIX2_MPE_FETCH_MAX_ENTRIES;
      mystrix2_mpe_fetch_changes_req_t request = {
          .cursor = cached_change_cursor,
          .maxCount = request_count,
      };
      mystrix2_mpe_fetch_changes_rsp_t response_payload = {};
      CoprocessorLink::Frame response = {};
      uint16_t expected_length = 0U;
      uint8_t entry_index = 0U;

      if ((scan_frame == nullptr) || !cached_changes_active || (request_count == 0U)) {
        ResetCachedChanges();
        return false;
      }

      if (!mpe_host.appTransact(
              MYSTRIX2_MPE_APP_CMD_FETCH_CHANGES, &request, sizeof(request), response, kAppCommandTimeoutMs)) {
        mpe_transport.clearRx();
        ESP_LOGW(kTag,
                 "fetch changes failed at cursor=%u/%u: %s (status=%u)",
                 cached_change_cursor,
                 cached_change_count,
                 mpe_host.lastError(),
                 mpe_host.lastStatus());
        return false;
      }

      if (response.length < 2U) {
        mpe_transport.clearRx();
        ESP_LOGW(kTag, "fetch changes response too short: %u", response.length);
        return false;
      }

      memcpy(&response_payload, response.payload, response.length);
      if (response_payload.status != CoprocessorLink::kStatusOk) {
        ESP_LOGW(kTag, "fetch changes rejected: status=%u", response_payload.status);
        return false;
      }

      if (response_payload.returnedCount > request_count) {
        mpe_transport.clearRx();
        ESP_LOGW(kTag,
                 "fetch changes returned too many entries: returned=%u requested=%u",
                 response_payload.returnedCount,
                 request_count);
        ResetCachedChanges();
        return false;
      }

      expected_length =
          (uint16_t)(2U + ((uint16_t)response_payload.returnedCount * (uint16_t)sizeof(response_payload.entries[0])));
      if (response.length != expected_length) {
        mpe_transport.clearRx();
        ESP_LOGW(kTag, "fetch changes response length mismatch: got=%u expected=%u", response.length, expected_length);
        ResetCachedChanges();
        return false;
      }

      for (entry_index = 0U; entry_index < response_payload.returnedCount; ++entry_index) {
        const auto& entry = response_payload.entries[entry_index];
        if ((entry.x >= kMpeMatrixXSize) || (entry.y >= kMpeMatrixYSize)) {
          continue;
        }

        memcpy(scan_frame->reading[entry.x][entry.y], entry.reading, sizeof(entry.reading));
        ESP_LOGI(kTag,
                 "fetch change: x=%u y=%u [%u %u %u %u %u %u %u %u]",
                 entry.x,
                 entry.y,
                 entry.reading[0],
                 entry.reading[1],
                 entry.reading[2],
                 entry.reading[3],
                 entry.reading[4],
                 entry.reading[5],
                 entry.reading[6],
                 entry.reading[7]);
      }

      if (response_payload.returnedCount == 0U) {
        ResetCachedChanges();
        return false;
      }

      cached_change_cursor = (uint8_t)(cached_change_cursor + response_payload.returnedCount);
      // ESP_LOGI(kTag,
      //          "fetch changes ok: returned=%u cursor=%u/%u",
      //          response_payload.returnedCount,
      //          cached_change_cursor,
      //          cached_change_count);
      if (cached_change_cursor >= cached_change_count) {
        ResetCachedChanges();
      }

      return response_payload.returnedCount > 0U;
    }

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
        app_ready = false;
        ResetCachedChanges();
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
          app_ready = false;
          ResetCachedChanges();
          probe_started = false;
          vTaskDelete(nullptr);
          return;
        }

        ESP_LOGI(kTag, "enter bootloader acked, waiting for bootloader");
        mpe_transport.clearRx();
        vTaskDelay(pdMS_TO_TICKS(200));
        if (!mpe_host.begin()) {
          ESP_LOGE(kTag, "host re-begin failed after enter bootloader: %s", mpe_host.lastError());
          app_ready = false;
          ResetCachedChanges();
          probe_started = false;
          vTaskDelete(nullptr);
          return;
        }
        mpe_transport.clearRx();

        if (!queryWithRetries("bootloader", query)) {
          ESP_LOGE(kTag, "bootloader did not come back after enter bootloader");
          app_ready = false;
          ResetCachedChanges();
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
        app_ready = false;
        ResetCachedChanges();
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
        app_ready = false;
        ResetCachedChanges();
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
        app_ready = false;
        ResetCachedChanges();
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

      app_ready = app_query_ok && (app_query.endpoint == CoprocessorLink::COPROCESSOR_ENDPOINT_APP);
      ResetCachedChanges();
      ESP_LOGI(kTag, "MPE init complete: firmware verified, OTA checked, app started");
      probe_started = false;
      vTaskDelete(nullptr);
    }

    void ScanTask(void*) {
      ESP_LOGI(kTag, "ScanTask entered");
      while (true) {
        if (!host_initialized || !app_ready || probe_started || (scan_frame == nullptr)) {
          vTaskDelay(pdMS_TO_TICKS(kScanTaskIntervalMs));
          continue;
        }

        if (!cached_changes_active) {
          if (!CacheChanges()) {
            vTaskDelay(pdMS_TO_TICKS(kScanTaskIntervalMs));
            continue;
          }

          if (!cached_changes_active) {
            vTaskDelay(pdMS_TO_TICKS(kScanTaskIntervalMs));
            continue;
          }
        }

        (void)FetchChanges();
        vTaskDelay(pdMS_TO_TICKS(kScanTaskIntervalMs));
      }
    }
  }

  void Init() {
    if (host_initialized) {
      return;
    }

    if (scan_frame == nullptr) {
      scan_frame = static_cast<KeyMatrixScanFrame*>(malloc(sizeof(KeyMatrixScanFrame)));
      if (scan_frame == nullptr) {
        ESP_LOGE(kTag, "failed to allocate MPE scan frame");
        return;
      }
      memset(scan_frame, 0, sizeof(KeyMatrixScanFrame));
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

    app_ready = false;
    ResetCachedChanges();
    host_initialized = true;
    ESP_LOGI(kTag, "MPE transport initialized");
    ESP_LOGI(kTag, "MPE local scan frame allocated: %u bytes", (unsigned)sizeof(KeyMatrixScanFrame));
  }

  void Start() {
    ESP_LOGI(kTag,
             "Start entered: host_initialized=%u probe_started=%u scan_task_started=%u",
             host_initialized ? 1U : 0U,
             probe_started ? 1U : 0U,
             scan_task_started ? 1U : 0U);
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

    if (!scan_task_started) {
      ok = xTaskCreate([](void* arg) {
                         (void)arg;
                         ScanTask(nullptr);
                       },
                       "mpe_scan",
                       4096,
                       nullptr,
                       1,
                       nullptr);
      if (ok != pdPASS) {
        scan_task_started = false;
        ESP_LOGE(kTag, "failed to create MPE scan task");
        return;
      }

      scan_task_started = true;
      ESP_LOGI(kTag, "MPE scan task started");
    }

    app_ready = false;
    ResetCachedChanges();
    ESP_LOGI(kTag, "MPE probe task started");
  }

  IRAM_ATTR bool Scan() {
    return false;
  }
}
