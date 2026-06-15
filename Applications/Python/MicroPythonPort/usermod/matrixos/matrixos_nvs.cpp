// Exposes MatrixOS NVS typed, bytes, and string storage helpers to MicroPython.
#include "matrixos_common.h"
#include "matrixos_modules.h"

#include <cstring>

extern "C" {
#include "py/runtime.h"
}

using namespace MatrixOSPython;

mp_obj_t nvs_get(size_t argc, const mp_obj_t* args) {
  vector<char> value = MatrixOS::NVS::GetVariable(ObjectToHash(args[0]));
  if (value.empty())
  {
    return argc > 1 ? args[1] : mp_const_none;
  }

  if (argc <= 1 || args[1] == mp_const_none)
  {
    return mp_obj_new_bytes((const byte*)value.data(), value.size());
  }

  mp_obj_t defaultObj = args[1];
  if (mp_obj_is_bool(defaultObj))
  {
    return mp_obj_new_bool(value[0] != 0);
  }

  if (mp_obj_is_int(defaultObj))
  {
    uint32_t number = 0;
    size_t copyLength = value.size() < sizeof(number) ? value.size() : sizeof(number);
    memcpy(&number, value.data(), copyLength);
    return mp_obj_new_int_from_uint(number);
  }

  if (mp_obj_is_str(defaultObj))
  {
    return mp_obj_new_str(value.data(), value.size());
  }

  if (mp_obj_is_type(defaultObj, &mp_type_bytes) || mp_obj_is_type(defaultObj, &mp_type_bytearray))
  {
    return mp_obj_new_bytes((const byte*)value.data(), value.size());
  }

  mp_raise_TypeError(MP_ERROR_TEXT("default must be None, bool, int, str, or bytes"));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(nvs_get_obj, 1, 2, nvs_get);

mp_obj_t nvs_set(mp_obj_t keyObj, mp_obj_t valueObj) {
  uint32_t key = ObjectToHash(keyObj);

  if (mp_obj_is_bool(valueObj))
  {
    uint8_t value = valueObj == mp_const_true ? 1 : 0;
    return mp_obj_new_bool(MatrixOS::NVS::SetVariable(key, &value, sizeof(value)));
  }

  if (mp_obj_is_int(valueObj))
  {
    mp_int_t number = mp_obj_get_int(valueObj);
    if (number < 0)
    {
      mp_raise_ValueError(MP_ERROR_TEXT("NVS integers must be unsigned"));
    }

    if (number <= UINT8_MAX)
    {
      uint8_t value = (uint8_t)number;
      return mp_obj_new_bool(MatrixOS::NVS::SetVariable(key, &value, sizeof(value)));
    }

    if (number <= UINT16_MAX)
    {
      uint16_t value = (uint16_t)number;
      return mp_obj_new_bool(MatrixOS::NVS::SetVariable(key, &value, sizeof(value)));
    }

    uint32_t value = (uint32_t)number;
    return mp_obj_new_bool(MatrixOS::NVS::SetVariable(key, &value, sizeof(value)));
  }

  if (mp_obj_is_str(valueObj))
  {
    size_t length = 0;
    const char* value = mp_obj_str_get_data(valueObj, &length);
    return mp_obj_new_bool(MatrixOS::NVS::SetVariable(key, (void*)value, length));
  }

  mp_buffer_info_t buffer;
  mp_get_buffer_raise(valueObj, &buffer, MP_BUFFER_READ);
  return mp_obj_new_bool(MatrixOS::NVS::SetVariable(key, buffer.buf, buffer.len));
}
MP_DEFINE_CONST_FUN_OBJ_2(nvs_set_obj, nvs_set);

mp_obj_t nvs_get_u8(mp_obj_t hashObj, mp_obj_t defaultObj) {
  uint8_t value = (uint8_t)mp_obj_get_int_truncated(defaultObj);
  MatrixOS::NVS::GetVariable(ObjectToHash(hashObj), &value, sizeof(value));
  return mp_obj_new_int(value);
}
MP_DEFINE_CONST_FUN_OBJ_2(nvs_get_u8_obj, nvs_get_u8);

mp_obj_t nvs_set_u8(mp_obj_t hashObj, mp_obj_t valueObj) {
  uint8_t value = (uint8_t)mp_obj_get_int_truncated(valueObj);
  return mp_obj_new_bool(MatrixOS::NVS::SetVariable(ObjectToHash(hashObj), &value, sizeof(value)));
}
MP_DEFINE_CONST_FUN_OBJ_2(nvs_set_u8_obj, nvs_set_u8);

mp_obj_t nvs_get_u16(mp_obj_t hashObj, mp_obj_t defaultObj) {
  uint16_t value = (uint16_t)mp_obj_get_int_truncated(defaultObj);
  MatrixOS::NVS::GetVariable(ObjectToHash(hashObj), &value, sizeof(value));
  return mp_obj_new_int(value);
}
MP_DEFINE_CONST_FUN_OBJ_2(nvs_get_u16_obj, nvs_get_u16);

mp_obj_t nvs_set_u16(mp_obj_t hashObj, mp_obj_t valueObj) {
  uint16_t value = (uint16_t)mp_obj_get_int_truncated(valueObj);
  return mp_obj_new_bool(MatrixOS::NVS::SetVariable(ObjectToHash(hashObj), &value, sizeof(value)));
}
MP_DEFINE_CONST_FUN_OBJ_2(nvs_set_u16_obj, nvs_set_u16);

mp_obj_t nvs_get_u32(mp_obj_t hashObj, mp_obj_t defaultObj) {
  uint32_t value = (uint32_t)mp_obj_get_int_truncated(defaultObj);
  MatrixOS::NVS::GetVariable(ObjectToHash(hashObj), &value, sizeof(value));
  return mp_obj_new_int_from_uint(value);
}
MP_DEFINE_CONST_FUN_OBJ_2(nvs_get_u32_obj, nvs_get_u32);

mp_obj_t nvs_set_u32(mp_obj_t hashObj, mp_obj_t valueObj) {
  uint32_t value = (uint32_t)mp_obj_get_int_truncated(valueObj);
  return mp_obj_new_bool(MatrixOS::NVS::SetVariable(ObjectToHash(hashObj), &value, sizeof(value)));
}
MP_DEFINE_CONST_FUN_OBJ_2(nvs_set_u32_obj, nvs_set_u32);

mp_obj_t nvs_get_size(mp_obj_t hashObj) {
  return mp_obj_new_int(MatrixOS::NVS::GetSize(ObjectToHash(hashObj)));
}
MP_DEFINE_CONST_FUN_OBJ_1(nvs_get_size_obj, nvs_get_size);

mp_obj_t nvs_delete(mp_obj_t hashObj) {
  return mp_obj_new_bool(MatrixOS::NVS::DeleteVariable(ObjectToHash(hashObj)));
}
MP_DEFINE_CONST_FUN_OBJ_1(nvs_delete_obj, nvs_delete);

mp_obj_t nvs_get_bytes(size_t argc, const mp_obj_t* args) {
  vector<char> value = MatrixOS::NVS::GetVariable(ObjectToHash(args[0]));
  if (value.empty())
  {
    return argc > 1 ? args[1] : mp_const_none;
  }
  return mp_obj_new_bytes((const byte*)value.data(), value.size());
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(nvs_get_bytes_obj, 1, 2, nvs_get_bytes);

mp_obj_t nvs_set_bytes(mp_obj_t hashObj, mp_obj_t dataObj) {
  mp_buffer_info_t buffer;
  mp_get_buffer_raise(dataObj, &buffer, MP_BUFFER_READ);
  return mp_obj_new_bool(MatrixOS::NVS::SetVariable(ObjectToHash(hashObj), buffer.buf, buffer.len));
}
MP_DEFINE_CONST_FUN_OBJ_2(nvs_set_bytes_obj, nvs_set_bytes);

mp_obj_t nvs_get_string(size_t argc, const mp_obj_t* args) {
  vector<char> value = MatrixOS::NVS::GetVariable(ObjectToHash(args[0]));
  if (value.empty())
  {
    return argc > 1 ? args[1] : mp_obj_new_str("", 0);
  }
  return mp_obj_new_str(value.data(), value.size());
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(nvs_get_string_obj, 1, 2, nvs_get_string);

mp_obj_t nvs_set_string(mp_obj_t hashObj, mp_obj_t valueObj) {
  size_t length = 0;
  const char* value = mp_obj_str_get_data(valueObj, &length);
  return mp_obj_new_bool(MatrixOS::NVS::SetVariable(ObjectToHash(hashObj), (void*)value, length));
}
MP_DEFINE_CONST_FUN_OBJ_2(nvs_set_string_obj, nvs_set_string);

static const mp_rom_map_elem_t nvs_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&nvs_get_obj)},
    {MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&nvs_set_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_u8), MP_ROM_PTR(&nvs_get_u8_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_u8), MP_ROM_PTR(&nvs_set_u8_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_u16), MP_ROM_PTR(&nvs_get_u16_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_u16), MP_ROM_PTR(&nvs_set_u16_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_u32), MP_ROM_PTR(&nvs_get_u32_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_u32), MP_ROM_PTR(&nvs_set_u32_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_size), MP_ROM_PTR(&nvs_get_size_obj)},
    {MP_ROM_QSTR(MP_QSTR_delete), MP_ROM_PTR(&nvs_delete_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_bytes), MP_ROM_PTR(&nvs_get_bytes_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_bytes), MP_ROM_PTR(&nvs_set_bytes_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_string), MP_ROM_PTR(&nvs_get_string_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_string), MP_ROM_PTR(&nvs_set_string_obj)},
};
MP_DEFINE_CONST_DICT(nvs_globals, nvs_globals_table);

extern const mp_obj_module_t matrixos_nvs_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&nvs_globals,
};
