// Exposes native MatrixOS UI components to MicroPython.
#include "matrixos_ui_components.h"

extern "C" {
#include "py/runtime.h"
}

using namespace MatrixOSPython;

void component_init(matrixos_component_base_t* self, const mp_obj_type_t* type, UIComponent* component) {
  self->base.type = type;
  self->component = component;
  self->owns_component = true;
  self->enable_callback = mp_const_none;
}

void component_set_enable_callback(matrixos_component_base_t* self) {
  self->component->SetEnableFunc([self]() -> bool {
    if (self->enable_callback == mp_const_none)
    {
      return self->component->enabled;
    }

    mp_obj_t args[] = {self->enable_callback};
    mp_obj_t result = ProtectedCall(1, args);
    return result == mp_const_none ? false : mp_obj_is_true(result);
  });
}

mp_obj_t component_set_enabled(mp_obj_t selfObj, mp_obj_t enabledObj) {
  matrixos_component_base_t* self = (matrixos_component_base_t*)MP_OBJ_TO_PTR(selfObj);
  self->component->SetEnabled(mp_obj_is_true(enabledObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(component_set_enabled_obj, component_set_enabled);

mp_obj_t component_set_enable_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_component_base_t* self = (matrixos_component_base_t*)MP_OBJ_TO_PTR(selfObj);
  self->enable_callback = callbackObj;
  component_set_enable_callback(self);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(component_set_enable_func_obj, component_set_enable_func);

mp_obj_t component_close(mp_obj_t selfObj) {
  matrixos_component_base_t* self = (matrixos_component_base_t*)MP_OBJ_TO_PTR(selfObj);
  if (self->component == nullptr)
  {
    return mp_const_true;
  }
  if (UI::IsComponentAttached(self->component))
  {
    return mp_const_false;
  }
  if (self->owns_component)
  {
    delete self->component;
  }
  self->component = nullptr;
  return mp_const_true;
}
MP_DEFINE_CONST_FUN_OBJ_1(component_close_obj, component_close);

typedef struct _matrixos_button_obj_t {
  matrixos_component_base_t base;
  UIButton* button;
  mp_obj_t press_callback;
  mp_obj_t hold_callback;
  mp_obj_t color_callback;
} matrixos_button_obj_t;

typedef struct _matrixos_selector_obj_t {
  matrixos_component_base_t base;
  UISelector* selector;
  uint16_t value;
  mp_obj_t value_callback;
  mp_obj_t change_callback;
  mp_obj_t color_callback;
  mp_obj_t individual_color_callback;
  mp_obj_t name_callback;
} matrixos_selector_obj_t;

typedef struct _matrixos_number_obj_t {
  matrixos_component_base_t base;
  UI4pxNumber* number;
  int32_t value;
  mp_obj_t value_callback;
  mp_obj_t color_callback;
} matrixos_number_obj_t;

typedef struct _matrixos_toggle_obj_t {
  matrixos_component_base_t base;
  UIToggle* toggle;
  bool value;
  mp_obj_t press_callback;
  mp_obj_t hold_callback;
  mp_obj_t color_callback;
} matrixos_toggle_obj_t;

mp_obj_t button_make_new(const mp_obj_type_t* type, size_t argc, size_t n_kw, const mp_obj_t* args) {
  (void)n_kw;
  matrixos_button_obj_t* self = mp_obj_malloc(matrixos_button_obj_t, type);
  self->button = new UIButton();
  component_init(&self->base, type, self->button);
  self->press_callback = mp_const_none;
  self->hold_callback = mp_const_none;
  self->color_callback = mp_const_none;
  if (argc > 0)
  {
    size_t length = 0;
    const char* text = mp_obj_str_get_data(args[0], &length);
    self->button->SetName(string(text, length));
  }
  if (argc > 1)
  {
    self->button->SetColor(ObjectToColor(args[1]));
  }
  return MP_OBJ_FROM_PTR(self);
}

mp_obj_t button_set_name(mp_obj_t selfObj, mp_obj_t nameObj) {
  matrixos_button_obj_t* self = (matrixos_button_obj_t*)MP_OBJ_TO_PTR(selfObj);
  size_t length = 0;
  const char* text = mp_obj_str_get_data(nameObj, &length);
  self->button->SetName(string(text, length));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(button_set_name_obj, button_set_name);

mp_obj_t button_set_color(mp_obj_t selfObj, mp_obj_t colorObj) {
  matrixos_button_obj_t* self = (matrixos_button_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->button->SetColor(ObjectToColor(colorObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(button_set_color_obj, button_set_color);

mp_obj_t button_set_size(mp_obj_t selfObj, mp_obj_t dimensionObj) {
  matrixos_button_obj_t* self = (matrixos_button_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->button->SetSize(ObjectToDimension(dimensionObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(button_set_size_obj, button_set_size);

mp_obj_t button_set_color_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_button_obj_t* self = (matrixos_button_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->color_callback = callbackObj;
  self->button->SetColorFunc([self]() -> Color {
    return CallColorCallback(self->color_callback, self->button->color);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(button_set_color_func_obj, button_set_color_func);

mp_obj_t button_on_press(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_button_obj_t* self = (matrixos_button_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->press_callback = callbackObj;
  self->button->OnPress([self]() {
    mp_obj_t args[] = {self->press_callback};
    ProtectedCall(1, args);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(button_on_press_obj, button_on_press);

mp_obj_t button_on_hold(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_button_obj_t* self = (matrixos_button_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->hold_callback = callbackObj;
  self->button->OnHold([self]() {
    mp_obj_t args[] = {self->hold_callback};
    ProtectedCall(1, args);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(button_on_hold_obj, button_on_hold);

static const mp_rom_map_elem_t button_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&component_close_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_enabled), MP_ROM_PTR(&component_set_enabled_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_enable_func), MP_ROM_PTR(&component_set_enable_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_name), MP_ROM_PTR(&button_set_name_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_color), MP_ROM_PTR(&button_set_color_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_color_func), MP_ROM_PTR(&button_set_color_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_size), MP_ROM_PTR(&button_set_size_obj)},
    {MP_ROM_QSTR(MP_QSTR_on_press), MP_ROM_PTR(&button_on_press_obj)},
    {MP_ROM_QSTR(MP_QSTR_on_hold), MP_ROM_PTR(&button_on_hold_obj)},
};
MP_DEFINE_CONST_DICT(button_locals, button_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    button_type,
    MP_QSTR_Button,
    MP_TYPE_FLAG_NONE,
    make_new, (const void*)button_make_new,
    locals_dict, &button_locals
);

mp_obj_t selector_make_new(const mp_obj_type_t* type, size_t argc, size_t n_kw, const mp_obj_t* args) {
  (void)n_kw;
  matrixos_selector_obj_t* self = mp_obj_malloc(matrixos_selector_obj_t, type);
  self->selector = new UISelector();
  component_init(&self->base, type, self->selector);
  self->value = 0;
  self->value_callback = mp_const_none;
  self->change_callback = mp_const_none;
  self->color_callback = mp_const_none;
  self->individual_color_callback = mp_const_none;
  self->name_callback = mp_const_none;
  self->selector->SetValuePointer(&self->value);
  if (argc > 0)
  {
    self->selector->SetDimension(ObjectToDimension(args[0]));
  }
  if (argc > 1)
  {
    self->selector->SetCount((uint16_t)mp_obj_get_int(args[1]));
  }
  return MP_OBJ_FROM_PTR(self);
}

mp_obj_t selector_set_value(mp_obj_t selfObj, mp_obj_t valueObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->value = (uint16_t)mp_obj_get_int(valueObj);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_set_value_obj, selector_set_value);

mp_obj_t selector_get_value(mp_obj_t selfObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  return mp_obj_new_int(self->selector->GetValue());
}
MP_DEFINE_CONST_FUN_OBJ_1(selector_get_value_obj, selector_get_value);

mp_obj_t selector_set_value_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->value_callback = callbackObj;
  self->selector->SetValueFunc([self]() -> uint16_t {
    return (uint16_t)CallIntCallback(self->value_callback, self->value);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_set_value_func_obj, selector_set_value_func);

mp_obj_t selector_on_change(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->change_callback = callbackObj;
  self->selector->OnChange([self](uint16_t value) {
    mp_obj_t args[] = {self->change_callback, mp_obj_new_int(value)};
    ProtectedCall(2, args);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_on_change_obj, selector_on_change);

mp_obj_t selector_set_dimension(mp_obj_t selfObj, mp_obj_t dimensionObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->selector->SetDimension(ObjectToDimension(dimensionObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_set_dimension_obj, selector_set_dimension);

mp_obj_t selector_set_name(mp_obj_t selfObj, mp_obj_t nameObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  size_t length = 0;
  const char* text = mp_obj_str_get_data(nameObj, &length);
  self->selector->SetName(string(text, length));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_set_name_obj, selector_set_name);

mp_obj_t selector_set_count(mp_obj_t selfObj, mp_obj_t countObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->selector->SetCount((uint16_t)mp_obj_get_int(countObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_set_count_obj, selector_set_count);

mp_obj_t selector_set_direction(mp_obj_t selfObj, mp_obj_t directionObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->selector->SetDirection((UISelectorDirection)mp_obj_get_int(directionObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_set_direction_obj, selector_set_direction);

mp_obj_t selector_set_lit_mode(mp_obj_t selfObj, mp_obj_t litModeObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->selector->SetLitMode((UISelectorLitMode)mp_obj_get_int(litModeObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_set_lit_mode_obj, selector_set_lit_mode);

mp_obj_t selector_set_color(mp_obj_t selfObj, mp_obj_t colorObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->selector->SetColor(ObjectToColor(colorObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_set_color_obj, selector_set_color);

mp_obj_t selector_set_color_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->color_callback = callbackObj;
  self->selector->SetColorFunc([self]() -> Color {
    return CallColorCallback(self->color_callback, Color::White);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_set_color_func_obj, selector_set_color_func);

mp_obj_t selector_set_individual_color_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->individual_color_callback = callbackObj;
  self->selector->SetIndividualColorFunc([self](uint16_t index) -> Color {
    return CallIndexedColorCallback(self->individual_color_callback, index, Color::White);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_set_individual_color_func_obj, selector_set_individual_color_func);

mp_obj_t selector_set_name_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_selector_obj_t* self = (matrixos_selector_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->name_callback = callbackObj;
  self->selector->SetIndividualNameFunc([self](uint16_t index) -> string {
    return CallIndexedStringCallback(self->name_callback, index, self->selector->name);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(selector_set_name_func_obj, selector_set_name_func);

static const mp_rom_map_elem_t selector_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&component_close_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_enabled), MP_ROM_PTR(&component_set_enabled_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_enable_func), MP_ROM_PTR(&component_set_enable_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_value), MP_ROM_PTR(&selector_set_value_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_value), MP_ROM_PTR(&selector_get_value_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_value_func), MP_ROM_PTR(&selector_set_value_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_on_change), MP_ROM_PTR(&selector_on_change_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_dimension), MP_ROM_PTR(&selector_set_dimension_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_name), MP_ROM_PTR(&selector_set_name_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_count), MP_ROM_PTR(&selector_set_count_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_direction), MP_ROM_PTR(&selector_set_direction_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_lit_mode), MP_ROM_PTR(&selector_set_lit_mode_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_color), MP_ROM_PTR(&selector_set_color_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_color_func), MP_ROM_PTR(&selector_set_color_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_individual_color_func), MP_ROM_PTR(&selector_set_individual_color_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_name_func), MP_ROM_PTR(&selector_set_name_func_obj)},
};
MP_DEFINE_CONST_DICT(selector_locals, selector_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    selector_type,
    MP_QSTR_Selector,
    MP_TYPE_FLAG_NONE,
    make_new, (const void*)selector_make_new,
    locals_dict, &selector_locals
);

mp_obj_t number_make_new(const mp_obj_type_t* type, size_t argc, size_t n_kw, const mp_obj_t* args) {
  (void)n_kw;
  matrixos_number_obj_t* self = mp_obj_malloc(matrixos_number_obj_t, type);
  self->number = new UI4pxNumber();
  component_init(&self->base, type, self->number);
  self->value = 0;
  self->value_callback = mp_const_none;
  self->color_callback = mp_const_none;
  self->number->SetValuePointer(&self->value);
  if (argc > 0)
  {
    self->number->SetDigits((uint8_t)mp_obj_get_int(args[0]));
  }
  return MP_OBJ_FROM_PTR(self);
}

mp_obj_t number_set_value(mp_obj_t selfObj, mp_obj_t valueObj) {
  matrixos_number_obj_t* self = (matrixos_number_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->value = mp_obj_get_int(valueObj);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(number_set_value_obj, number_set_value);

mp_obj_t number_get_value(mp_obj_t selfObj) {
  matrixos_number_obj_t* self = (matrixos_number_obj_t*)MP_OBJ_TO_PTR(selfObj);
  return mp_obj_new_int(self->number->GetValue());
}
MP_DEFINE_CONST_FUN_OBJ_1(number_get_value_obj, number_get_value);

mp_obj_t number_set_value_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_number_obj_t* self = (matrixos_number_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->value_callback = callbackObj;
  self->number->SetValueFunc([self]() -> int32_t {
    return CallIntCallback(self->value_callback, self->value);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(number_set_value_func_obj, number_set_value_func);

mp_obj_t number_set_name(mp_obj_t selfObj, mp_obj_t nameObj) {
  matrixos_number_obj_t* self = (matrixos_number_obj_t*)MP_OBJ_TO_PTR(selfObj);
  size_t length = 0;
  const char* text = mp_obj_str_get_data(nameObj, &length);
  self->number->SetName(string(text, length));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(number_set_name_obj, number_set_name);

mp_obj_t number_set_color(mp_obj_t selfObj, mp_obj_t colorObj) {
  matrixos_number_obj_t* self = (matrixos_number_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->number->SetColor(ObjectToColor(colorObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(number_set_color_obj, number_set_color);

mp_obj_t number_set_alternative_color(mp_obj_t selfObj, mp_obj_t colorObj) {
  matrixos_number_obj_t* self = (matrixos_number_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->number->SetAlternativeColor(ObjectToColor(colorObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(number_set_alternative_color_obj, number_set_alternative_color);

mp_obj_t number_set_digits(mp_obj_t selfObj, mp_obj_t digitsObj) {
  matrixos_number_obj_t* self = (matrixos_number_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->number->SetDigits((uint8_t)mp_obj_get_int(digitsObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(number_set_digits_obj, number_set_digits);

mp_obj_t number_set_spacing(mp_obj_t selfObj, mp_obj_t spacingObj) {
  matrixos_number_obj_t* self = (matrixos_number_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->number->SetSpacing((uint8_t)mp_obj_get_int(spacingObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(number_set_spacing_obj, number_set_spacing);

mp_obj_t number_set_color_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_number_obj_t* self = (matrixos_number_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->color_callback = callbackObj;
  self->number->SetColorFunc([self](uint16_t digit) -> Color {
    return CallIndexedColorCallback(self->color_callback, digit, Color::White);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(number_set_color_func_obj, number_set_color_func);

static const mp_rom_map_elem_t number_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&component_close_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_enabled), MP_ROM_PTR(&component_set_enabled_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_enable_func), MP_ROM_PTR(&component_set_enable_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_value), MP_ROM_PTR(&number_set_value_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_value), MP_ROM_PTR(&number_get_value_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_value_func), MP_ROM_PTR(&number_set_value_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_name), MP_ROM_PTR(&number_set_name_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_color), MP_ROM_PTR(&number_set_color_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_alternative_color), MP_ROM_PTR(&number_set_alternative_color_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_digits), MP_ROM_PTR(&number_set_digits_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_spacing), MP_ROM_PTR(&number_set_spacing_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_color_func), MP_ROM_PTR(&number_set_color_func_obj)},
};
MP_DEFINE_CONST_DICT(number_locals, number_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    number_type,
    MP_QSTR_Number,
    MP_TYPE_FLAG_NONE,
    make_new, (const void*)number_make_new,
    locals_dict, &number_locals
);

mp_obj_t toggle_make_new(const mp_obj_type_t* type, size_t argc, size_t n_kw, const mp_obj_t* args) {
  (void)n_kw;
  matrixos_toggle_obj_t* self = mp_obj_malloc(matrixos_toggle_obj_t, type);
  self->toggle = new UIToggle();
  component_init(&self->base, type, self->toggle);
  self->value = false;
  self->press_callback = mp_const_none;
  self->hold_callback = mp_const_none;
  self->color_callback = mp_const_none;
  self->toggle->SetValuePointer(&self->value);
  if (argc > 0)
  {
    size_t length = 0;
    const char* text = mp_obj_str_get_data(args[0], &length);
    self->toggle->SetName(string(text, length));
  }
  if (argc > 1)
  {
    self->value = mp_obj_is_true(args[1]);
  }
  return MP_OBJ_FROM_PTR(self);
}

mp_obj_t toggle_set_value(mp_obj_t selfObj, mp_obj_t valueObj) {
  matrixos_toggle_obj_t* self = (matrixos_toggle_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->value = mp_obj_is_true(valueObj);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(toggle_set_value_obj, toggle_set_value);

mp_obj_t toggle_get_value(mp_obj_t selfObj) {
  matrixos_toggle_obj_t* self = (matrixos_toggle_obj_t*)MP_OBJ_TO_PTR(selfObj);
  return mp_obj_new_bool(self->value);
}
MP_DEFINE_CONST_FUN_OBJ_1(toggle_get_value_obj, toggle_get_value);

mp_obj_t toggle_set_name(mp_obj_t selfObj, mp_obj_t nameObj) {
  matrixos_toggle_obj_t* self = (matrixos_toggle_obj_t*)MP_OBJ_TO_PTR(selfObj);
  size_t length = 0;
  const char* text = mp_obj_str_get_data(nameObj, &length);
  self->toggle->SetName(string(text, length));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(toggle_set_name_obj, toggle_set_name);

mp_obj_t toggle_set_color(mp_obj_t selfObj, mp_obj_t colorObj) {
  matrixos_toggle_obj_t* self = (matrixos_toggle_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->toggle->SetColor(ObjectToColor(colorObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(toggle_set_color_obj, toggle_set_color);

mp_obj_t toggle_set_size(mp_obj_t selfObj, mp_obj_t dimensionObj) {
  matrixos_toggle_obj_t* self = (matrixos_toggle_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->toggle->SetSize(ObjectToDimension(dimensionObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(toggle_set_size_obj, toggle_set_size);

mp_obj_t toggle_set_color_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_toggle_obj_t* self = (matrixos_toggle_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->color_callback = callbackObj;
  self->toggle->SetColorFunc([self]() -> Color {
    return CallColorCallback(self->color_callback, self->toggle->color);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(toggle_set_color_func_obj, toggle_set_color_func);

mp_obj_t toggle_on_press(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_toggle_obj_t* self = (matrixos_toggle_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->press_callback = callbackObj;
  self->toggle->OnPress([self]() {
    mp_obj_t args[] = {self->press_callback, mp_obj_new_bool(self->value)};
    ProtectedCall(2, args);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(toggle_on_press_obj, toggle_on_press);

mp_obj_t toggle_on_hold(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_toggle_obj_t* self = (matrixos_toggle_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->hold_callback = callbackObj;
  self->toggle->OnHold([self]() {
    mp_obj_t args[] = {self->hold_callback};
    ProtectedCall(1, args);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(toggle_on_hold_obj, toggle_on_hold);

static const mp_rom_map_elem_t toggle_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&component_close_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_enabled), MP_ROM_PTR(&component_set_enabled_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_enable_func), MP_ROM_PTR(&component_set_enable_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_value), MP_ROM_PTR(&toggle_set_value_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_value), MP_ROM_PTR(&toggle_get_value_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_name), MP_ROM_PTR(&toggle_set_name_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_color), MP_ROM_PTR(&toggle_set_color_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_color_func), MP_ROM_PTR(&toggle_set_color_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_size), MP_ROM_PTR(&toggle_set_size_obj)},
    {MP_ROM_QSTR(MP_QSTR_on_press), MP_ROM_PTR(&toggle_on_press_obj)},
    {MP_ROM_QSTR(MP_QSTR_on_hold), MP_ROM_PTR(&toggle_on_hold_obj)},
};
MP_DEFINE_CONST_DICT(toggle_locals, toggle_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    toggle_type,
    MP_QSTR_Toggle,
    MP_TYPE_FLAG_NONE,
    make_new, (const void*)toggle_make_new,
    locals_dict, &toggle_locals
);
