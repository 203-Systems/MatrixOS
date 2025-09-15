#pragma once

#include "FreeRTOS.h"
#include "semphr.h"

namespace MatrixOS::Storage
{
  // Shared mutex for synchronizing storage access between File operations and USB MSC
  extern SemaphoreHandle_t storageMutex;

  // Initialize the storage mutex (call once during system init)
  void InitMutex();

  // Helper class for RAII-style mutex locking
  class Lock
  {
  public:
    Lock(uint32_t timeout_ms = portMAX_DELAY);
    ~Lock();

    bool IsLocked() const { return locked; }

  private:
    bool locked;
  };
}