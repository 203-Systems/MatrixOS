// Exposes MatrixOS USB and CDC helpers to MicroPython.
#include "matrixos_common.h"

extern "C" {
#include "py/obj.h"
#include "py/runtime.h"
}

namespace
{
string ObjectToString(mp_obj_t obj) {
  size_t length = 0;
  const char* text = mp_obj_str_get_data(obj, &length);
  return string(text, length);
}

mp_obj_t usb_connected() {
  return mp_obj_new_bool(MatrixOS::USB::Connected());
}
MP_DEFINE_CONST_FUN_OBJ_0(usb_connected_obj, usb_connected);

mp_obj_t cdc_connected() {
  return mp_obj_new_bool(MatrixOS::USB::CDC::Connected());
}
MP_DEFINE_CONST_FUN_OBJ_0(cdc_connected_obj, cdc_connected);

mp_obj_t cdc_available() {
  return mp_obj_new_int_from_uint(MatrixOS::USB::CDC::Available());
}
MP_DEFINE_CONST_FUN_OBJ_0(cdc_available_obj, cdc_available);

mp_obj_t cdc_poll() {
  MatrixOS::USB::CDC::Poll();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(cdc_poll_obj, cdc_poll);

mp_obj_t cdc_print(mp_obj_t textObj) {
  MatrixOS::USB::CDC::Print(ObjectToString(textObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cdc_print_obj, cdc_print);

mp_obj_t cdc_println(mp_obj_t textObj) {
  MatrixOS::USB::CDC::Println(ObjectToString(textObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(cdc_println_obj, cdc_println);

mp_obj_t cdc_flush() {
  MatrixOS::USB::CDC::Flush();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(cdc_flush_obj, cdc_flush);

mp_obj_t cdc_read() {
  int8_t value = MatrixOS::USB::CDC::Read();
  if (value < 0)
  {
    return mp_const_none;
  }
  return mp_obj_new_int((uint8_t)value);
}
MP_DEFINE_CONST_FUN_OBJ_0(cdc_read_obj, cdc_read);

mp_obj_t cdc_read_bytes(mp_obj_t lengthObj) {
  mp_int_t requested = mp_obj_get_int(lengthObj);
  if (requested <= 0)
  {
    return mp_obj_new_bytes(nullptr, 0);
  }

  vstr_t vstr;
  vstr_init_len(&vstr, (size_t)requested);
  uint32_t bytesRead = MatrixOS::USB::CDC::ReadBytes(vstr.buf, (uint32_t)requested);
  vstr.len = bytesRead;
  return mp_obj_new_bytes_from_vstr(&vstr);
}
MP_DEFINE_CONST_FUN_OBJ_1(cdc_read_bytes_obj, cdc_read_bytes);

mp_obj_t cdc_read_string() {
  string value = MatrixOS::USB::CDC::ReadString();
  return mp_obj_new_str(value.c_str(), value.size());
}
MP_DEFINE_CONST_FUN_OBJ_0(cdc_read_string_obj, cdc_read_string);

static const mp_rom_map_elem_t cdc_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_connected), MP_ROM_PTR(&cdc_connected_obj)},
    {MP_ROM_QSTR(MP_QSTR_available), MP_ROM_PTR(&cdc_available_obj)},
    {MP_ROM_QSTR(MP_QSTR_poll), MP_ROM_PTR(&cdc_poll_obj)},
    {MP_ROM_QSTR(MP_QSTR_print), MP_ROM_PTR(&cdc_print_obj)},
    {MP_ROM_QSTR(MP_QSTR_println), MP_ROM_PTR(&cdc_println_obj)},
    {MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&cdc_flush_obj)},
    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&cdc_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_read_bytes), MP_ROM_PTR(&cdc_read_bytes_obj)},
    {MP_ROM_QSTR(MP_QSTR_read_string), MP_ROM_PTR(&cdc_read_string_obj)},
};
MP_DEFINE_CONST_DICT(cdc_globals, cdc_globals_table);

const mp_obj_module_t matrixos_usb_cdc_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&cdc_globals,
};

static const mp_rom_map_elem_t usb_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_connected), MP_ROM_PTR(&usb_connected_obj)},
    {MP_ROM_QSTR(MP_QSTR_CDC), MP_ROM_PTR(&matrixos_usb_cdc_module)},
};
MP_DEFINE_CONST_DICT(usb_globals, usb_globals_table);
} // namespace

extern const mp_obj_module_t matrixos_usb_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&usb_globals,
};
