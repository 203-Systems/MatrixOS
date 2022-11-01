#pragma once
#include "Hash.h"

namespace MatrixOS::NVS
{
  int8_t GetVariable(uint32_t hash, void* pointer, uint16_t length);
  bool SetVariable(uint32_t hash, void* pointer, uint16_t length);
  bool DeleteVariable(uint32_t hash);
}

enum SavedVariableState { NotInited, Inited, Loaded, Deleted };

#define CreateSavedVar(scope, name, type, default_value) \
  SavedVariable<type> name = SavedVariable(StaticHash(scope "-" #name), (type)default_value)
template <class T>
class SavedVariable {
 public:
  uint32_t hash;
  SavedVariableState state = SavedVariableState::NotInited;
  T value;

  SavedVariable(string scope, string name, T default_value)  // Scope is basiclly namespace for the variable. I can't
                                                             // use "namespace" or "class" as variable name but you get
                                                             // the point
  {
    this->hash = Hash(scope + "-" + name);
    this->value = default_value;
    this->state = SavedVariableState::Inited;
  }

  SavedVariable(uint32_t hash, T default_value) {
    this->hash = hash;
    this->value = default_value;
    this->state = SavedVariableState::Inited;
  }

  bool Load() {
    if (MatrixOS::NVS::GetVariable(hash, &value, sizeof(T)) == 0)
    {
      state = SavedVariableState::Loaded;
      return true;
    }
    return false;
  }

  bool Loaded() { return state == SavedVariableState::Loaded; }

  bool Set(T new_value) {
    if (MatrixOS::NVS::SetVariable(hash, &new_value, sizeof(T)))
    {
      value = new_value;
      state = SavedVariableState::Loaded;
      return true;
    }
    return false;
  }

  bool TempSet(T new_value)  // Update the variable but do not save it.
  {
    value = new_value;
    state = SavedVariableState::Loaded;
    return true;
  }

  T Get() {
    if (!Loaded())  // If not yet loaded, it will try to update current cache with NVS data
    { Load(); }
    // Even if it didn't load, the default value will be used.
    return value;
  }

  bool Delete() {
    if (MatrixOS::NVS::DeleteVariable(hash))
    {
      this->state = SavedVariableState::Deleted;
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

  T& operator+=(T operation_value) {
    Set(value + operation_value);
    return *value;
  }
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

  operator T() { return Get(); }
};