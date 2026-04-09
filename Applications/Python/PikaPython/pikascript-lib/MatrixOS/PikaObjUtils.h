#pragma once

#include "PikaObj.h"
#include <type_traits>

// ============================================================================
// IsApprovedPikaValue<T> — compile-time allowlist for value wrappers.
//
// Only types explicitly approved here may be stored by-value inside PikaObj.
// To approve a new type:
//   1. Verify it is trivially copyable (no strings, containers, vtables)
//   2. Add a forward declaration below
//   3. Add a template specialization to the approved list
//   4. The static_assert enforces both traits at template instantiation
//
// For file-local types, add the specialization in the translation unit
// that defines the struct, before first use.
// ============================================================================

template <typename T>
struct IsApprovedPikaValue : std::false_type {};

// Forward declarations for globally-approved value types.
// Full definitions come from MatrixOS.h, included by all .cpp translation units.
struct Point;
struct Dimension;
struct Color;
struct InputId;
struct KeypadInfo;
struct MidiPacket;
struct InputEvent;
struct InputSnapshot;

template <> struct IsApprovedPikaValue<Point> : std::true_type {};
template <> struct IsApprovedPikaValue<Dimension> : std::true_type {};
template <> struct IsApprovedPikaValue<Color> : std::true_type {};
template <> struct IsApprovedPikaValue<InputId> : std::true_type {};
template <> struct IsApprovedPikaValue<KeypadInfo> : std::true_type {};
template <> struct IsApprovedPikaValue<MidiPacket> : std::true_type {};
template <> struct IsApprovedPikaValue<InputEvent> : std::true_type {};
template <> struct IsApprovedPikaValue<InputSnapshot> : std::true_type {};

// ============================================================================
// Value wrappers — for small, trivially-copyable, approved types only.
//
// These store a by-value copy inside PikaObj via obj_setStruct (memcpy).
// Do NOT use for types with strings, containers, function objects,
// unique_ptr, virtual methods, or non-trivial destructors.
// ============================================================================

template <typename T, typename... Args>
void createCppValueInPikaObj(PikaObj* pika_obj, Args&&... args)
{
    static_assert(IsApprovedPikaValue<T>::value,
        "createCppValueInPikaObj: T is not on the approved value-wrapper list. "
        "Add an IsApprovedPikaValue specialization or use handle wrappers.");
    static_assert(std::is_trivially_copyable_v<T>,
        "createCppValueInPikaObj: T must be trivially copyable.");
    T obj = T(std::forward<Args>(args)...);
    obj_setStruct(pika_obj, (char*)"_self", obj);
}

template <typename T>
void copyCppValueIntoPikaObj(PikaObj* pika_obj, const T& obj)
{
    static_assert(IsApprovedPikaValue<T>::value,
        "copyCppValueIntoPikaObj: T is not on the approved value-wrapper list.");
    static_assert(std::is_trivially_copyable_v<T>,
        "copyCppValueIntoPikaObj: T must be trivially copyable.");
    T copy = obj;  // mutable copy — obj_setStruct takes void*, not const void*
    obj_setStruct(pika_obj, (char*)"_self", copy);
}

template <typename T>
T* getCppValuePtrInPikaObj(PikaObj* pika_obj) {
    static_assert(IsApprovedPikaValue<T>::value,
        "getCppValuePtrInPikaObj: T is not on the approved value-wrapper list.");
    static_assert(std::is_trivially_copyable_v<T>,
        "getCppValuePtrInPikaObj: T must be trivially copyable.");
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