// Exposes MatrixOS LED drawing, layer, brightness, and partition APIs to MicroPython.
#include "matrixos_common.h"
#include "matrixos_modules.h"

extern "C" {
#include "py/objlist.h"
#include "py/runtime.h"
}

using namespace MatrixOSPython;

mp_obj_t led_clear(size_t argc, const mp_obj_t* args) {
  uint8_t layer = argc > 0 ? (uint8_t)mp_obj_get_int(args[0]) : 255;
  MatrixOS::LED::Fill(Color::Black, layer);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_clear_obj, 0, 1, led_clear);

mp_obj_t led_fill(size_t argc, const mp_obj_t* args) {
  Color color = ObjectToColor(args[0]);
  uint8_t layer = argc > 1 ? (uint8_t)mp_obj_get_int(args[1]) : 255;
  MatrixOS::LED::Fill(color, layer);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_fill_obj, 1, 2, led_fill);

mp_obj_t led_set_xy(size_t argc, const mp_obj_t* args) {
  int16_t x = (int16_t)mp_obj_get_int(args[0]);
  int16_t y = (int16_t)mp_obj_get_int(args[1]);
  Color color = ObjectToColor(args[2]);
  uint8_t layer = argc > 3 ? (uint8_t)mp_obj_get_int(args[3]) : 255;
  MatrixOS::LED::SetColor(Point(x, y), color, layer);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_set_xy_obj, 3, 4, led_set_xy);

mp_obj_t led_set_index(size_t argc, const mp_obj_t* args) {
  uint16_t index = (uint16_t)mp_obj_get_int(args[0]);
  Color color = ObjectToColor(args[1]);
  uint8_t layer = argc > 2 ? (uint8_t)mp_obj_get_int(args[2]) : 255;
  MatrixOS::LED::SetColor(index, color, layer);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_set_index_obj, 2, 3, led_set_index);

mp_obj_t led_update(size_t argc, const mp_obj_t* args) {
  uint8_t layer = argc > 0 ? (uint8_t)mp_obj_get_int(args[0]) : 255;
  MatrixOS::LED::Update(layer);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_update_obj, 0, 1, led_update);

mp_obj_t led_fill_partition(size_t argc, const mp_obj_t* args) {
  size_t nameLength = 0;
  const char* name = mp_obj_str_get_data(args[0], &nameLength);
  Color color = ObjectToColor(args[1]);
  uint8_t layer = argc > 2 ? (uint8_t)mp_obj_get_int(args[2]) : 255;
  return mp_obj_new_bool(MatrixOS::LED::FillPartition(string(name, nameLength), color, layer));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_fill_partition_obj, 2, 3, led_fill_partition);

