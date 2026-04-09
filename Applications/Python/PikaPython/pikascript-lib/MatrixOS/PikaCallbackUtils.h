#pragma once

#include "PikaObj.h"

// ============================================================================
// Callback storage helpers
//
// All callbacks are stored as named Arg entries inside the PikaObj wrapper.
// Teardown helpers allow explicit cleanup when the native object is destroyed.
// ============================================================================

inline bool SaveCallbackObjToPikaObj(PikaObj *self, char* name, Arg* callback)
{
    // Check if callback already registered
    Arg* existing_callback = obj_getArg(self, name);
    if (existing_callback) {
        arg_deinit(existing_callback);
    }

    // Save callback
    if(obj_setArg(self, name, callback) != PIKA_RES_OK)
    {
        return true;
    }

    return false;
}

// Remove a single named callback from the wrapper.
// Safe to call if the callback does not exist.
// obj_removeArg handles arg deinitialization internally.
inline void ClearCallbackInPikaObj(PikaObj *self, char* name)
{
    obj_removeArg(self, name);
}

inline Arg* CallCallbackInPikaObj0(PikaObj *self, char* name)
{
    Arg* callback = obj_getArg(self, name);
    if (!callback) {
        return nullptr;
    }

    Arg* callback_copy = arg_copy(callback);
    return obj_runMethodArg0(self, callback_copy);
}

inline Arg* CallCallbackInPikaObj1(PikaObj *self, char* name, Arg* arg1)
{
    Arg* callback = obj_getArg(self, name);
    if (!callback) {
        return nullptr;
    }

    Arg* callback_copy = arg_copy(callback);
    return obj_runMethodArg1(self, callback_copy, arg1);
}

inline Arg* CallCallbackInPikaObj2(PikaObj *self, char* name, Arg* arg1, Arg* arg2)
{
    Arg* callback = obj_getArg(self, name);
    if (!callback) {
        return nullptr;
    }

    Arg* callback_copy = arg_copy(callback);
    return obj_runMethodArg2(self, callback_copy, arg1, arg2);
}