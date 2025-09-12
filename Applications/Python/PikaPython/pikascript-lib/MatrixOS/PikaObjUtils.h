#pragma once

#include "PikaObj.h"

template <typename T>
void deleteCppObjInPikaObj(PikaObj* pika_obj) {
    T* ptr = static_cast<T*>(obj_getPtr(pika_obj, (char*)"_obj_ptr"));
    if (ptr) {
        delete ptr;
        obj_setPtr(pika_obj, (char*)"_obj_ptr", nullptr);
    }
}

template <typename T>
T* createCppObjPtrInPikaObj(PikaObj* pika_obj)
{
    deleteCppObjInPikaObj<T>(pika_obj);

    // Allocate raw object with default constructor
    T* ptr = new T();
    obj_setPtr(pika_obj, (char*)"_obj_ptr", ptr);

    return ptr;
}

// Overload for objects that need constructor arguments
template <typename T, typename... Args>
T* createCppObjPtrInPikaObj(PikaObj* pika_obj, Args&&... args)
{
    deleteCppObjInPikaObj<T>(pika_obj);

    // Allocate raw object with constructor arguments
    T* ptr = new T(std::forward<Args>(args)...);
    obj_setPtr(pika_obj, (char*)"_obj_ptr", ptr);

    return ptr;
}

template <typename T>
T* copyCppObjIntoPikaObj(PikaObj* pika_obj, const T& obj)
{
    deleteCppObjInPikaObj<T>(pika_obj);
    
    // Create a copy of the object
    T* ptr = new T(obj);
    obj_setPtr(pika_obj, (char*)"_obj_ptr", ptr);
    
    return ptr;
}

// Convenience functions for safer pointer access
template <typename T>
T* getCppObjPtrInPikaObj(PikaObj* pika_obj) {
    T* ptr = static_cast<T*>(obj_getPtr(pika_obj, (char*)"_obj_ptr"));
    return ptr;
}


template <typename T>
T& getCppObjInPikaObj(PikaObj* pika_obj) {
    T* ptr = static_cast<T*>(obj_getPtr(pika_obj, (char*)"_obj_ptr"));
    return *ptr;
}

inline bool hasCppObjInPikaObj(PikaObj* pika_obj) {
    return (obj_getPtr(pika_obj, (char*)"_obj_ptr")) != nullptr;
}