// Exposes MatrixOS UI utility dialogs to MicroPython.
#include "matrixos_common.h"
#include "matrixos_modules.h"

#include <climits>

#ifndef NO_QSTR
#include "UI/UIUtilities.h"
#endif

using namespace MatrixOSPython;

mp_obj_t ui_text_scroll(size_t argc, const mp_obj_t* args) {
  size_t length = 0;
  const char* text = mp_obj_str_get_data(args[0], &length);
  Color color = ObjectToColor(args[1]);
  uint16_t speed = argc > 2 ? (uint16_t)mp_obj_get_int(args[2]) : 10;
  bool loop = argc > 3 ? mp_obj_is_true(args[3]) : false;

  MatrixOS::UIUtility::TextScroll(string(text, length), color, speed, loop);
  return mp_const_none;
}
extern const mp_obj_fun_builtin_var_t ui_text_scroll_obj = {
    .base = {.type = &mp_type_fun_builtin_var},
    .sig = MP_OBJ_FUN_MAKE_SIG(2, 4, false),
    .fun = {.var = ui_text_scroll},
};

mp_obj_t ui_color_picker(size_t argc, const mp_obj_t* args) {
  Color color = ObjectToColor(args[0]);
  bool shade = argc > 1 ? mp_obj_is_true(args[1]) : true;
  if (!MatrixOS::UIUtility::ColorPicker(color, shade))
  {
    return mp_const_none;
  }
  return mp_obj_new_int_from_uint(ColorToRgb(color));
}
extern const mp_obj_fun_builtin_var_t ui_color_picker_obj = {
    .base = {.type = &mp_type_fun_builtin_var},
    .sig = MP_OBJ_FUN_MAKE_SIG(1, 2, false),
    .fun = {.var = ui_color_picker},
};

mp_obj_t ui_number_selector(size_t argc, const mp_obj_t* args) {
  int32_t value = mp_obj_get_int(args[0]);
  Color color = ObjectToColor(args[1]);
  size_t length = 0;
  const char* text = mp_obj_str_get_data(args[2], &length);
  int32_t lowerLimit = argc > 3 ? mp_obj_get_int(args[3]) : INT_MIN;
  int32_t upperLimit = argc > 4 ? mp_obj_get_int(args[4]) : INT_MAX;

  int32_t selected = MatrixOS::UIUtility::NumberSelector8x8(value, color, string(text, length), lowerLimit, upperLimit);
  return mp_obj_new_int(selected);
}
extern const mp_obj_fun_builtin_var_t ui_number_selector_obj = {
    .base = {.type = &mp_type_fun_builtin_var},
    .sig = MP_OBJ_FUN_MAKE_SIG(3, 5, false),
    .fun = {.var = ui_number_selector},
};
