#pragma once

#include "PikaObj.h"

template <typename T, typename... Args>
void createCppObjPtrInPikaObj(PikaObj* pika_obj, Args&&... args)
{
    T obj = T(std::forward<Args>(args)...);
    obj_setStruct(pika_obj, (char*)"_self", obj);
}

template <typename T>
void copyCppObjIntoPikaObj(PikaObj* pika_obj, T& obj)
{
    obj_setStruct(pika_obj, (char*)"_self", obj);
}

template <typename T>
T* getCppObjPtrInPikaObj(PikaObj* pika_obj) {
    T* ptr = (T*)(obj_getStruct(pika_obj, (char*)"_self"));
    return ptr;
}

template <typename T>
T& getCppObjInPikaObj(PikaObj* pika_obj) {
    T* ptr = (T*)(obj_getStruct(pika_obj, (char*)"_self"));
    return *ptr;
}

inline bool hasCppObjInPikaObj(PikaObj* pika_obj) {
    return (obj_getStruct(pika_obj, (char*)"_self")) != nullptr;
}