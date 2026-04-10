#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;
typedef uint32_t StackType_t;

typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
typedef void* SemaphoreHandle_t;
typedef void* TimerHandle_t;
typedef void* MessageBufferHandle_t;

typedef void (*TaskFunction_t)(void*);

typedef struct StaticTask_t { void* reserved; } StaticTask_t;
typedef struct StaticTimer_t { void* reserved; } StaticTimer_t;

typedef enum {
  taskSCHEDULER_NOT_STARTED = 0,
  taskSCHEDULER_RUNNING = 1,
  taskSCHEDULER_SUSPENDED = 2
} eTaskSchedulerState;

typedef enum {
  eRunning = 0,
  eReady,
  eBlocked,
  eSuspended,
  eDeleted,
  eInvalid
} eTaskState;

#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define pdFAIL 0

#define configTICK_RATE_HZ 1000
#define portTICK_PERIOD_MS (1000 / configTICK_RATE_HZ)
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / portTICK_PERIOD_MS))

#define tskIDLE_PRIORITY 0
#define configMAX_PRIORITIES 8
#define configMINIMAL_STACK_SIZE 128
#define configTOTAL_HEAP_SIZE (1024 * 1024)
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5

#define portMAX_DELAY 0xffffffffu

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

#ifndef DRAM_ATTR
#define DRAM_ATTR
#endif

void* pvPortMalloc(size_t size);
void vPortFree(void* ptr);
size_t xPortGetFreeHeapSize(void);
size_t xPortGetMinimumEverFreeHeapSize(void);

#ifdef __cplusplus
}
#endif
