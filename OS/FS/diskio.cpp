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
#if DEVICE_FATFS == 1
  return (DSTATUS)Device::FatFS::Status(pdrv);
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
#if DEVICE_FATFS == 1
  return (DSTATUS)Device::FatFS::Init(pdrv);
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
#if DEVICE_FATFS == 1
  return (DRESULT)Device::FatFS::Read(pdrv, buff, sector, count);
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
#if DEVICE_FATFS == 1
  return (DRESULT)Device::FatFS::Write(pdrv, buff, sector, count);
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
#if DEVICE_FATFS == 1
  return (DRESULT)Device::FatFS::IOControl(pdrv, cmd, buff);
#else
  return RES_PARERR;
#endif
}

} // extern "C"