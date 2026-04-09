#pragma once

#include "PikaObj.h"

// ============================================================================
// Value wrappers — for small, trivially-copyable types only.
//
// Approved value types: Point, Dimension, Color, InputId, KeypadInfo,
//                       PythonKeyEvent, PythonInputEventValue
//
// These store a by-value copy inside PikaObj via obj_setStruct (memcpy).
// Do NOT use for types with strings, containers, function objects,
// unique_ptr, virtual methods, or non-trivial destructors.
// ============================================================================

template <typename T, typename... Args>
void createCppValueInPikaObj(PikaObj* pika_obj, Args&&... args)
{
    T obj = T(std::forward<Args>(args)...);
    obj_setStruct(pika_obj, (char*)"_self", obj);
}

template <typename T>
void copyCppValueIntoPikaObj(PikaObj* pika_obj, T& obj)
{
    obj_setStruct(pika_obj, (char*)"_self", obj);
}

template <typename T>
T* getCppValuePtrInPikaObj(PikaObj* pika_obj) {
    return (T*)(obj_getStruct(pika_obj, (char*)"_self"));
}

inline bool hasCppValueInPikaObj(PikaObj* pika_obj) {
    return (obj_getStruct(pika_obj, (char*)"_self")) != nullptr;
}

// ============================================================================
// Handle wrappers — for non-trivial C++ objects.
//
// Required handle types: UI, UIComponent, UIButton, UISelector, UI4pxNumber
//
// The native object is heap-allocated with new.
// PikaObj stores only a pointer to a NativeHandle holder.
// The holder tracks an alive flag to prevent use-after-destroy.
// Explicit destroyCppHandleInPikaObj<T>() must be called on teardown.
//
// The handle stores a type-erased void* so that base-class retrieval
// (e.g. getCppHandlePtrInPikaObj<UIComponent> on a UIButton wrapper)
// works safely for single-inheritance hierarchies.
// ============================================================================

struct NativeHandle {
  void* ptr;
  bool alive;
};

template <typename T, typename... Args>
T* createCppHandleInPikaObj(PikaObj* pika_obj, Args&&... args)
{
    T* native = new T(std::forward<Args>(args)...);
    NativeHandle* handle = new NativeHandle{static_cast<void*>(native), true};
    obj_setPtr(pika_obj, (char*)"_handle", (void*)handle);
    return native;
}

template <typename T>
T* getCppHandlePtrInPikaObj(PikaObj* pika_obj)
{
    NativeHandle* handle = (NativeHandle*)obj_getPtr(pika_obj, (char*)"_handle");
    if (!handle || !handle->alive) return nullptr;
    return static_cast<T*>(handle->ptr);
}

inline bool hasCppHandleInPikaObj(PikaObj* pika_obj)
{
    NativeHandle* handle = (NativeHandle*)obj_getPtr(pika_obj, (char*)"_handle");
    return handle && handle->alive;
}

template <typename T>
void destroyCppHandleInPikaObj(PikaObj* pika_obj)
{
    NativeHandle* handle = (NativeHandle*)obj_getPtr(pika_obj, (char*)"_handle");
    if (!handle) return;
    if (handle->alive) {
        delete static_cast<T*>(handle->ptr);
        handle->ptr = nullptr;
        handle->alive = false;
    }
    delete handle;
    obj_setPtr(pika_obj, (char*)"_handle", nullptr);
}

// ============================================================================
// Legacy aliases — kept temporarily so unchanged value-wrapper callers
// (Point, Dimension, Color, KeyEvent, etc.) continue to compile.
// These will be removed after all callers are migrated to the explicit
// value/handle APIs above.
// ============================================================================

template <typename T, typename... Args>
void createCppObjPtrInPikaObj(PikaObj* pika_obj, Args&&... args)
{
    createCppValueInPikaObj<T>(pika_obj, std::forward<Args>(args)...);
}

template <typename T>
void copyCppObjIntoPikaObj(PikaObj* pika_obj, T& obj)
{
    copyCppValueIntoPikaObj<T>(pika_obj, obj);
}

template <typename T>
T* getCppObjPtrInPikaObj(PikaObj* pika_obj) {
    return getCppValuePtrInPikaObj<T>(pika_obj);
}

template <typename T>
T& getCppObjInPikaObj(PikaObj* pika_obj) {
    T* ptr = getCppValuePtrInPikaObj<T>(pika_obj);
    return *ptr;
}

inline bool hasCppObjInPikaObj(PikaObj* pika_obj) {
    return hasCppValueInPikaObj(pika_obj);
}