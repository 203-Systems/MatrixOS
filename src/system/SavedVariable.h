#pragma once
#include "framework/Framework.h"
#include "UserVariables.h"

struct SavedVar
{
    string name;
    uint8_t length;
    void* pointer;

    template<typename T>
    SavedVar(string name, T* pointer)
    {
        this->name = name;
        this->pointer = pointer;
        this->length = sizeof(*pointer);
    }
    
    SavedVar(string name, string* pointer)
    {
        this->name = name;
        this->pointer = pointer;
        this->length = (*pointer).length();
    }
};

namespace MatrixOS::UserVar
{   
    const inline SavedVar savedVar[] =
    {
        SavedVar("Brightness", &MatrixOS::UserVar::brightness),
        SavedVar("Rotation", &MatrixOS::UserVar::rotation)
        // SavedVar("Test", &str)
    };
}