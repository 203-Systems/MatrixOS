#include "Device.h"

#include "Drivers/CoprocessorLink/MPEDriver/MPECoprocessorLink.h"

#include <cstddef>
#include <cstring>
#include <inttypes.h>

#include "Drivers/CoprocessorLink/MPEDriver/Firmware/mystrix2_mpe_driver.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "Variants/Mystrix2/UltraProto2/Config.h"

namespace Device::KeyPad::MPE
{
namespace
{
// uint16_t ComputeCenterValue(const mpe_coprocessor_link_protocol_change_entry_t& entry)
// {
//     // reading layout is assumed to be:
//     //
//     //   [0] [1] [2]
//     //   [3]  C  [4]
//     //   [5] [6] [7]
//     //
//     // where C is the missing center value we want to estimate.
//     //
//     // edge points   = [1], [3], [4], [6]
//     // corner points = [0], [2], [5], [7]

//     const uint32_t edge_sum =
//         (uint32_t)entry.reading[1] +
//         (uint32_t)entry.reading[3] +
//         (uint32_t)entry.reading[4] +
//         (uint32_t)entry.reading[6];

//     const uint32_t corner_sum =
//         (uint32_t)entry.reading[0] +
//         (uint32_t)entry.reading[2] +
//         (uint32_t)entry.reading[5] +
//         (uint32_t)entry.reading[7];

//     // Generalized center estimation formula:
//     //
//     //   center = E + k * (E - C)
//     //
//     // where:
//     //   E = avg(edge)   = edge_sum / 4
//     //   C = avg(corner) = corner_sum / 4
//     //
//     // Expanding:
//     //
//     //   center = (1 + k) * edge_sum / 4 - k * corner_sum / 4
//     //
//     // To avoid floating point, use k_x100:
//     //
//     //   k = k_x100 / 100
//     //
//     // So:
//     //
//     //   center = ((100 + k_x100) * edge_sum - k_x100 * corner_sum) / 400
//     //
//     // Examples:
//     //   k_x100 = 100  -> k = 1.00  -> center = 2E - C
//     //   k_x100 = 135  -> k = 1.35
//     //   k_x100 = 50   -> k = 0.50

//     const uint16_t k_x100 = 135U;

//     const int64_t numerator =
//         (int64_t)(100U + k_x100) * (int64_t)edge_sum -
//         (int64_t)k_x100 * (int64_t)corner_sum;

//     // Clamp negative result to zero
//     if (numerator <= 0)
//     {
//         return 0U;
//     }

//     // Round to nearest integer when dividing by 400
//     const uint32_t center = (uint32_t)((numerator + 200) / 400);

//     // Saturate to uint16_t range
//     if (center > 65535U)
//     {
//         return 65535U;
//     }

//     return (uint16_t)center;
// }

uint16_t ComputeCenterValue(const mpe_coprocessor_link_protocol_change_entry_t& entry) {
  // reading layout:
  //
  //   [0] [1] [2]
  //   [3]  C  [4]
  //   [5] [6] [7]
  //
  // edge points   = [1], [3], [4], [6]
  // corner points = [0], [2], [5], [7]

  const uint32_t e0 = (uint32_t)entry.reading[1];
  const uint32_t e1 = (uint32_t)entry.reading[3];
  const uint32_t e2 = (uint32_t)entry.reading[4];
  const uint32_t e3 = (uint32_t)entry.reading[6];

  const uint32_t c0 = (uint32_t)entry.reading[0];
  const uint32_t c1 = (uint32_t)entry.reading[2];
  const uint32_t c2 = (uint32_t)entry.reading[5];
  const uint32_t c3 = (uint32_t)entry.reading[7];

  const uint32_t edge_sum = e0 + e1 + e2 + e3;
  const uint32_t corner_sum = c0 + c1 + c2 + c3;

  const uint32_t edgeAvg = (edge_sum + 2U) / 4U;
  const uint32_t cornerAvg = (corner_sum + 2U) / 4U;

  // Tunable gain:
  // center = E + k * max(E - C, 0)
  //
  // k = k_x100 / 100
  // Suggested starting point:
  //   50  -> conservative
  //   70  -> recommended default
  //   100 -> stronger boost
  const uint32_t k_x100 = 50U;

  uint32_t center = edgeAvg;

  if (edgeAvg > cornerAvg)
  {
    const uint32_t diff = edgeAvg - cornerAvg;

    // Rounded integer version of:
    // center += k * diff
    center += (k_x100 * diff + 50U) / 100U;
  }

  // Limit how much center can exceed the strongest neighbor.
  // margin_x100 = 20 -> allow up to +20%
  const uint32_t margin_x100 = 20U;

  uint32_t maxNeighbor = (uint32_t)entry.reading[0];
  for (uint32_t i = 1; i < 8; ++i)
  {
    const uint32_t v = (uint32_t)entry.reading[i];
    if (v > maxNeighbor)
    {
      maxNeighbor = v;
    }
  }

  const uint32_t maxAllowed = (maxNeighbor * (100U + margin_x100) + 50U) / 100U;

  if (center > maxAllowed)
  {
    center = maxAllowed;
  }

  if (center > 65535U)
  {
    center = 65535U;
  }

  return (uint16_t)center;
}
} // namespace

MPECoprocessorLink::MPECoprocessorLink() : transport(MPE_UART_PORT, UPT2::KEYPAD_TX_PIN, UPT2::KEYPAD_RX_PIN, MPE_BAUD), host(transport) {}

void MPECoprocessorLink::ResetCachedChanges() {
  cachedChanges = {};
}

void MPECoprocessorLink::LogQueryStatus(const char* prefix, const CoprocessorLink::QueryResponse& query) const {
  ESP_LOGI(TAG,
           "%s: status=%u proto=%u endpoint=%u flags=0x%02X bl_crc=0x%08" PRIX32 " app_crc=0x%08" PRIX32 " app_size=%" PRIu32
           " app_max=%" PRIu32,
           prefix, query.status, query.protoVersion, query.endpoint, query.flags, query.bootloaderCrc32, query.appCrc32, query.appSize,
           query.appMaxSize);
}

void MPECoprocessorLink::LogFetchedChange(const mpe_coprocessor_link_protocol_change_entry_t& entry) const {
  // ESP_LOGI(TAG,
  //          "fetch change: x=%u y=%u [%u %u %u %u %u %u %u %u]",
  //          entry.x,
  //          entry.y,
  //          entry.reading[0],
  //          entry.reading[1],
  //          entry.reading[2],
  //          entry.reading[3],
  //          entry.reading[4],
  //          entry.reading[5],
  //          entry.reading[6],
  //          entry.reading[7]);
}

bool MPECoprocessorLink::QueryWithRetries(const char* phase, CoprocessorLink::QueryResponse& response) {
  uint32_t attempts = 0U;

  ESP_LOGI(TAG, "starting %s query loop", phase);
  while ((attempts < QUERY_MAX_ATTEMPTS) && !host.query(response, QUERY_TIMEOUT_MS))
  {
    ESP_LOGW(TAG, "%s query attempt %u/%u failed: error=%s status=%u", phase, (unsigned)(attempts + 1U), (unsigned)QUERY_MAX_ATTEMPTS,
             host.lastError(), host.lastStatus());

    uint8_t rawRx[64];
    const size_t rawLen = transport.readAvailable(rawRx, sizeof(rawRx));
    if (rawLen > 0U)
    {
      ESP_LOGW(TAG, "%s query retry stray data: len=%u first=%02X %02X %02X %02X", phase, (unsigned)rawLen, rawRx[0],
               rawLen > 1 ? rawRx[1] : 0, rawLen > 2 ? rawRx[2] : 0, rawLen > 3 ? rawRx[3] : 0);
    }

    attempts++;
    vTaskDelay(pdMS_TO_TICKS(QUERY_POLL_INTERVAL_MS));
  }

  if (attempts >= QUERY_MAX_ATTEMPTS)
  {
    ESP_LOGW(TAG, "%s query failed: %s (status=%u)", phase, host.lastError(), host.lastStatus());
    return false;
  }

  return true;
}

bool MPECoprocessorLink::QueryAppAfterJump(CoprocessorLink::QueryResponse& response) {
  for (uint32_t attempt = 0U; attempt < 5U; ++attempt)
  {
    if (host.query(response, OTA_TIMEOUT_MS))
    {
      return true;
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }

  return false;
}

bool MPECoprocessorLink::ProbeAndStartApp() {
  CoprocessorLink::QueryResponse query = {};

  if (!QueryWithRetries("initial", query))
  {
    appReady = false;
    ResetCachedChanges();
    return false;
  }

  LogQueryStatus("query ok", query);

  if (query.endpoint == CoprocessorLink::COPROCESSOR_ENDPOINT_APP)
  {
    uint8_t enterStatus = INVALID_STATUS;

    ESP_LOGI(TAG, "coprocessor app detected, requesting enter bootloader");
    if (!host.enterBootloader(&enterStatus, QUERY_TIMEOUT_MS))
    {
      ESP_LOGE(TAG, "enter bootloader failed: %s (status=%u)", host.lastError(), enterStatus);
      appReady = false;
      ResetCachedChanges();
      return false;
    }

    transport.clearRx();
    vTaskDelay(pdMS_TO_TICKS(200));
    if (!host.begin())
    {
      ESP_LOGE(TAG, "host re-begin failed after enter bootloader: %s", host.lastError());
      appReady = false;
      ResetCachedChanges();
      return false;
    }

    transport.clearRx();
    if (!QueryWithRetries("bootloader", query))
    {
      ESP_LOGE(TAG, "bootloader did not come back after enter bootloader");
      appReady = false;
      ResetCachedChanges();
      return false;
    }

    LogQueryStatus("bootloader query", query);
  }

  if ((query.endpoint != CoprocessorLink::COPROCESSOR_ENDPOINT_BOOTLOADER) || !query.hasExtendedStatus)
  {
    ESP_LOGE(TAG, "invalid bootloader status: endpoint=%u extended=%u", query.endpoint, query.hasExtendedStatus ? 1U : 0U);
    appReady = false;
    ResetCachedChanges();
    return false;
  }

  if (!host.programImage(query, mystrix2_mpe_driver_image, mystrix2_mpe_driver_image_size, mystrix2_mpe_driver_image_crc32, OTA_CHUNK_SIZE,
                         OTA_TIMEOUT_MS))
  {
    ESP_LOGE(TAG, "OTA failed: %s (status=%u)", host.lastError(), host.lastStatus());
    appReady = false;
    ResetCachedChanges();
    return false;
  }

  uint8_t jumpStatus = INVALID_STATUS;
  if (!host.appJump(&jumpStatus, QUERY_TIMEOUT_MS))
  {
    ESP_LOGE(TAG, "app jump failed: %s (status=%u)", host.lastError(), jumpStatus);
    appReady = false;
    ResetCachedChanges();
    return false;
  }

  vTaskDelay(pdMS_TO_TICKS(50));

  CoprocessorLink::QueryResponse appQuery = {};
  const bool appQueryOk = QueryAppAfterJump(appQuery);
  if (appQueryOk)
  {
    LogQueryStatus("post-jump query", appQuery);
  }
  else
  {
    ESP_LOGW(TAG, "post-jump query failed: %s (status=%u)", host.lastError(), host.lastStatus());
  }

  appReady = appQueryOk && (appQuery.endpoint == CoprocessorLink::COPROCESSOR_ENDPOINT_APP);
  ResetCachedChanges();
  return appReady;
}

bool MPECoprocessorLink::CacheChanges() {
  CoprocessorLink::Frame response = {};
  mpe_coprocessor_link_protocol_cache_changes_rsp_t cache_response = {};

  if (!host.appTransact(MPE_COPROCESSOR_LINK_PROTOCOL_APP_CMD_CACHE_CHANGES, nullptr, 0U, response, APP_COMMAND_TIMEOUT_MS))
  {
    transport.clearRx();
    ESP_LOGW(TAG, "cache changes failed: %s (status=%u)", host.lastError(), host.lastStatus());
    return false;
  }

  if (response.length != sizeof(cache_response))
  {
    transport.clearRx();
    ESP_LOGW(TAG, "cache changes response length invalid: %u", response.length);
    return false;
  }

  memcpy(&cache_response, response.payload, sizeof(cache_response));
  if (cache_response.status != CoprocessorLink::kStatusOk)
  {
    ESP_LOGW(TAG, "cache changes rejected: status=%u", cache_response.status);
    return false;
  }

  cachedChanges.count = cache_response.cachedCount;
  cachedChanges.cursor = 0U;
  cachedChanges.active = (cachedChanges.count > 0U);
  if (cachedChanges.active)
  {
    // ESP_LOGI(TAG, "cache changes ok: cached=%u", cachedChanges.count);
  }

  return true;
}

void MPECoprocessorLink::ApplyChange(const mpe_coprocessor_link_protocol_change_entry_t& entry) {
  if (mpeLiveData == nullptr)
  {
    return;
  }

  if ((entry.x >= MATRIX_SIZE) || (entry.y >= MATRIX_SIZE))
  {
    return;
  }

  const uint8_t base_x = (uint8_t)(entry.x * SECTOR_GRID_SIZE);
  const uint8_t base_y = (uint8_t)(entry.y * SECTOR_GRID_SIZE);
  bool below_threshold = true;

  for (uint8_t index = 0U; index < 8U; ++index)
  {
    if (entry.reading[index] >= LOW_THRESHOLD)
    {
      below_threshold = false;
      break;
    }
  }

  if (below_threshold)
  {
    for (uint8_t local_x = 0U; local_x < SECTOR_GRID_SIZE; ++local_x)
    {
      for (uint8_t local_y = 0U; local_y < SECTOR_GRID_SIZE; ++local_y)
      {
        (*mpeLiveData)[base_x + local_x][base_y + local_y] = 0U;
      }
    }

    return;
  }

  // Side Value
  (*mpeLiveData)[base_x + 0U][base_y + 0U] = entry.reading[0];
  (*mpeLiveData)[base_x + 1U][base_y + 0U] = entry.reading[1];
  (*mpeLiveData)[base_x + 2U][base_y + 0U] = entry.reading[2];
  (*mpeLiveData)[base_x + 0U][base_y + 1U] = entry.reading[3];
  (*mpeLiveData)[base_x + 2U][base_y + 1U] = entry.reading[4];
  (*mpeLiveData)[base_x + 0U][base_y + 2U] = entry.reading[5];
  (*mpeLiveData)[base_x + 1U][base_y + 2U] = entry.reading[6];
  (*mpeLiveData)[base_x + 2U][base_y + 2U] = entry.reading[7];

  // Calculate center value
  (*mpeLiveData)[base_x + 1U][base_y + 1U] = ComputeCenterValue(entry);
}

bool MPECoprocessorLink::FetchChanges() {
  const uint8_t remaining = (cachedChanges.count > cachedChanges.cursor) ? (uint8_t)(cachedChanges.count - cachedChanges.cursor) : 0U;
  const uint8_t request_count =
      (remaining < MPE_COPROCESSOR_LINK_PROTOCOL_FETCH_MAX_ENTRIES) ? remaining : MPE_COPROCESSOR_LINK_PROTOCOL_FETCH_MAX_ENTRIES;
  constexpr uint16_t kFetchResponseHeaderSize = offsetof(mpe_coprocessor_link_protocol_fetch_changes_rsp_t, entries);
  mpe_coprocessor_link_protocol_fetch_changes_req_t request = {
      .cursor = cachedChanges.cursor,
      .maxCount = request_count,
  };
  mpe_coprocessor_link_protocol_fetch_changes_rsp_t response_payload = {};
  CoprocessorLink::Frame response = {};
  uint16_t expected_length = 0U;

  if (!cachedChanges.active || (request_count == 0U))
  {
    ResetCachedChanges();
    return false;
  }

  if (!host.appTransact(MPE_COPROCESSOR_LINK_PROTOCOL_APP_CMD_FETCH_CHANGES, &request, sizeof(request), response, APP_COMMAND_TIMEOUT_MS))
  {
    transport.clearRx();
    ESP_LOGW(TAG, "fetch changes failed at cursor=%u/%u: %s (status=%u)", cachedChanges.cursor, cachedChanges.count, host.lastError(),
             host.lastStatus());
    return false;
  }

  if (response.length < kFetchResponseHeaderSize)
  {
    transport.clearRx();
    ESP_LOGW(TAG, "fetch changes response too short: %u", response.length);
    return false;
  }

  memcpy(&response_payload, response.payload, kFetchResponseHeaderSize);
  if (response_payload.status != CoprocessorLink::kStatusOk)
  {
    ESP_LOGW(TAG, "fetch changes rejected: status=%u", response_payload.status);
    return false;
  }

  if (response_payload.returnedCount > request_count)
  {
    transport.clearRx();
    ESP_LOGW(TAG, "fetch changes returned too many entries: returned=%u requested=%u", response_payload.returnedCount, request_count);
    ResetCachedChanges();
    return false;
  }

  expected_length =
      (uint16_t)(kFetchResponseHeaderSize + ((uint16_t)response_payload.returnedCount * (uint16_t)sizeof(response_payload.entries[0])));
  if (response.length != expected_length)
  {
    transport.clearRx();
    ESP_LOGW(TAG, "fetch changes response length mismatch: got=%u expected=%u", response.length, expected_length);
    ResetCachedChanges();
    return false;
  }

  if (response_payload.returnedCount > 0U)
  {
    memcpy(response_payload.entries, response.payload + kFetchResponseHeaderSize, expected_length - kFetchResponseHeaderSize);
  }

  for (uint8_t entry_index = 0U; entry_index < response_payload.returnedCount; ++entry_index)
  {
    const auto& entry = response_payload.entries[entry_index];
    ApplyChange(entry);
    LogFetchedChange(entry);
  }

  if ((mpeLiveData != nullptr) && (mpeSnapshotBuffers[snapshotWriteIndex] != nullptr))
  {
    memcpy(mpeSnapshotBuffers[snapshotWriteIndex], mpeLiveData, sizeof(MPEData));
    publishedSnapshot = mpeSnapshotBuffers[snapshotWriteIndex];
    snapshotWriteIndex ^= 1U;
  }

  if (response_payload.returnedCount == 0U)
  {
    ResetCachedChanges();
    return false;
  }

  cachedChanges.cursor = (uint8_t)(cachedChanges.cursor + response_payload.returnedCount);
  if (cachedChanges.cursor >= cachedChanges.count)
  {
    ResetCachedChanges();
  }

  return true;
}

void MPECoprocessorLink::TaskEntry(void* context) {
  static_cast<MPECoprocessorLink*>(context)->TaskLoop();
}

void MPECoprocessorLink::TaskLoop() {
  while (true)
  {
    if (!hostInitialized)
    {
      vTaskDelay(pdMS_TO_TICKS(TASK_INTERVAL_MS));
      continue;
    }

    if (!appReady)
    {
      (void)ProbeAndStartApp();
      vTaskDelay(pdMS_TO_TICKS(TASK_INTERVAL_MS));
      continue;
    }

    if (!cachedChanges.active)
    {
      if (!CacheChanges())
      {
        vTaskDelay(pdMS_TO_TICKS(TASK_INTERVAL_MS));
        continue;
      }

      if (!cachedChanges.active)
      {
        vTaskDelay(pdMS_TO_TICKS(TASK_INTERVAL_MS));
        continue;
      }
    }

    (void)FetchChanges();
    vTaskDelay(pdMS_TO_TICKS(TASK_INTERVAL_MS));
  }
}

void MPECoprocessorLink::Init() {
  if (hostInitialized)
  {
    return;
  }

  if (mpeLiveData == nullptr)
  {
    mpeLiveData = (MPEData*)heap_caps_calloc(1, sizeof(MPEData), MALLOC_CAP_8BIT);
    mpeSnapshotBuffers[0] = (MPEData*)heap_caps_calloc(1, sizeof(MPEData), MALLOC_CAP_8BIT);
    mpeSnapshotBuffers[1] = (MPEData*)heap_caps_calloc(1, sizeof(MPEData), MALLOC_CAP_8BIT);
    if ((mpeLiveData == nullptr) || (mpeSnapshotBuffers[0] == nullptr) || (mpeSnapshotBuffers[1] == nullptr))
    {
      ESP_LOGE(TAG, "failed to allocate MPE buffers");
      return;
    }
  }

  memset(mpeLiveData, 0, sizeof(MPEData));
  memset(mpeSnapshotBuffers[0], 0, sizeof(MPEData));
  memset(mpeSnapshotBuffers[1], 0, sizeof(MPEData));
  publishedSnapshot = mpeSnapshotBuffers[0];
  snapshotWriteIndex = 1U;
  if (!host.begin())
  {
    ESP_LOGE(TAG, "host init failed: %s", host.lastError());
    return;
  }

  appReady = false;
  ResetCachedChanges();
  hostInitialized = true;
  ESP_LOGI(TAG, "MPE transport initialized");
  ESP_LOGD(TAG, "driver image ready: size=%" PRIu32 " crc=0x%08" PRIX32, mystrix2_mpe_driver_image_size, mystrix2_mpe_driver_image_crc32);
  ESP_LOGD(TAG, "coprocessor uart config: port=%d tx=%d rx=%d baud=%" PRIu32, (int)MPE_UART_PORT, (int)UPT2::KEYPAD_TX_PIN,
           (int)UPT2::KEYPAD_RX_PIN, MPE_BAUD);
}

void MPECoprocessorLink::Start() {
  if (!hostInitialized || taskStarted)
  {
    return;
  }

  const BaseType_t result = xTaskCreate(TaskEntry, "mpe_link", TASK_STACK_SIZE, this, TASK_PRIORITY, nullptr);
  if (result != pdPASS)
  {
    ESP_LOGE(TAG, "failed to create task: mpe_link");
    return;
  }

  appReady = false;
  ResetCachedChanges();
  taskStarted = true;
}

const MPECoprocessorLink::MPEData& MPECoprocessorLink::GetMPEData() const {
  return *publishedSnapshot;
}
} // namespace Device::KeyPad::MPE
