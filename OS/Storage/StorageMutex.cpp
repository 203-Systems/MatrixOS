#include "StorageMutex.h"
#include "MatrixOS.h"

namespace MatrixOS::Storage
{
  SemaphoreHandle_t storageMutex = NULL;

  void InitMutex()
  {
    if (storageMutex == NULL)
    {
      storageMutex = xSemaphoreCreateMutex();
      if (storageMutex == NULL)
      {
        MatrixOS::Logging::LogError("Storage", "Failed to create storage mutex!");
      }
      else
      {
        MatrixOS::Logging::LogInfo("Storage", "Storage mutex initialized");
      }
    }
  }

  Lock::Lock(uint32_t timeout_ms)
  {
    locked = false;
    if (storageMutex != NULL)
    {
      TickType_t timeout = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
      if (xSemaphoreTake(storageMutex, timeout) == pdTRUE)
      {
        locked = true;
      }
      else
      {
        MatrixOS::Logging::LogWarning("Storage", "Failed to acquire storage mutex within timeout");
      }
    }
  }

  Lock::~Lock()
  {
    if (locked && storageMutex != NULL)
    {
      xSemaphoreGive(storageMutex);
    }
  }
}