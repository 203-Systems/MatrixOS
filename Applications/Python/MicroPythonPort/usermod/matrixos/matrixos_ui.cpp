// Exposes native MatrixOS UI containers and module globals to MicroPython.
#include "matrixos_common.h"
#include "matrixos_modules.h"
#include "matrixos_ui_components.h"

extern "C" {
#include "py/objlist.h"
#include "py/runtime.h"
}

using namespace MatrixOSPython;

typedef struct _matrixos_ui_obj_t {
  mp_obj_base_t base;
  UI* ui;
  mp_obj_t setup_callback;
  mp_obj_t loop_callback;
  mp_obj_t global_loop_callback;
  mp_obj_t pre_render_callback;
  mp_obj_t post_render_callback;
  mp_obj_t end_callback;
  mp_obj_t input_callback;
  mp_obj_t components;
} matrixos_ui_obj_t;

mp_obj_t ui_make_new(const mp_obj_type_t* type, size_t argc, size_t n_kw, const mp_obj_t* args) {
  (void)n_kw;
  string name = "";
  Color color = Color::White;
  bool newLayer = true;

  if (argc > 0)
  {
    size_t length = 0;
    const char* text = mp_obj_str_get_data(args[0], &length);
    name = string(text, length);
  }
  if (argc > 1)
  {
    color = ObjectToColor(args[1]);
  }
  if (argc > 2)
  {
    newLayer = mp_obj_is_true(args[2]);
  }

  matrixos_ui_obj_t* self = mp_obj_malloc(matrixos_ui_obj_t, type);
  self->ui = new UI(name, color, newLayer);
  self->setup_callback = mp_const_none;
  self->loop_callback = mp_const_none;
  self->global_loop_callback = mp_const_none;
  self->pre_render_callback = mp_const_none;
  self->post_render_callback = mp_const_none;
  self->end_callback = mp_const_none;
  self->input_callback = mp_const_none;
  self->components = mp_obj_new_list(0, nullptr);
  return MP_OBJ_FROM_PTR(self);
}

