// Exposes MatrixOS system lifecycle, timing, version, and app launch helpers to MicroPython.
#include "matrixos_common.h"
#include "matrixos_modules.h"

extern "C" {
#include "py/runtime.h"
}

using namespace MatrixOSPython;

mp_obj_t sys_millis() {
  return MakeTick(MatrixOS::SYS::Millis());
}
MP_DEFINE_CONST_FUN_OBJ_0(sys_millis_obj, sys_millis);

mp_obj_t sys_micros() {
  return MakeTick(MatrixOS::SYS::Micros());
}
MP_DEFINE_CONST_FUN_OBJ_0(sys_micros_obj, sys_micros);

mp_obj_t sys_sleep_ms(mp_obj_t msObj) {
  MatrixOS::SYS::DelayMs((uint32_t)mp_obj_get_int(msObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(sys_sleep_ms_obj, sys_sleep_ms);

mp_obj_t sys_exit_app() {
  MatrixOS::SYS::ExitAPP();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(sys_exit_app_obj, sys_exit_app);

mp_obj_t sys_reboot() {
  MatrixOS::SYS::Reboot();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(sys_reboot_obj, sys_reboot);

mp_obj_t sys_bootloader() {
  MatrixOS::SYS::Bootloader();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(sys_bootloader_obj, sys_bootloader);

mp_obj_t sys_open_setting() {
  MatrixOS::SYS::OpenSetting();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(sys_open_setting_obj, sys_open_setting);

mp_obj_t sys_execute_app(size_t argc, const mp_obj_t* args) {
  vector<string> appArgs;
  if (argc > 2)
  {
    mp_obj_t* argItems = nullptr;
    size_t argCount = 0;
    mp_obj_get_array(args[2], &argCount, &argItems);
    for (size_t i = 0; i < argCount; i++)
    {
      size_t length = 0;
      const char* text = mp_obj_str_get_data(argItems[i], &length);
      appArgs.push_back(string(text, length));
    }
  }

  if (mp_obj_is_int(args[0]) && argc == 1)
  {
    MatrixOS::SYS::ExecuteAPP((uint32_t)mp_obj_get_int_truncated(args[0]), appArgs);
  }
  else if (mp_obj_is_int(args[0]))
  {
    MatrixOS::SYS::ExecuteAPP((uint32_t)mp_obj_get_int_truncated(args[0]), appArgs);
  }
  else
  {
    if (argc < 2)
    {
      mp_raise_ValueError(MP_ERROR_TEXT("execute_app requires app_id or author, app_name"));
    }
    size_t authorLength = 0;
    size_t appNameLength = 0;
    const char* author = mp_obj_str_get_data(args[0], &authorLength);
    const char* appName = mp_obj_str_get_data(args[1], &appNameLength);
    MatrixOS::SYS::ExecuteAPP(string(author, authorLength), string(appName, appNameLength), appArgs);
  }
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(sys_execute_app_obj, 1, 3, sys_execute_app);

mp_obj_t sys_version() {
  return mp_obj_new_str(MATRIXOS_VERSION_STRING.c_str(), MATRIXOS_VERSION_STRING.size());
}
MP_DEFINE_CONST_FUN_OBJ_0(sys_version_obj, sys_version);

mp_obj_t sys_version_id() {
  return mp_obj_new_int_from_uint(MATRIXOS_VERSION_ID);
}
MP_DEFINE_CONST_FUN_OBJ_0(sys_version_id_obj, sys_version_id);

mp_obj_t sys_task_yield() {
  taskYIELD();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(sys_task_yield_obj, sys_task_yield);

static const mp_rom_map_elem_t sys_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_millis), MP_ROM_PTR(&sys_millis_obj)},
    {MP_ROM_QSTR(MP_QSTR_micros), MP_ROM_PTR(&sys_micros_obj)},
    {MP_ROM_QSTR(MP_QSTR_sleep_ms), MP_ROM_PTR(&sys_sleep_ms_obj)},
    {MP_ROM_QSTR(MP_QSTR_task_yield), MP_ROM_PTR(&sys_task_yield_obj)},
    {MP_ROM_QSTR(MP_QSTR_exit_app), MP_ROM_PTR(&sys_exit_app_obj)},
    {MP_ROM_QSTR(MP_QSTR_reboot), MP_ROM_PTR(&sys_reboot_obj)},
    {MP_ROM_QSTR(MP_QSTR_bootloader), MP_ROM_PTR(&sys_bootloader_obj)},
    {MP_ROM_QSTR(MP_QSTR_open_setting), MP_ROM_PTR(&sys_open_setting_obj)},
    {MP_ROM_QSTR(MP_QSTR_execute_app), MP_ROM_PTR(&sys_execute_app_obj)},
    {MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&sys_version_obj)},
    {MP_ROM_QSTR(MP_QSTR_version_id), MP_ROM_PTR(&sys_version_id_obj)},
};
MP_DEFINE_CONST_DICT(sys_globals, sys_globals_table);

extern const mp_obj_module_t matrixos_sys_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&sys_globals,
};
