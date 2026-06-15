// Declares MatrixOS MicroPython subsystem modules and exported native types.
#pragma once

extern "C" {
#include "py/obj.h"
}

extern const mp_obj_type_t matrixos_timer_type;
extern const mp_obj_module_t matrixos_sys_module;
extern const mp_obj_module_t matrixos_led_module;
extern const mp_obj_module_t matrixos_input_module;
extern const mp_obj_module_t matrixos_color_effects_module;
extern const mp_obj_module_t matrixos_nvs_module;
extern const mp_obj_module_t matrixos_utils_module;
extern const mp_obj_module_t matrixos_ui_module;
extern const mp_obj_module_t matrixos_logging_module;
extern const mp_obj_module_t matrixos_filesystem_module;
extern const mp_obj_module_t matrixos_usb_module;
extern const mp_obj_module_t matrixos_hid_module;
extern const mp_obj_module_t matrixos_midi_module;

extern const mp_obj_fun_builtin_var_t matrixos_color_obj;