mp_obj_t ui_start(mp_obj_t selfObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  if (self->ui != nullptr)
  {
    self->ui->Start();
  }
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(ui_start_obj, ui_start);

mp_obj_t ui_exit(mp_obj_t selfObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  if (self->ui != nullptr)
  {
    self->ui->Exit();
  }
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(ui_exit_obj, ui_exit);

mp_obj_t ui_close(mp_obj_t selfObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  if (self->ui != nullptr)
  {
    delete self->ui;
    self->ui = nullptr;
  }
  self->components = mp_obj_new_list(0, nullptr);
  return mp_const_true;
}
MP_DEFINE_CONST_FUN_OBJ_1(ui_close_obj, ui_close);

mp_obj_t ui_set_name(mp_obj_t selfObj, mp_obj_t nameObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  size_t length = 0;
  const char* text = mp_obj_str_get_data(nameObj, &length);
  self->ui->SetName(string(text, length));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_set_name_obj, ui_set_name);

mp_obj_t ui_set_color(mp_obj_t selfObj, mp_obj_t colorObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->ui->SetColor(ObjectToColor(colorObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_set_color_obj, ui_set_color);

mp_obj_t ui_set_new_layer(mp_obj_t selfObj, mp_obj_t createObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->ui->ShouldCreatenewLEDLayer(mp_obj_is_true(createObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_set_new_layer_obj, ui_set_new_layer);

mp_obj_t ui_allow_exit(mp_obj_t selfObj, mp_obj_t allowObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->ui->AllowExit(mp_obj_is_true(allowObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_allow_exit_obj, ui_allow_exit);

mp_obj_t ui_set_fps(mp_obj_t selfObj, mp_obj_t fpsObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->ui->SetFPS((uint16_t)mp_obj_get_int(fpsObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_set_fps_obj, ui_set_fps);

mp_obj_t ui_clear(mp_obj_t selfObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->ui->ClearUIComponents();
  self->components = mp_obj_new_list(0, nullptr);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(ui_clear_obj, ui_clear);

mp_obj_t ui_add(mp_obj_t selfObj, mp_obj_t componentObj, mp_obj_t pointObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  matrixos_component_base_t* component = (matrixos_component_base_t*)MP_OBJ_TO_PTR(componentObj);
  if (component->component == nullptr)
  {
    mp_raise_ValueError(MP_ERROR_TEXT("component is closed"));
  }
  self->ui->AddUIComponent(component->component, ObjectToPoint(pointObj));
  mp_obj_list_append(self->components, componentObj);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(ui_add_obj, ui_add);

mp_obj_t ui_set_setup_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->setup_callback = callbackObj;
  self->ui->SetSetupFunc([self]() {
    mp_obj_t args[] = {self->setup_callback};
    ProtectedCall(1, args);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_set_setup_func_obj, ui_set_setup_func);

mp_obj_t ui_set_loop_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->loop_callback = callbackObj;
  self->ui->SetLoopFunc([self]() {
    mp_obj_t args[] = {self->loop_callback};
    ProtectedCall(1, args);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_set_loop_func_obj, ui_set_loop_func);

mp_obj_t ui_set_global_loop_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->global_loop_callback = callbackObj;
  self->ui->SetGlobalLoopFunc([self]() {
    mp_obj_t args[] = {self->global_loop_callback};
    ProtectedCall(1, args);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_set_global_loop_func_obj, ui_set_global_loop_func);

mp_obj_t ui_set_pre_render_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->pre_render_callback = callbackObj;
  self->ui->SetPreRenderFunc([self]() {
    mp_obj_t args[] = {self->pre_render_callback};
    ProtectedCall(1, args);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_set_pre_render_func_obj, ui_set_pre_render_func);

mp_obj_t ui_set_post_render_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->post_render_callback = callbackObj;
  self->ui->SetPostRenderFunc([self]() {
    mp_obj_t args[] = {self->post_render_callback};
    ProtectedCall(1, args);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_set_post_render_func_obj, ui_set_post_render_func);

mp_obj_t ui_set_end_func(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->end_callback = callbackObj;
  self->ui->SetEndFunc([self]() {
    mp_obj_t args[] = {self->end_callback};
    ProtectedCall(1, args);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_set_end_func_obj, ui_set_end_func);

mp_obj_t ui_set_input_handler(mp_obj_t selfObj, mp_obj_t callbackObj) {
  matrixos_ui_obj_t* self = (matrixos_ui_obj_t*)MP_OBJ_TO_PTR(selfObj);
  self->input_callback = callbackObj;
  self->ui->SetInputEventHandler([self](InputEvent* event) -> bool {
    mp_obj_t args[] = {self->input_callback, MakeInputEvent(*event)};
    mp_obj_t result = ProtectedCall(2, args);
    return result != mp_const_none && mp_obj_is_true(result);
  });
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ui_set_input_handler_obj, ui_set_input_handler);

static const mp_rom_map_elem_t ui_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&ui_start_obj)},
    {MP_ROM_QSTR(MP_QSTR_exit), MP_ROM_PTR(&ui_exit_obj)},
    {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&ui_close_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_name), MP_ROM_PTR(&ui_set_name_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_color), MP_ROM_PTR(&ui_set_color_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_new_layer), MP_ROM_PTR(&ui_set_new_layer_obj)},
    {MP_ROM_QSTR(MP_QSTR_allow_exit), MP_ROM_PTR(&ui_allow_exit_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_fps), MP_ROM_PTR(&ui_set_fps_obj)},
    {MP_ROM_QSTR(MP_QSTR_add), MP_ROM_PTR(&ui_add_obj)},
    {MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&ui_clear_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_setup_func), MP_ROM_PTR(&ui_set_setup_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_loop_func), MP_ROM_PTR(&ui_set_loop_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_global_loop_func), MP_ROM_PTR(&ui_set_global_loop_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_pre_render_func), MP_ROM_PTR(&ui_set_pre_render_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_post_render_func), MP_ROM_PTR(&ui_set_post_render_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_end_func), MP_ROM_PTR(&ui_set_end_func_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_input_handler), MP_ROM_PTR(&ui_set_input_handler_obj)},
};
MP_DEFINE_CONST_DICT(ui_locals, ui_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    ui_type,
    MP_QSTR_UI,
    MP_TYPE_FLAG_NONE,
    make_new, (const void*)ui_make_new,
    locals_dict, &ui_locals
);

extern const mp_obj_fun_builtin_var_t ui_text_scroll_obj;
extern const mp_obj_fun_builtin_var_t ui_color_picker_obj;
extern const mp_obj_fun_builtin_var_t ui_number_selector_obj;

static const mp_rom_map_elem_t ui_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_UI), MP_ROM_PTR(&ui_type)},
    {MP_ROM_QSTR(MP_QSTR_Button), MP_ROM_PTR(&button_type)},
    {MP_ROM_QSTR(MP_QSTR_Selector), MP_ROM_PTR(&selector_type)},
    {MP_ROM_QSTR(MP_QSTR_Number), MP_ROM_PTR(&number_type)},
    {MP_ROM_QSTR(MP_QSTR_Toggle), MP_ROM_PTR(&toggle_type)},
    {MP_ROM_QSTR(MP_QSTR_text_scroll), MP_ROM_PTR(&ui_text_scroll_obj)},
    {MP_ROM_QSTR(MP_QSTR_color_picker), MP_ROM_PTR(&ui_color_picker_obj)},
    {MP_ROM_QSTR(MP_QSTR_number_selector), MP_ROM_PTR(&ui_number_selector_obj)},
    {MP_ROM_QSTR(MP_QSTR_DIRECTION_RIGHT_THEN_DOWN), MP_ROM_INT((int)UISelectorDirection::RIGHT_THEN_DOWN)},
    {MP_ROM_QSTR(MP_QSTR_DIRECTION_DOWN_THEN_RIGHT), MP_ROM_INT((int)UISelectorDirection::DOWN_THEN_RIGHT)},
    {MP_ROM_QSTR(MP_QSTR_DIRECTION_LEFT_THEN_DOWN), MP_ROM_INT((int)UISelectorDirection::LEFT_THEN_DOWN)},
    {MP_ROM_QSTR(MP_QSTR_DIRECTION_DOWN_THEN_LEFT), MP_ROM_INT((int)UISelectorDirection::DOWN_THEN_LEFT)},
    {MP_ROM_QSTR(MP_QSTR_DIRECTION_UP_THEN_RIGHT), MP_ROM_INT((int)UISelectorDirection::UP_THEN_RIGHT)},
    {MP_ROM_QSTR(MP_QSTR_DIRECTION_RIGHT_THEN_UP), MP_ROM_INT((int)UISelectorDirection::RIGHT_THEN_UP)},
    {MP_ROM_QSTR(MP_QSTR_DIRECTION_UP_THEN_LEFT), MP_ROM_INT((int)UISelectorDirection::UP_THEN_LEFT)},
    {MP_ROM_QSTR(MP_QSTR_DIRECTION_LEFT_THEN_UP), MP_ROM_INT((int)UISelectorDirection::LEFT_THEN_UP)},
    {MP_ROM_QSTR(MP_QSTR_LIT_EQUAL), MP_ROM_INT((int)UISelectorLitMode::LIT_EQUAL)},
    {MP_ROM_QSTR(MP_QSTR_LIT_LESS_EQUAL_THAN), MP_ROM_INT((int)UISelectorLitMode::LIT_LESS_EQUAL_THAN)},
    {MP_ROM_QSTR(MP_QSTR_LIT_GREATER_EQUAL_THAN), MP_ROM_INT((int)UISelectorLitMode::LIT_GREATER_EQUAL_THAN)},
    {MP_ROM_QSTR(MP_QSTR_LIT_ALWAYS), MP_ROM_INT((int)UISelectorLitMode::LIT_ALWAYS)},
};
MP_DEFINE_CONST_DICT(ui_module_globals, ui_module_globals_table);

extern const mp_obj_module_t matrixos_ui_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&ui_module_globals,
};
