#pragma once

#include "PikaObj.h"

template <typename T>
PikaObj* createPikaObj(const T& obj) {
    // Allocate managed object
    auto ptr = std::make_shared<T>(obj);

    // Store the shared_ptr itself inside PikaObj
    PikaObj* new_obj = newNormalObj(New_PikaObj);
    obj_setPtr(new_obj, (char*)"cpp_obj_ptr", new std::shared_ptr<T>(ptr));

    return new_obj;
}

template <typename T>
bool loadIntoPikaObj(PikaObj* pikaObj, const T& obj) {
    // Allocate managed object
    auto ptr = std::make_shared<T>(obj);

    // Store the shared_ptr itself inside PikaObj
    obj_setPtr(pikaObj, (char*)"cpp_obj_ptr", new std::shared_ptr<T>(ptr));

    return true;
}

template <typename T>
T& getCppObj(PikaObj* pika_obj) {
    auto sptr = static_cast<std::shared_ptr<T>*>(obj_getPtr(pika_obj, (char*)"cpp_obj_ptr"));
    if (!sptr || !(*sptr)) {
        throw std::runtime_error("No valid C++ object stored in PikaObj");
    }
    return **sptr;
}