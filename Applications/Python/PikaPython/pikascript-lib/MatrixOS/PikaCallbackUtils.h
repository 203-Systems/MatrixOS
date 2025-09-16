#pragma once

#include "PikaObj.h"

inline PikaEventListener* g_pika_user_listener = nullptr;
inline uint32_t callbackRegistered = 0;

uint32_t RegisterCallback(Arg* cb)
{
    if(NULL == cb)
    {
        return UINT32_MAX;
    }

    if (NULL == g_pika_user_listener){
        pika_eventListener_init(&g_pika_user_listener);
    }

    uint32_t eventId = callbackRegistered;
    callbackRegistered++;

    pika_eventListener_registEventCallback(g_pika_user_listener, eventId, cb);

    return eventId;
}

Arg* EventCallback(uint32_t eventId, int eventSignal = 0)
{
    if(eventId == UINT32_MAX)
    {
        return nullptr;
    }

    return pks_eventListener_sendSignalAwaitResult(g_pika_user_listener, eventId, eventSignal);
}