#pragma once
#include "Hash.h"

namespace MatrixOS::NVS
{
    int8_t GetVariable(uint32_t hash, void* pointer, uint16_t length); 
    bool SetVariable(uint32_t hash, void* pointer, uint16_t length);
    bool DeleteVariable(uint32_t hash);
}

#define SavedVar(scope, name, default_value) SavedVariable(StaticHash(scope "-" name), default_value) //This way the class string and name string are not gonna be compiled in

template <class T>
class SavedVariable
{
    public:
    uint32_t hash;
    bool loaded = false;
    T value;

    SavedVariable(string scope, string name, T default_value) //Scope is basiclly namespace for the variable. I can't use "namespace" or "class" as variable name but you get the point
    {
        uint32_t hash = Hash(scope + "-" + name);
        SavedVariable(hash, default_value);
    }

    SavedVariable(uint32_t hash, T default_value)
    {
        this->hash = hash;
        this->loaded = Load();
    }

    bool Load()
    {
        return MatrixOS::NVS::GetVariable(hash, &value, sizeof(T)) == 0;
    }

    bool Loaded()
    {
        return loaded;
    }

    bool Set(T new_value)
    {
        if(MatrixOS::NVS::SetVariable(hash, &new_value, sizeof(T)))
        {
            value = new_value;
            return true;
        }
        return false;
    }

    T Get()
    {
        return value;
    }

    bool Delete()
    {
        if(MatrixOS::NVS::DeleteVariable(hash))
        {
            loaded = false;
            return true;
        }
        return false;
    }

    bool operator = (T new_value) {return Set(new_value);}

    operator T() {return Get();}
};