#pragma once
#include "Hash.h"

namespace MatrixOS::NVS
{
  int8_t GetVariable(uint32_t hash, void* pointer, uint16_t length);
  bool SetVariable(uint32_t hash, void* pointer, uint16_t length);
  bool DeleteVariable(uint32_t hash);
}

enum SavedVarState { NotInited, Inited, Loaded, Deleted };

#define CreateSavedVar(scope, name, type, default_value) \
  SavedVar<type> name = SavedVar(StaticHash(scope "-" #name), (type)default_value)
  
template <class T>
class SavedVar {
 public:
  uint32_t hash;
  SavedVarState state = SavedVarState::NotInited;
  T value;

  SavedVar(string scope, string name, T default_value)  // Scope is basically namespace for the variable. I can't
                                                             // use "namespace" or "class" as variable name but you get
                                                             // the point
  {
    this->hash = StringHash(scope + "-" + name);
    this->value = default_value;
    this->state = SavedVarState::Inited;
  }

  SavedVar(uint32_t hash, T default_value) {
    this->hash = hash;
    this->value = default_value;
    this->state = SavedVarState::Inited;
  }

  bool Load() {
    if (MatrixOS::NVS::GetVariable(hash, &value, sizeof(T)) == 0)
    {
      state = SavedVarState::Loaded;
      return true;
    }
    return false;
  }

  bool Loaded() { return state == SavedVarState::Loaded; }

  bool Set(T new_value) {
    if (MatrixOS::NVS::SetVariable(hash, &new_value, sizeof(T)))
    {
      value = new_value;
      state = SavedVarState::Loaded;
      return true;
    }
    return false;
  }

  bool TempSet(T new_value)  // Update the variable but do not save it.
  {
    value = new_value;
    state = SavedVarState::Loaded;
    return true;
  }

  bool Save() { return Set(value); }

  T& Get() {
    if (!Loaded())  // If not yet loaded, it will try to update current cache with NVS data
    { Load(); }
    // Even if it didn't load, the default value will be used.
    return value;
  }

  bool Delete() {
    if (MatrixOS::NVS::DeleteVariable(hash))
    {
      this->state = SavedVarState::Deleted;
      return true;
    }
    return false;
  }

  bool operator=(T new_value) { return Set(new_value); }

  bool operator==(T new_value) { return value == new_value; }
  bool operator!=(T new_value) { return value != new_value; }
  bool operator>(T new_value) { return value == new_value; }
  bool operator<(T new_value) { return value < new_value; }
  bool operator>=(T new_value) { return value >= new_value; }
  bool operator<=(T new_value) { return value <= new_value; }

  T operator+(T operation_value) { return value + operation_value; }
  T operator-(T operation_value) { return value - operation_value; }
  T operator*(T operation_value) { return value * operation_value; }
  T operator/(T operation_value) { return value / operation_value; }
  T operator%(T operation_value) { return value % operation_value; }

 
  T& operator-=(T operation_value) {
    Set(value - operation_value);
    return *value;
  }
  T& operator*=(T operation_value) {
    Set(value * operation_value);
    return *value;
  }
  T& operator/=(T operation_value) {
    Set(value / operation_value);
    return *value;
  }
  T& operator%=(T operation_value) {
    Set(value % operation_value);
    return *value;
  }

  T& operator++() {
    Set(value + 1);
    return *value;
  };
  T operator++(int) {
    T temp_value = value;
    Set(value + 1);
    return temp_value;
  };

  T& operator--() {
    Set(value + 1);
    return *value;
  };

  T operator--(int) {
    T temp_value = value;
    Set(value - 1);
    return temp_value;
  };

  T* operator&() { return &Get(); }

  operator T() { return Get(); }
  operator T*() { return &Get(); }
};