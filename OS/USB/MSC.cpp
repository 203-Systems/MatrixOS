#include "MatrixOS.h"
#include "tusb.h"
#include "Device.h"
#include "class/msc/msc.h"

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

  const Device::Storage::StorageStatus* status = Device::Storage::Status();

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
  const Device::Storage::StorageStatus* status = Device::Storage::Status();
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
  #if DEVICE_STORAGE == 1
  // Simple passthrough to Storage layer - ESP-IDF style
  uint32_t sector_count = bufsize / 512;  // Convert bytes to sector count (Hard code this for now to improve performance)
  if (!Device::Storage::ReadSectors(lba, sector_count, buffer)) {
    tud_msc_set_sense(lun, SCSI_SENSE_MEDIUM_ERROR, 0x11, 0x00);
    return -1;
  }

  return bufsize;
  #else
  // Return dummy data (all zeros) for debugging
  memset(buffer, 0, bufsize);
  return bufsize;
  #endif
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  #if DEVICE_STORAGE == 1
  // Simple passthrough to Storage layer - ESP-IDF style
  uint32_t sector_count = bufsize / 512;  // Convert bytes to sector count (Hard code this for now to improve performance)
  if (!Device::Storage::WriteSectors(lba, sector_count, buffer)) {
    tud_msc_set_sense(lun, SCSI_SENSE_MEDIUM_ERROR, 0x11, 0x00);
    return -1;
  }

  return bufsize;
  #else
  // Pretend write succeeded for debugging
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
  const Device::Storage::StorageStatus* status = Device::Storage::Status();

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