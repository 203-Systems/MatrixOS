// Exposes the MatrixOS.Color constructor for packed RGB values.
#include "matrixos_common.h"
#include "matrixos_modules.h"

using namespace MatrixOSPython;

mp_obj_t matrixos_color(size_t argc, const mp_obj_t* args) {
  if (argc == 1)
  {
    return mp_obj_new_int(ColorToRgb(ObjectToColor(args[0])));
  }

  uint8_t r = (uint8_t)mp_obj_get_int(args[0]);
  uint8_t g = (uint8_t)mp_obj_get_int(args[1]);
  uint8_t b = (uint8_t)mp_obj_get_int(args[2]);
  uint8_t w = argc > 3 ? (uint8_t)mp_obj_get_int(args[3]) : 0;
  return mp_obj_new_int(ColorToRgb(Color(r, g, b, w)));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(matrixos_color_obj, 1, 4, matrixos_color);
