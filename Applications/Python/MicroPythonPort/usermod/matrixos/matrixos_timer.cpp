// Implements the MatrixOS.Timer MicroPython type.
#include "matrixos_common.h"
#include "matrixos_modules.h"

using namespace MatrixOSPython;

typedef struct _matrixos_timer_obj_t {
  mp_obj_base_t base;
  uint32_t previous;
} matrixos_timer_obj_t;

mp_obj_t timer_make_new(const mp_obj_type_t* type, size_t argc, size_t n_kw, const mp_obj_t* args) {
  (void)argc;
  (void)n_kw;
  (void)args;

  matrixos_timer_obj_t* self = mp_obj_malloc(matrixos_timer_obj_t, type);
  self->previous = (uint32_t)MatrixOS::SYS::Millis();
  return MP_OBJ_FROM_PTR(self);
}

mp_obj_t timer_record_current(mp_obj_t selfObj) {
  matrixos_timer_obj_t* self = (matrixos_timer_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->previous = (uint32_t)MatrixOS::SYS::Millis();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(timer_record_current_obj, timer_record_current);

mp_obj_t timer_since_last_tick(mp_obj_t selfObj) {
  matrixos_timer_obj_t* self = (matrixos_timer_obj_t*)MP_OBJ_TO_PTR(selfObj);
  return mp_obj_new_int((uint32_t)MatrixOS::SYS::Millis() - self->previous);
}
MP_DEFINE_CONST_FUN_OBJ_1(timer_since_last_tick_obj, timer_since_last_tick);

mp_obj_t timer_is_longer(mp_obj_t selfObj, mp_obj_t msObj) {
  matrixos_timer_obj_t* self = (matrixos_timer_obj_t*)MP_OBJ_TO_PTR(selfObj);
  uint32_t ms = (uint32_t)mp_obj_get_int(msObj);
  return mp_obj_new_bool(((uint32_t)MatrixOS::SYS::Millis() - self->previous) >= ms);
}
MP_DEFINE_CONST_FUN_OBJ_2(timer_is_longer_obj, timer_is_longer);

mp_obj_t timer_tick(size_t argc, const mp_obj_t* args) {
  matrixos_timer_obj_t* self = (matrixos_timer_obj_t*)MP_OBJ_TO_PTR(args[0]);
  uint32_t ms = (uint32_t)mp_obj_get_int(args[1]);
  bool continuous = argc > 2 && mp_obj_is_true(args[2]);
  uint32_t now = (uint32_t)MatrixOS::SYS::Millis();
  if ((uint32_t)(now - self->previous) < ms)
  {
    return mp_const_false;
  }

  self->previous = continuous ? (self->previous + ms) : now;
  return mp_const_true;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(timer_tick_obj, 2, 3, timer_tick);

static const mp_rom_map_elem_t timer_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_tick), MP_ROM_PTR(&timer_tick_obj)},
    {MP_ROM_QSTR(MP_QSTR_is_longer), MP_ROM_PTR(&timer_is_longer_obj)},
    {MP_ROM_QSTR(MP_QSTR_since_last_tick), MP_ROM_PTR(&timer_since_last_tick_obj)},
    {MP_ROM_QSTR(MP_QSTR_record_current), MP_ROM_PTR(&timer_record_current_obj)},
};
MP_DEFINE_CONST_DICT(timer_locals, timer_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    matrixos_timer_type,
    MP_QSTR_Timer,
    MP_TYPE_FLAG_NONE,
    make_new, (const void*)timer_make_new,
    locals_dict, &timer_locals
);
