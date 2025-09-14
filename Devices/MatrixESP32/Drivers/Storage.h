#pragma once

#include "MatrixOS.h"

namespace Device
{
  namespace Storage
  {
    // Initialize Storage subsystem
    void Init();

    // Get current status (returns pointer to cached status)
    const Status* GetStatus();

    // Direct sector access
    bool ReadSectors(uint8_t pdrv, uint32_t lba, uint8_t* buffer, uint32_t count);
    bool WriteSectors(uint8_t pdrv, uint32_t lba, const uint8_t* buffer, uint32_t count);
  }
}