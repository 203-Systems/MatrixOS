#pragma once

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

TaskHandle_t xTaskCreateStatic(TaskFunction_t task, const char* name, uint32_t stackDepth, void* params,
                               UBaseType_t priority, StackType_t* stackBuffer, StaticTask_t* taskBuffer);
BaseType_t xTaskCreate(TaskFunction_t task, const char* name, uint16_t stackDepth, void* params, UBaseType_t priority,
                       TaskHandle_t* taskHandle);
void vTaskDelete(TaskHandle_t task);
void vTaskDelay(TickType_t ticks);
void taskYIELD(void);
TickType_t xTaskGetTickCount(void);
eTaskSchedulerState xTaskGetSchedulerState(void);
void vTaskStartScheduler(void);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
void vTaskSuspend(TaskHandle_t task);
void vTaskResume(TaskHandle_t task);
void vTaskSuspendAll(void);
BaseType_t xTaskResumeAll(void);
eTaskState eTaskGetState(TaskHandle_t task);
uint32_t ulTaskNotifyTake(BaseType_t clearOnExit, TickType_t ticksToWait);
void xTaskNotifyGive(TaskHandle_t task);
void* pvTaskGetThreadLocalStoragePointer(TaskHandle_t task, BaseType_t index);
void vTaskSetThreadLocalStoragePointer(TaskHandle_t task, BaseType_t index, void* value);

#ifdef __cplusplus
}
#endif
