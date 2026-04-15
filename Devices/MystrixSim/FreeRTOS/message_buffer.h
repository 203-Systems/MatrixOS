#pragma once

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

MessageBufferHandle_t xMessageBufferCreate(size_t size);
size_t xMessageBufferSendFromISR(MessageBufferHandle_t buffer, const void* data, size_t size, BaseType_t* taskWoken);
size_t xMessageBufferReceive(MessageBufferHandle_t buffer, void* outBuffer, size_t bufferSize, TickType_t ticksToWait);
BaseType_t xMessageBufferReset(MessageBufferHandle_t buffer);

#ifdef __cplusplus
}
#endif
