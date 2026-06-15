// Shared declarations for MatrixOS UI component MicroPython bindings.
#pragma once

#include "matrixos_common.h"

#ifndef NO_QSTR
#ifdef MAX
#undef MAX
#endif
#include "UI/UI.h"
#endif

extern "C" {
#include "py/obj.h"
}

typedef struct _matrixos_component_base_t {
  mp_obj_base_t base;
  UIComponent* component;
  bool owns_component;
  mp_obj_t enable_callback;
} matrixos_component_base_t;

bool component_close_native(matrixos_component_base_t* self, bool force);

extern const mp_obj_type_t button_type;
extern const mp_obj_type_t selector_type;
extern const mp_obj_type_t number_type;
extern const mp_obj_type_t toggle_type;
extern const mp_obj_type_t custom_component_type;
