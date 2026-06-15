// Exposes small MatrixOS utility helpers to MicroPython.
#include "matrixos_common.h"
#include "matrixos_modules.h"

mp_obj_t utils_string_hash(mp_obj_t textObj) {
  size_t textLength = 0;
  const char* text = mp_obj_str_get_data(textObj, &textLength);
  return mp_obj_new_int_from_uint(StringHash(string(text, textLength)));
}
MP_DEFINE_CONST_FUN_OBJ_1(utils_string_hash_obj, utils_string_hash);

static const mp_rom_map_elem_t utils_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_string_hash), MP_ROM_PTR(&utils_string_hash_obj)},
};
MP_DEFINE_CONST_DICT(utils_globals, utils_globals_table);

extern const mp_obj_module_t matrixos_utils_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&utils_globals,
};
