#pragma once

#include "PikaObj.h"

// ============================================================================
// Callback context — safety layer for native-to-Python callback invocation.
//
// Each handle wrapper creates a PythonCallbackContext during construction.
// Native lambdas capture ctx* instead of raw PikaObj*.
// Close()/destroy invalidates the context so callbacks become no-ops.
//
// This eliminates the dangling PikaObj* risk: lambdas never dereference
// the PikaObj pointer without first checking the alive flag on the context.
// The context itself remains valid for the lifetime of all lambdas because
// Close() destroys the native object (and thus all lambdas) before freeing
// the context.
// ============================================================================

struct PythonCallbackContext {
  PikaObj* owner;
  bool alive;
};

// Create a callback context for a handle wrapper.
// Call this in the constructor after creating the native handle.
// Pins the wrapper object via refcount so it cannot be GC'd while
// native callbacks may still fire.
inline PythonCallbackContext* InitCallbackContext(PikaObj* self)
{
    PythonCallbackContext* ctx = new PythonCallbackContext{self, true};
    obj_setPtr(self, (char*)"_cbCtx", (void*)ctx);
    obj_refcntInc(self);
    return ctx;
}

// Retrieve the callback context for a handle wrapper.
// Returns nullptr if no context exists.
inline PythonCallbackContext* GetCallbackContext(PikaObj* self)
{
    return (PythonCallbackContext*)obj_getPtr(self, (char*)"_cbCtx");
}

// Mark the callback context as dead.
// All future callback invocations through this context will no-op.
// Call this BEFORE destroying the native handle.
inline void InvalidateCallbackContext(PikaObj* self)
{
    PythonCallbackContext* ctx = GetCallbackContext(self);
    if (ctx)
    {
        ctx->alive = false;
    }
}

// Destroy the callback context and free its memory.
// Call this AFTER destroying the native handle (which destroys all
// std::function lambdas that captured this context pointer).
// Releases the refcount pin established by InitCallbackContext.
inline void DestroyCallbackContext(PikaObj* self)
{
    PythonCallbackContext* ctx = GetCallbackContext(self);
    if (ctx)
    {
        ctx->alive = false;
        PikaObj* pinned = ctx->owner;
        delete ctx;
        obj_setPtr(self, (char*)"_cbCtx", nullptr);
        if (pinned) {
            obj_refcntDec(pinned);
        }
    }
}

// ============================================================================
// Callback storage helpers
//
// All callbacks are stored as named Arg entries inside the PikaObj wrapper.
// ============================================================================

inline bool SaveCallbackObjToPikaObj(PikaObj *self, char* name, Arg* callback)
{
    Arg* existing_callback = obj_getArg(self, name);
    if (existing_callback) {
        arg_deinit(existing_callback);
    }

    if(obj_setArg(self, name, callback) != PIKA_RES_OK)
    {
        return true;
    }

    return false;
}

// Remove a single named callback from the wrapper.
// Safe to call if the callback does not exist.
inline void ClearCallbackInPikaObj(PikaObj *self, char* name)
{
    obj_removeArg(self, name);
}

// ============================================================================
// Safe callback invocation helpers
//
// These check the callback context alive flag before touching PikaObj.
// Use these from native lambdas that captured PythonCallbackContext*.
// ============================================================================

inline Arg* SafeCallCallback0(PythonCallbackContext* ctx, char* name)
{
    if (!ctx || !ctx->alive) return nullptr;
    Arg* callback = obj_getArg(ctx->owner, name);
    if (!callback) return nullptr;

    Arg* callback_copy = arg_copy(callback);
    return obj_runMethodArg0(ctx->owner, callback_copy);
}

inline Arg* SafeCallCallback1(PythonCallbackContext* ctx, char* name, Arg* arg1)
{
    if (!ctx || !ctx->alive) return nullptr;
    Arg* callback = obj_getArg(ctx->owner, name);
    if (!callback) return nullptr;

    Arg* callback_copy = arg_copy(callback);
    return obj_runMethodArg1(ctx->owner, callback_copy, arg1);
}

inline Arg* SafeCallCallback2(PythonCallbackContext* ctx, char* name, Arg* arg1, Arg* arg2)
{
    if (!ctx || !ctx->alive) return nullptr;
    Arg* callback = obj_getArg(ctx->owner, name);
    if (!callback) return nullptr;

    Arg* callback_copy = arg_copy(callback);
    return obj_runMethodArg2(ctx->owner, callback_copy, arg1, arg2);
}

