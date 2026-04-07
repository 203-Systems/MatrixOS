#pragma once

#include <stdint.h>

#include "Drivers/CoprocessorLink/CoprocessorLinkHost.h"
#include "Drivers/CoprocessorLink/CoprocessorLinkTransport.h"
#include "Drivers/CoprocessorLink/MPEDriver/MPECoprocessorLinkProtocol.h"
#include "freertos/FreeRTOS.h"

namespace Device::KeyPad::MPE
{
  class MPECoprocessorLink
  {
   public:
    static constexpr char TAG[] = "MPE-Driver";
    static constexpr uint8_t MATRIX_SIZE = 8U;
    static constexpr uint8_t SECTOR_GRID_SIZE = 3U;
    static constexpr uint8_t MPE_DATA_SIZE = MATRIX_SIZE * SECTOR_GRID_SIZE;
    static constexpr uint16_t LOW_THRESHOLD = 512U;
    static constexpr uart_port_t MPE_UART_PORT = UART_NUM_1;
    static constexpr uint32_t MPE_BAUD = 2500000U;
    static constexpr uint32_t QUERY_TIMEOUT_MS = 150U;
    static constexpr uint32_t QUERY_POLL_INTERVAL_MS = 50U;
    static constexpr uint32_t QUERY_MAX_ATTEMPTS = 40U;
    static constexpr uint16_t OTA_CHUNK_SIZE = 128U;
    static constexpr uint32_t OTA_TIMEOUT_MS = 2000U;
    static constexpr uint32_t APP_COMMAND_TIMEOUT_MS = 150U;
    static constexpr uint32_t TASK_INTERVAL_MS = 5U;
    static constexpr uint32_t TASK_STACK_SIZE = 4096U;
    static constexpr UBaseType_t TASK_PRIORITY = 1U;
    static constexpr uint8_t INVALID_STATUS = 0xFFU;
    using MPEData = uint16_t[MPE_DATA_SIZE][MPE_DATA_SIZE];

    MPECoprocessorLink();

    void Init();
    void Start();

    const MPEData& GetMPEData() const;

   private:
    struct CachedChangesState
    {
      bool active = false;
      uint8_t count = 0U;
      uint8_t cursor = 0U;
    };

    void ResetCachedChanges();
    bool QueryWithRetries(const char* phase, CoprocessorLink::QueryResponse& response);
    bool QueryAppAfterJump(CoprocessorLink::QueryResponse& response);
    bool ProbeAndStartApp();
    bool CacheChanges();
    bool FetchChanges();
    void LogQueryStatus(const char* prefix, const CoprocessorLink::QueryResponse& query) const;
    void LogFetchedChange(const mpe_coprocessor_link_protocol_change_entry_t& entry) const;
    void ApplyChange(const mpe_coprocessor_link_protocol_change_entry_t& entry);

    void TaskLoop();
    static void TaskEntry(void* context);

    CoprocessorLink::UartTransport transport;
    CoprocessorLink::Host host;
    MPEData* mpe_live_data = nullptr;
    MPEData* mpe_snapshot_buffers[2] = {nullptr, nullptr};
    const MPEData* published_snapshot = nullptr;
    uint8_t snapshot_write_index = 0U;
    CachedChangesState cached_changes = {};
    bool host_initialized = false;
    bool task_started = false;
    bool app_ready = false;
  };
}