mp_obj_t led_next_brightness() {
  MatrixOS::LED::NextBrightness();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(led_next_brightness_obj, led_next_brightness);

mp_obj_t led_set_brightness(mp_obj_t brightnessObj) {
  MatrixOS::LED::SetBrightness((uint8_t)mp_obj_get_int(brightnessObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(led_set_brightness_obj, led_set_brightness);

mp_obj_t led_set_brightness_multiplier(mp_obj_t partitionObj, mp_obj_t multiplierObj) {
  size_t nameLength = 0;
  const char* name = mp_obj_str_get_data(partitionObj, &nameLength);
  float multiplier = (float)mp_obj_get_int(multiplierObj) / 1000.0f;
  return mp_obj_new_bool(MatrixOS::LED::SetBrightnessMultiplier(string(name, nameLength), multiplier));
}
MP_DEFINE_CONST_FUN_OBJ_2(led_set_brightness_multiplier_obj, led_set_brightness_multiplier);

mp_obj_t led_current_layer() {
  return mp_obj_new_int(MatrixOS::LED::CurrentLayer());
}
MP_DEFINE_CONST_FUN_OBJ_0(led_current_layer_obj, led_current_layer);

mp_obj_t led_create_layer(size_t argc, const mp_obj_t* args) {
  uint16_t crossfade = argc > 0 ? (uint16_t)mp_obj_get_int(args[0]) : crossfadeDuration;
  return mp_obj_new_int(MatrixOS::LED::CreateLayer(crossfade));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_create_layer_obj, 0, 1, led_create_layer);

mp_obj_t led_copy_layer(mp_obj_t destObj, mp_obj_t srcObj) {
  MatrixOS::LED::CopyLayer((uint8_t)mp_obj_get_int(destObj), (uint8_t)mp_obj_get_int(srcObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(led_copy_layer_obj, led_copy_layer);

mp_obj_t led_destroy_layer(size_t argc, const mp_obj_t* args) {
  uint16_t crossfade = argc > 0 ? (uint16_t)mp_obj_get_int(args[0]) : crossfadeDuration;
  return mp_obj_new_bool(MatrixOS::LED::DestroyLayer(crossfade));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_destroy_layer_obj, 0, 1, led_destroy_layer);

mp_obj_t led_fade(size_t argc, const mp_obj_t* args) {
  uint16_t crossfade = argc > 0 ? (uint16_t)mp_obj_get_int(args[0]) : crossfadeDuration;
  MatrixOS::LED::Fade(crossfade);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_fade_obj, 0, 1, led_fade);

mp_obj_t led_pause_update(size_t argc, const mp_obj_t* args) {
  bool pause = argc == 0 || mp_obj_is_true(args[0]);
  MatrixOS::LED::PauseUpdate(pause);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_pause_update_obj, 0, 1, led_pause_update);

mp_obj_t led_count() {
  return mp_obj_new_int(MatrixOS::LED::GetLEDCount());
}
MP_DEFINE_CONST_FUN_OBJ_0(led_count_obj, led_count);

mp_obj_t led_partitions() {
  mp_obj_t partitions = mp_obj_new_list(0, nullptr);
  for (const LEDPartition& partition : Device::LED::partitions)
  {
    mp_obj_list_append(partitions, MakePartition(partition));
  }
  return partitions;
}
MP_DEFINE_CONST_FUN_OBJ_0(led_partitions_obj, led_partitions);

mp_obj_t led_get_partition(mp_obj_t partitionObj) {
  if (mp_obj_is_int(partitionObj))
  {
    int index = mp_obj_get_int(partitionObj);
    if (index < 0 || index >= (int)Device::LED::partitions.size())
    {
      return mp_const_none;
    }
    return MakePartition(Device::LED::partitions[index]);
  }

  size_t nameLength = 0;
  const char* nameData = mp_obj_str_get_data(partitionObj, &nameLength);
  string name(nameData, nameLength);
  for (const LEDPartition& partition : Device::LED::partitions)
  {
    if (partition.name == name)
    {
      return MakePartition(partition);
    }
  }

  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(led_get_partition_obj, led_get_partition);

static const mp_rom_map_elem_t led_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&led_clear_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&led_fill_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_xy), MP_ROM_PTR(&led_set_xy_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_index), MP_ROM_PTR(&led_set_index_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill_partition), MP_ROM_PTR(&led_fill_partition_obj)},
    {MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&led_update_obj)},
    {MP_ROM_QSTR(MP_QSTR_next_brightness), MP_ROM_PTR(&led_next_brightness_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_brightness), MP_ROM_PTR(&led_set_brightness_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_brightness_multiplier), MP_ROM_PTR(&led_set_brightness_multiplier_obj)},
    {MP_ROM_QSTR(MP_QSTR_current_layer), MP_ROM_PTR(&led_current_layer_obj)},
    {MP_ROM_QSTR(MP_QSTR_create_layer), MP_ROM_PTR(&led_create_layer_obj)},
    {MP_ROM_QSTR(MP_QSTR_copy_layer), MP_ROM_PTR(&led_copy_layer_obj)},
    {MP_ROM_QSTR(MP_QSTR_destroy_layer), MP_ROM_PTR(&led_destroy_layer_obj)},
    {MP_ROM_QSTR(MP_QSTR_fade), MP_ROM_PTR(&led_fade_obj)},
    {MP_ROM_QSTR(MP_QSTR_pause_update), MP_ROM_PTR(&led_pause_update_obj)},
    {MP_ROM_QSTR(MP_QSTR_count), MP_ROM_PTR(&led_count_obj)},
    {MP_ROM_QSTR(MP_QSTR_partitions), MP_ROM_PTR(&led_partitions_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_partition), MP_ROM_PTR(&led_get_partition_obj)},
};
MP_DEFINE_CONST_DICT(led_globals, led_globals_table);

extern const mp_obj_module_t matrixos_led_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&led_globals,
};
