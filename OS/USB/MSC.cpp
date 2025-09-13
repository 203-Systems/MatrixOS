#include "MatrixOS.h"
#include "tusb.h"

#if CFG_TUD_MSC
#include "diskio.h"
#include "../FS/FS.h"

//--------------------------------------------------------------------+
// MSC callbacks
//--------------------------------------------------------------------+

// Invoked when received SCSI_CMD_INQUIRY
// Application fills vendor id, product id and revision with string up to 8, 16, 4 characters respectively
uint32_t tud_msc_inquiry2_cb(uint8_t lun, scsi_inquiry_resp_t* inquiry_resp) {
  (void) lun;

  const char vid[] = "MatrixOS";
  const char pid[] = "Mass Storage";
  const char rev[] = "1.0";

  memcpy(inquiry_resp->vendor_id  , vid, strlen(vid));
  memcpy(inquiry_resp->product_id , pid, strlen(pid));
  memcpy(inquiry_resp->product_rev, rev, strlen(rev));

  return sizeof(scsi_inquiry_resp_t);
}

// Invoked when received Test Unit Ready command.
// Return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
  (void) lun;

  #if DEVICE_FATFS == 1
  return MatrixOS::FS::Available();
  #else
  return false;
  #endif
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
  (void) lun;

  #if DEVICE_FATFS == 1
  // Get capacity from Device layer
  uint8_t result = Device::FatFS::IOControl(0, GET_SECTOR_COUNT, block_count);
  if (result == 0) {
    *block_size = 512; // Standard sector size
  } else {
    // Default values if unable to get capacity
    *block_count = 0;
    *block_size = 512;
  }
  #else
  *block_count = 0;
  *block_size = 512;
  #endif
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
  (void) lun;
  (void) power_condition;

  if (load_eject) {
    if (start) {
      // Load disk storage - nothing to do as our storage is always available
    } else {
      // Unload disk storage - nothing to do as we don't support ejection
    }
  }

  return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  (void) lun;

  #if DEVICE_FATFS == 1
  // Check if we can read this sector
  if (offset % 512 != 0 || bufsize % 512 != 0) {
    return TUD_MSC_RET_ERROR;
  }

  uint32_t sector_count = bufsize / 512;
  uint32_t start_sector = lba + (offset / 512);

  uint8_t result = Device::FatFS::Read(0, (uint8_t*)buffer, start_sector, sector_count);

  if (result == 0) {
    return bufsize;
  } else {
    return TUD_MSC_RET_ERROR;
  }
  #else
  (void) lba;
  (void) offset;
  (void) buffer;
  (void) bufsize;
  return TUD_MSC_RET_ERROR;
  #endif
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  (void) lun;

  #if DEVICE_FATFS == 1
  // Check if we can write this sector
  if (offset % 512 != 0 || bufsize % 512 != 0) {
    return TUD_MSC_RET_ERROR;
  }

  uint32_t sector_count = bufsize / 512;
  uint32_t start_sector = lba + (offset / 512);

  uint8_t result = Device::FatFS::Write(0, buffer, start_sector, sector_count);

  if (result == 0) {
    return bufsize;
  } else {
    return TUD_MSC_RET_ERROR;
  }
  #else
  (void) lba;
  (void) offset;
  (void) buffer;
  (void) bufsize;
  return TUD_MSC_RET_ERROR;
  #endif
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
  // read10 & write10 has their own callback and MUST not be handled here

  void const* response = NULL;
  int32_t resplen = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0]) {
    default:
      // Set Sense = Invalid Command Operation
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

  return (int32_t) resplen;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void tud_msc_write10_complete_cb(uint8_t lun) {
  (void) lun;

  // Could implement cache flushing here if needed
}

// Invoked when received GET_MAX_LUN request, required for multiple LUNs implementation
uint8_t tud_msc_get_maxlun_cb(void) {
  return 1; // Single LUN
}

// Invoked to check if device is writable as part of SCSI WRITE10
bool tud_msc_is_writable_cb (uint8_t lun) {
  (void) lun;

  #if DEVICE_FATFS == 1
  return MatrixOS::FS::Available();
  #else
  return false;
  #endif
}

#endif // CFG_TUD_MSC