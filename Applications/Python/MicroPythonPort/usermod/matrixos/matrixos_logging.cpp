// Exposes MatrixOS structured logging helpers to MicroPython.
#include "matrixos_common.h"
#include "matrixos_modules.h"

namespace
{
void ReadTagAndMessage(mp_obj_t tagObj, mp_obj_t messageObj, string* tag, string* message) {
  size_t tagLength = 0;
  size_t messageLength = 0;
  const char* tagText = mp_obj_str_get_data(tagObj, &tagLength);
  const char* messageText = mp_obj_str_get_data(messageObj, &messageLength);
  *tag = string(tagText, tagLength);
  *message = string(messageText, messageLength);
}
} // namespace

mp_obj_t logging_error(mp_obj_t tagObj, mp_obj_t messageObj) {
  string tag;
  string message;
  ReadTagAndMessage(tagObj, messageObj, &tag, &message);
  MatrixOS::Logging::LogError(tag, "%s", message.c_str());
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(logging_error_obj, logging_error);

mp_obj_t logging_warning(mp_obj_t tagObj, mp_obj_t messageObj) {
  string tag;
  string message;
  ReadTagAndMessage(tagObj, messageObj, &tag, &message);
  MatrixOS::Logging::LogWarning(tag, "%s", message.c_str());
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(logging_warning_obj, logging_warning);

mp_obj_t logging_info(mp_obj_t tagObj, mp_obj_t messageObj) {
  string tag;
  string message;
  ReadTagAndMessage(tagObj, messageObj, &tag, &message);
  MatrixOS::Logging::LogInfo(tag, "%s", message.c_str());
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(logging_info_obj, logging_info);

mp_obj_t logging_debug(mp_obj_t tagObj, mp_obj_t messageObj) {
  string tag;
  string message;
  ReadTagAndMessage(tagObj, messageObj, &tag, &message);
  MatrixOS::Logging::LogDebug(tag, "%s", message.c_str());
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(logging_debug_obj, logging_debug);

mp_obj_t logging_verbose(mp_obj_t tagObj, mp_obj_t messageObj) {
  string tag;
  string message;
  ReadTagAndMessage(tagObj, messageObj, &tag, &message);
  MatrixOS::Logging::LogVerbose(tag, "%s", message.c_str());
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(logging_verbose_obj, logging_verbose);

static const mp_rom_map_elem_t logging_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_error), MP_ROM_PTR(&logging_error_obj)},
    {MP_ROM_QSTR(MP_QSTR_warning), MP_ROM_PTR(&logging_warning_obj)},
    {MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&logging_info_obj)},
    {MP_ROM_QSTR(MP_QSTR_debug), MP_ROM_PTR(&logging_debug_obj)},
    {MP_ROM_QSTR(MP_QSTR_verbose), MP_ROM_PTR(&logging_verbose_obj)},
};
MP_DEFINE_CONST_DICT(logging_globals, logging_globals_table);

extern const mp_obj_module_t matrixos_logging_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&logging_globals,
};
