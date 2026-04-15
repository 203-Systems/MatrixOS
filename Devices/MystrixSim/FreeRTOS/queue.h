#pragma once

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t itemSize);
BaseType_t xQueueSend(QueueHandle_t queue, const void* item, TickType_t ticksToWait);
BaseType_t xQueueReceive(QueueHandle_t queue, void* buffer, TickType_t ticksToWait);
BaseType_t xQueueReset(QueueHandle_t queue);
UBaseType_t uxQueueSpacesAvailable(QueueHandle_t queue);
void vQueueDelete(QueueHandle_t queue);

#ifdef __cplusplus
}
#endif
