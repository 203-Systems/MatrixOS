// Assembles the top-level MatrixOS MicroPython module from subsystem bindings.
#include "matrixos_common.h"
#include "matrixos_modules.h"

static const mp_rom_map_elem_t matrixos_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_MatrixOS)},
    {MP_ROM_QSTR(MP_QSTR_Color), MP_ROM_PTR(&matrixos_color_obj)},
    {MP_ROM_QSTR(MP_QSTR_Timer), MP_ROM_PTR(&matrixos_timer_type)},
    {MP_ROM_QSTR(MP_QSTR_SYS), MP_ROM_PTR(&matrixos_sys_module)},
    {MP_ROM_QSTR(MP_QSTR_LED), MP_ROM_PTR(&matrixos_led_module)},
    {MP_ROM_QSTR(MP_QSTR_Input), MP_ROM_PTR(&matrixos_input_module)},
    {MP_ROM_QSTR(MP_QSTR_ColorEffects), MP_ROM_PTR(&matrixos_color_effects_module)},
    {MP_ROM_QSTR(MP_QSTR_NVS), MP_ROM_PTR(&matrixos_nvs_module)},
    {MP_ROM_QSTR(MP_QSTR_Utils), MP_ROM_PTR(&matrixos_utils_module)},
    {MP_ROM_QSTR(MP_QSTR_UI), MP_ROM_PTR(&matrixos_ui_module)},
    {MP_ROM_QSTR(MP_QSTR_Logging), MP_ROM_PTR(&matrixos_logging_module)},
    {MP_ROM_QSTR(MP_QSTR_FileSystem), MP_ROM_PTR(&matrixos_filesystem_module)},
    {MP_ROM_QSTR(MP_QSTR_HID), MP_ROM_PTR(&matrixos_hid_module)},
    {MP_ROM_QSTR(MP_QSTR_MIDI), MP_ROM_PTR(&matrixos_midi_module)},
    {MP_ROM_QSTR(MP_QSTR_USB), MP_ROM_PTR(&matrixos_usb_module)},
};
MP_DEFINE_CONST_DICT(matrixos_globals, matrixos_globals_table);

extern "C" const mp_obj_module_t matrixos_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&matrixos_globals,
};

MP_REGISTER_MODULE(MP_QSTR_MatrixOS, matrixos_module);
