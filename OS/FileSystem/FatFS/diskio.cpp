/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for MatrixOS FatFs     (C)203 Systems      */
/*-----------------------------------------------------------------------*/
/* This module bridges FatFs to the MatrixOS Device layer               */
/*-----------------------------------------------------------------------*/

#include "ff.h"     /* Obtains integer types */
#include "diskio.h" /* Declarations of disk functions */
#include "Device.h" /* MatrixOS Device layer interface */

extern "C" {

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                     */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
  BYTE pdrv /* Physical drive number to identify the drive */
)
{
#if DEVICE_STORAGE == 1
  const Device::Storage::StorageStatus* status = Device::Storage::Status();

  if (!status->available)
    return STA_NOINIT;

  if (status->write_protected)
    return STA_PROTECT;

  return 0; // Ready
#else
  return STA_NOINIT;
#endif
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                   */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
  BYTE pdrv /* Physical drive number to identify the drive */
)
{
#if DEVICE_STORAGE == 1
  // MSC subsystem handles initialization, just return current status
  return disk_status(pdrv);
#else
  return STA_NOINIT;
#endif
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
  BYTE pdrv,    /* Physical drive number to identify the drive */
  BYTE* buff,   /* Data buffer to store read data */
  LBA_t sector, /* Start sector in LBA */
  UINT count    /* Number of sectors to read */
)
{
#if DEVICE_STORAGE == 1
  bool result = Device::Storage::ReadSectors(sector, count, buff);
  return result ? RES_OK : RES_ERROR;
#else
  return RES_NOTRDY;
#endif
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                      */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
  BYTE pdrv,        /* Physical drive number to identify the drive */
  const BYTE* buff, /* Data to be written */
  LBA_t sector,     /* Start sector in LBA */
  UINT count        /* Number of sectors to write */
)
{
#if DEVICE_STORAGE == 1
  bool result = Device::Storage::WriteSectors(sector, count, buff);
  return result ? RES_OK : RES_ERROR;
#else
  return RES_NOTRDY;
#endif
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                              */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
  BYTE pdrv,  /* Physical drive number (0..) */
  BYTE cmd,   /* Control code */
  void* buff  /* Buffer to send/receive control data */
)
{
#if DEVICE_STORAGE == 1
  const Device::Storage::StorageStatus* status = Device::Storage::Status();

  switch (cmd)
  {
    case 0: // CTRL_SYNC
      return RES_OK; // Always successful for SDMMC

    case 1: // GET_SECTOR_COUNT
      if (status->available && buff)
      {
        *(uint32_t*)buff = status->sector_count;
        return RES_OK;
      }
      return RES_ERROR;

    case 2: // GET_SECTOR_SIZE
      if (buff)
      {
        *(uint16_t*)buff = status->sector_size;
        return RES_OK;
      }
      return RES_ERROR;

    case 3: // GET_BLOCK_SIZE
      if (status->available && buff)
      {
        *(uint32_t*)buff = status->block_size;
        return RES_OK;
      }
      return RES_ERROR;

    default:
      return RES_PARERR; // Unsupported command
  }
#else
  return RES_PARERR;
#endif
}

/*-----------------------------------------------------------------------*/
/* Get current time for FatFs                                           */
/*-----------------------------------------------------------------------*/

DWORD get_fattime (void)
{
  return 0;
}

} // extern "C"