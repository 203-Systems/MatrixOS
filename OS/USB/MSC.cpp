#include "MatrixOS.h"
#include "tusb.h"
#include "Device.h"
#include "class/msc/msc.h"
#include "Storage/StorageMutex.h"

#define DEVICE_STORAGE 1

//--------------------------------------------------------------------+
// MSC callbacks
//--------------------------------------------------------------------+

// Invoked when received SCSI_CMD_INQUIRY
// Application fills vendor id, product id and revision with string up to 8, 16, 4 characters respectively
uint32_t tud_msc_inquiry2_cb(uint8_t lun, scsi_inquiry_resp_t* inquiry_resp) {
  // MatrixOS::Logging::LogInfo("MSC", "INQUIRY - LUN: %d", lun);

  const char vid[] = "MatrixOS";
  const char pid[] = "Mass Storage";
  const char rev[] = "1.0";

  memcpy(inquiry_resp->vendor_id  , vid, strlen(vid));
  memcpy(inquiry_resp->product_id , pid, strlen(pid));
  memcpy(inquiry_resp->product_rev, rev, strlen(rev));

  // MatrixOS::Logging::LogInfo("MSC", "INQUIRY response sent");
  return sizeof(scsi_inquiry_resp_t);
}

// Invoked when received Test Unit Ready command.
// Return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
  // MatrixOS::Logging::LogInfo("MSC", "TEST_UNIT_READY - LUN: %d", lun);

  const Device::Storage::Status* status = Device::Storage::GetStatus();

  // MatrixOS::Logging::LogInfo("MSC", "Storage - Available: %s, Sectors: %d, SectorSize: %d, WriteProtected: %s",
  //   status->available ? "YES" : "NO",
  //   status->sector_count,
  //   status->sector_size,
  //   status->write_protected ? "YES" : "NO");

  return status->available;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
  // MatrixOS::Logging::LogInfo("MSC", "CAPACITY - LUN: %d", lun);

  #if DEVICE_STORAGE == 1
  const Device::Storage::Status* status = Device::Storage::GetStatus();
  if (status->available) {
    *block_count = status->sector_count;
    *block_size = status->sector_size;
    // MatrixOS::Logging::LogInfo("MSC", "Reporting capacity - Blocks: %d, BlockSize: %d", *block_count, *block_size);
  } else {
    // When no storage available, still report minimal capacity
    // Actual I/O operations will fail with proper error codes
    *block_count = 1; // Minimal 1 sector
    *block_size = 512;
    // MatrixOS::Logging::LogWarning("MSC", "Storage not available - minimal capacity reported");
  }
  #else
  // Default minimal capacity when storage driver not available
  *block_count = 1; // Minimal 1 sector
  *block_size = 512;
  // MatrixOS::Logging::LogWarning("MSC", "Storage driver disabled - minimal capacity reported");
  #endif
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
  // MatrixOS::Logging::LogInfo("MSC", "START_STOP - LUN: %d, PowerCond: %d, Start: %s, LoadEject: %s",
  //   lun, power_condition, start ? "YES" : "NO", load_eject ? "YES" : "NO");

  if (load_eject) {
    if (start) {
      // Load disk storage - nothing to do as our storage is always available
      // MatrixOS::Logging::LogInfo("MSC", "Load disk requested");
    } else {
      // Unload disk storage - nothing to do as we don't support ejection
      // MatrixOS::Logging::LogInfo("MSC", "Unload disk requested");
    }
  }

  return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  // MatrixOS::Logging::LogInfo("MSC", "READ10 - LUN: %d, LBA: %d, Offset: %d, BufSize: %d", lun, lba, offset, bufsize);

  #if DEVICE_STORAGE == 1
  // For reads, we need to handle partial sector reads
  // Calculate which sectors we need to read
  uint32_t start_byte = (lba * 512) + offset;
  uint32_t start_sector = start_byte / 512;
  uint32_t end_byte = start_byte + bufsize - 1;
  uint32_t end_sector = end_byte / 512;
  uint32_t sector_count = end_sector - start_sector + 1;

  // MatrixOS::Logging::LogInfo("MSC", "Reading sectors: start=%d, count=%d for bytes %d-%d",
  //   start_sector, sector_count, start_byte, end_byte);

  // If we need to read partial sectors, use a cached sector buffer
  if (sector_count * 512 != bufsize || (start_byte % 512) != 0) {
    // Partial sector read - use sector cache for efficiency
    static uint8_t cached_sector[512];
    static uint32_t cached_sector_lba = UINT32_MAX; // Invalid LBA initially

    if (sector_count > 1) {
      MatrixOS::Logging::LogError("MSC", "READ10 - Partial read spans multiple sectors: %d", sector_count);
      return TUD_MSC_RET_ERROR;
    }

    // Check if we already have this sector cached
    bool cache_hit = (cached_sector_lba == start_sector);

    if (!cache_hit) {
      // Need to read new sector
      // Lock storage mutex before reading
      MatrixOS::Storage::Lock lock(1000); // 1 second timeout
      if (!lock.IsLocked()) {
        MatrixOS::Logging::LogError("MSC", "Failed to acquire storage lock for partial read");
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
        return -1;
      }

      bool result = Device::Storage::ReadSectors(0, start_sector, cached_sector, 1);
      if (!result) {
        MatrixOS::Logging::LogError("MSC", "Partial read failed");
        cached_sector_lba = UINT32_MAX; // Invalidate cache
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
        return -1;
      }
      cached_sector_lba = start_sector;
    }

    // Copy the requested portion from cached sector
    uint32_t buffer_offset = start_byte - (start_sector * 512);
    memcpy(buffer, cached_sector + buffer_offset, bufsize);
    // MatrixOS::Logging::LogInfo("MSC", "Partial read successful - copied %d bytes from offset %d (cached: %s)",
    //   bufsize, buffer_offset, cache_hit ? "hit" : "miss");
  } else {
    // Full sector aligned read
    // Lock storage mutex before reading
    MatrixOS::Storage::Lock lock(1000); // 1 second timeout
    if (!lock.IsLocked()) {
      MatrixOS::Logging::LogError("MSC", "Failed to acquire storage lock for full read");
      tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
      return -1;
    }

    bool result = Device::Storage::ReadSectors(0, start_sector, (uint8_t*)buffer, sector_count);
    // MatrixOS::Logging::LogInfo("MSC", "Full read result: %s", result ? "SUCCESS" : "FAILED");

    if (!result) {
      MatrixOS::Logging::LogError("MSC", "Full read failed");
      tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
      return -1;
    }
  }

  return bufsize;
  #else
  // Return dummy data (all zeros) for debugging
  memset(buffer, 0, bufsize);
  // MatrixOS::Logging::LogWarning("MSC", "READ10 - returning dummy data (storage disabled)");
  return bufsize;
  #endif
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  // MatrixOS::Logging::LogInfo("MSC", "WRITE10 - LUN: %d, LBA: %d, Offset: %d, BufSize: %d", lun, lba, offset, bufsize);

  #if DEVICE_STORAGE == 1
  // Calculate which sectors we need to write
  uint32_t start_byte = (lba * 512) + offset;
  uint32_t start_sector = start_byte / 512;
  uint32_t end_byte = start_byte + bufsize - 1;
  uint32_t end_sector = end_byte / 512;
  uint32_t sector_count = end_sector - start_sector + 1;

  // MatrixOS::Logging::LogInfo("MSC", "Writing sectors: start=%d, count=%d", start_sector, sector_count);

  // Check if we need partial sector writes (read-modify-write)
  if (sector_count * 512 != bufsize || (start_byte % 512) != 0) {
    // Partial sector write - need read-modify-write
    static uint8_t temp_sector[512];

    if (sector_count > 1) {
      MatrixOS::Logging::LogError("MSC", "WRITE10 - Partial write spans multiple sectors: %d", sector_count);
      return TUD_MSC_RET_ERROR;
    }

    // Lock storage mutex before read-modify-write
    MatrixOS::Storage::Lock lock(1000); // 1 second timeout
    if (!lock.IsLocked()) {
      MatrixOS::Logging::LogError("MSC", "Failed to acquire storage lock for partial write");
      tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
      return -1;
    }

    // Read existing sector
    bool result = Device::Storage::ReadSectors(0, start_sector, temp_sector, 1);
    if (!result) {
      MatrixOS::Logging::LogError("MSC", "Failed to read sector for partial write");
      tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
      return -1;
    }

    // Modify the relevant portion
    uint32_t buffer_offset = start_byte - (start_sector * 512);
    memcpy(temp_sector + buffer_offset, buffer, bufsize);

    // Write back the modified sector
    result = Device::Storage::WriteSectors(0, start_sector, temp_sector, 1);
    if (!result) {
      MatrixOS::Logging::LogError("MSC", "Failed to write modified sector");
      tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
      return -1;
    }
  } else {
    // Full sector aligned write
    // Lock storage mutex before writing
    MatrixOS::Storage::Lock lock(1000); // 1 second timeout
    if (!lock.IsLocked()) {
      MatrixOS::Logging::LogError("MSC", "Failed to acquire storage lock for write");
      tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
      return -1;
    }

    // Try to write to real storage first
    bool result = Device::Storage::WriteSectors(0, start_sector, buffer, sector_count);

    // MatrixOS::Logging::LogInfo("MSC", "WriteSectors result: %s", result ? "SUCCESS" : "FAILED");

    if (!result) {
      // If real storage fails, set sense data and return error
      tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3A, 0x00);
      MatrixOS::Logging::LogError("MSC", "WRITE10 failed - sense data set");
      return -1;
    }
  }

  return bufsize;
  #else
  // Pretend write succeeded for debugging
  // MatrixOS::Logging::LogWarning("MSC", "WRITE10 - pretending success (storage disabled)");
  return bufsize;
  #endif
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
  // MatrixOS::Logging::LogInfo("MSC", "SCSI_CMD - LUN: %d, CMD: 0x%02X, BufSize: %d", lun, scsi_cmd[0], bufsize);

  // read10 & write10 has their own callback and MUST not be handled here

  void const* response = NULL;
  int32_t resplen = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0]) {
    default:
      // Set Sense = Invalid Command Operation
      // MatrixOS::Logging::LogWarning("MSC", "Unknown SCSI command: 0x%02X", scsi_cmd[0]);
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
      resplen = -1;
    break;
  }

  // return resplen must not larger than bufsize
  if (resplen > bufsize) resplen = bufsize;

  if (response && (resplen > 0)) {
    if (in_xfer) {
      memcpy(buffer, response, (size_t) resplen);
    } else {
      // SCSI output
    }
  }

  // MatrixOS::Logging::LogInfo("MSC", "SCSI_CMD response length: %d", resplen);
  return (int32_t) resplen;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void tud_msc_write10_complete_cb(uint8_t lun) {
  // MatrixOS::Logging::LogInfo("MSC", "WRITE10_COMPLETE - LUN: %d", lun);

  // Could implement cache flushing here if needed
}

// Invoked when received GET_MAX_LUN request, required for multiple LUNs implementation
uint8_t tud_msc_get_maxlun_cb(void) {
  // MatrixOS::Logging::LogInfo("MSC", "GET_MAX_LUN - returning 1");
  return 1; // Single LUN
}

// Invoked to check if device is writable as part of SCSI WRITE10
bool tud_msc_is_writable_cb (uint8_t lun) {
  // MatrixOS::Logging::LogInfo("MSC", "IS_WRITABLE - LUN: %d", lun);

  #if DEVICE_STORAGE == 1
  const Device::Storage::Status* status = Device::Storage::GetStatus();

  bool writable = status->available && !status->write_protected;
  // MatrixOS::Logging::LogInfo("MSC", "Writable: %s (available=%s, write_protected=%s)",
  //   writable ? "YES" : "NO",
  //   status->available ? "YES" : "NO",
  //   status->write_protected ? "YES" : "NO");

  return writable;
  #else
  // MatrixOS::Logging::LogWarning("MSC", "Storage disabled - not writable");
  return false;
  #endif
}