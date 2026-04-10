#pragma once

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TimerCallbackFunction_t)(TimerHandle_t timer);

TimerHandle_t xTimerCreateStatic(const char* name, TickType_t period, BaseType_t autoReload, void* timerId,
                                 TimerCallbackFunction_t callback, StaticTimer_t* buffer);
BaseType_t xTimerStart(TimerHandle_t timer, TickType_t ticksToWait);
BaseType_t xTimerStop(TimerHandle_t timer, TickType_t ticksToWait);
void* pvTimerGetTimerID(TimerHandle_t timer);

#ifdef __cplusplus
}
#endif
