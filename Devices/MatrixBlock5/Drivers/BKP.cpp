#include "Device.h"

// Undefine the BKP macro from STM32 HAL to avoid namespace conflict
#ifdef BKP
#define STM32_BKP_BASE BKP
#undef BKP
#endif

namespace Device::BackupReg
{
  uint16_t size = 20;

  uint32_t Read(uint32_t address) {
    RTC_HandleTypeDef RtcHandle;
    RtcHandle.Instance = RTC;
    return HAL_RTCEx_BKUPRead(&RtcHandle, address);
  }

  int8_t Write(uint32_t address, uint32_t data) {
    if (address >= size)
      return -1;
    RTC_HandleTypeDef RtcHandle;
    RtcHandle.Instance = RTC;
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&RtcHandle, address, data);
    HAL_PWR_DisableBkUpAccess();
    return 0;
  }
}

// Restore the BKP macro for STM32 HAL
#ifdef STM32_BKP_BASE
#define BKP STM32_BKP_BASE
#endif