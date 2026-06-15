// Exposes MatrixOS ColorEffects helpers as packed RGB MicroPython functions.
#include "matrixos_common.h"
#include "matrixos_modules.h"

extern "C" {
#include "py/runtime.h"
}

using namespace MatrixOSPython;

static uint16_t GetPeriodArg(const mp_obj_t* args, size_t argc, size_t index, uint16_t defaultPeriod) {
  uint16_t period = argc > index ? (uint16_t)mp_obj_get_int(args[index]) : defaultPeriod;
  if (period == 0)
  {
    mp_raise_ValueError(MP_ERROR_TEXT("period must be greater than 0"));
  }
  return period;
}

mp_obj_t color_effects_rainbow(size_t argc, const mp_obj_t* args) {
  uint16_t period = GetPeriodArg(args, argc, 0, 1000);
  int32_t offset = argc > 1 ? (int32_t)mp_obj_get_int(args[1]) : 0;
  return mp_obj_new_int(ColorToRgb(ColorEffects::Rainbow(period, offset)));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(color_effects_rainbow_obj, 0, 2, color_effects_rainbow);

mp_obj_t color_effects_color_breath(size_t argc, const mp_obj_t* args) {
  Color color = ObjectToColor(args[0]);
  uint16_t period = GetPeriodArg(args, argc, 1, 1000);
  int32_t offset = argc > 2 ? (int32_t)mp_obj_get_int(args[2]) : 0;
  return mp_obj_new_int(ColorToRgb(ColorEffects::ColorBreath(color, period, offset)));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(color_effects_color_breath_obj, 1, 3, color_effects_color_breath);

mp_obj_t color_effects_color_strobe(size_t argc, const mp_obj_t* args) {
  Color color = ObjectToColor(args[0]);
  uint16_t period = GetPeriodArg(args, argc, 1, 1000);
  int32_t offset = argc > 2 ? (int32_t)mp_obj_get_int(args[2]) : 0;
  return mp_obj_new_int(ColorToRgb(ColorEffects::ColorStrobe(color, period, offset)));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(color_effects_color_strobe_obj, 1, 3, color_effects_color_strobe);

mp_obj_t color_effects_color_saw(size_t argc, const mp_obj_t* args) {
  Color color = ObjectToColor(args[0]);
  uint16_t period = GetPeriodArg(args, argc, 1, 1000);
  int32_t offset = argc > 2 ? (int32_t)mp_obj_get_int(args[2]) : 0;
  return mp_obj_new_int(ColorToRgb(ColorEffects::ColorSaw(color, period, offset)));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(color_effects_color_saw_obj, 1, 3, color_effects_color_saw);

static const mp_rom_map_elem_t color_effects_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_rainbow), MP_ROM_PTR(&color_effects_rainbow_obj)},
    {MP_ROM_QSTR(MP_QSTR_color_breath), MP_ROM_PTR(&color_effects_color_breath_obj)},
    {MP_ROM_QSTR(MP_QSTR_color_strobe), MP_ROM_PTR(&color_effects_color_strobe_obj)},
    {MP_ROM_QSTR(MP_QSTR_color_saw), MP_ROM_PTR(&color_effects_color_saw_obj)},
};
MP_DEFINE_CONST_DICT(color_effects_globals, color_effects_globals_table);

extern const mp_obj_module_t matrixos_color_effects_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&color_effects_globals,
};
