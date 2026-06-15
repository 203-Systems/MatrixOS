// Exposes MatrixOS HID keyboard, gamepad, and RawHID helpers to MicroPython.
#include "matrixos_common.h"

extern "C" {
#include "py/obj.h"
#include "py/runtime.h"
}

namespace
{
uint16_t ObjectToU16(mp_obj_t obj) {
  mp_int_t value = mp_obj_get_int(obj);
  if (value < 0)
  {
    value = 0;
  }
  if (value > UINT16_MAX)
  {
    value = UINT16_MAX;
  }
  return (uint16_t)value;
}

uint8_t ObjectToU8(mp_obj_t obj) {
  mp_int_t value = mp_obj_get_int(obj);
  if (value < 0)
  {
    value = 0;
  }
  if (value > UINT8_MAX)
  {
    value = UINT8_MAX;
  }
  return (uint8_t)value;
}

int16_t ObjectToI16(mp_obj_t obj) {
  mp_int_t value = mp_obj_get_int(obj);
  if (value < INT16_MIN)
  {
    value = INT16_MIN;
  }
  if (value > INT16_MAX)
  {
    value = INT16_MAX;
  }
  return (int16_t)value;
}

mp_obj_t hid_init() {
  MatrixOS::HID::Init();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(hid_init_obj, hid_init);

mp_obj_t hid_reset() {
  MatrixOS::HID::Reset();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(hid_reset_obj, hid_reset);

mp_obj_t hid_ready() {
  return mp_obj_new_bool(MatrixOS::HID::Ready());
}
MP_DEFINE_CONST_FUN_OBJ_0(hid_ready_obj, hid_ready);

mp_obj_t keyboard_tap(size_t argc, const mp_obj_t* args) {
  uint16_t lengthMs = argc > 1 ? ObjectToU16(args[1]) : 100;
  return mp_obj_new_bool(MatrixOS::HID::Keyboard::Tap((KeyboardKeycode)mp_obj_get_int(args[0]), lengthMs));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(keyboard_tap_obj, 1, 2, keyboard_tap);

mp_obj_t keyboard_press(mp_obj_t keycodeObj) {
  return mp_obj_new_bool(MatrixOS::HID::Keyboard::Press((KeyboardKeycode)mp_obj_get_int(keycodeObj)));
}
MP_DEFINE_CONST_FUN_OBJ_1(keyboard_press_obj, keyboard_press);

mp_obj_t keyboard_release(mp_obj_t keycodeObj) {
  return mp_obj_new_bool(MatrixOS::HID::Keyboard::Release((KeyboardKeycode)mp_obj_get_int(keycodeObj)));
}
MP_DEFINE_CONST_FUN_OBJ_1(keyboard_release_obj, keyboard_release);

mp_obj_t keyboard_release_all() {
  MatrixOS::HID::Keyboard::ReleaseAll();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(keyboard_release_all_obj, keyboard_release_all);

mp_obj_t gamepad_tap(size_t argc, const mp_obj_t* args) {
  uint16_t lengthMs = argc > 1 ? ObjectToU16(args[1]) : 100;
  MatrixOS::HID::Gamepad::Tap(ObjectToU8(args[0]), lengthMs);
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gamepad_tap_obj, 1, 2, gamepad_tap);

mp_obj_t gamepad_press(mp_obj_t buttonObj) {
  MatrixOS::HID::Gamepad::Press(ObjectToU8(buttonObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gamepad_press_obj, gamepad_press);

mp_obj_t gamepad_release(mp_obj_t buttonObj) {
  MatrixOS::HID::Gamepad::Release(ObjectToU8(buttonObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gamepad_release_obj, gamepad_release);

mp_obj_t gamepad_release_all() {
  MatrixOS::HID::Gamepad::ReleaseAll();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(gamepad_release_all_obj, gamepad_release_all);

mp_obj_t gamepad_button(mp_obj_t buttonObj, mp_obj_t stateObj) {
  MatrixOS::HID::Gamepad::Button(ObjectToU8(buttonObj), mp_obj_is_true(stateObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(gamepad_button_obj, gamepad_button);

mp_obj_t gamepad_buttons(mp_obj_t maskObj) {
  MatrixOS::HID::Gamepad::Buttons((uint32_t)mp_obj_get_int_truncated(maskObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gamepad_buttons_obj, gamepad_buttons);

mp_obj_t gamepad_x_axis(mp_obj_t valueObj) {
  MatrixOS::HID::Gamepad::XAxis(ObjectToI16(valueObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gamepad_x_axis_obj, gamepad_x_axis);

mp_obj_t gamepad_y_axis(mp_obj_t valueObj) {
  MatrixOS::HID::Gamepad::YAxis(ObjectToI16(valueObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gamepad_y_axis_obj, gamepad_y_axis);

mp_obj_t gamepad_z_axis(mp_obj_t valueObj) {
  MatrixOS::HID::Gamepad::ZAxis(ObjectToI16(valueObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gamepad_z_axis_obj, gamepad_z_axis);

mp_obj_t gamepad_rx_axis(mp_obj_t valueObj) {
  MatrixOS::HID::Gamepad::RXAxis(ObjectToI16(valueObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gamepad_rx_axis_obj, gamepad_rx_axis);

mp_obj_t gamepad_ry_axis(mp_obj_t valueObj) {
  MatrixOS::HID::Gamepad::RYAxis(ObjectToI16(valueObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gamepad_ry_axis_obj, gamepad_ry_axis);

mp_obj_t gamepad_rz_axis(mp_obj_t valueObj) {
  MatrixOS::HID::Gamepad::RZAxis(ObjectToI16(valueObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gamepad_rz_axis_obj, gamepad_rz_axis);

mp_obj_t gamepad_dpad(mp_obj_t directionObj) {
  MatrixOS::HID::Gamepad::DPad((GamepadDPadDirection)ObjectToU8(directionObj));
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(gamepad_dpad_obj, gamepad_dpad);

mp_obj_t rawhid_get(size_t argc, const mp_obj_t* args) {
  uint32_t timeoutMs = argc > 0 ? (uint32_t)mp_obj_get_int(args[0]) : 0;
  uint8_t* report = nullptr;
  size_t size = MatrixOS::HID::RawHID::Get(&report, timeoutMs);
  if (size == 0 || report == nullptr)
  {
    return mp_const_none;
  }
  return mp_obj_new_bytes(report, size);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rawhid_get_obj, 0, 1, rawhid_get);

mp_obj_t rawhid_send(mp_obj_t dataObj) {
  mp_buffer_info_t buffer;
  mp_get_buffer_raise(dataObj, &buffer, MP_BUFFER_READ);
  vector<uint8_t> report((uint8_t*)buffer.buf, (uint8_t*)buffer.buf + buffer.len);
  return mp_obj_new_bool(MatrixOS::HID::RawHID::Send(report));
}
MP_DEFINE_CONST_FUN_OBJ_1(rawhid_send_obj, rawhid_send);

static const mp_rom_map_elem_t keyboard_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_tap), MP_ROM_PTR(&keyboard_tap_obj)},
    {MP_ROM_QSTR(MP_QSTR_press), MP_ROM_PTR(&keyboard_press_obj)},
    {MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&keyboard_release_obj)},
    {MP_ROM_QSTR(MP_QSTR_release_all), MP_ROM_PTR(&keyboard_release_all_obj)},
};
MP_DEFINE_CONST_DICT(keyboard_globals, keyboard_globals_table);

const mp_obj_module_t matrixos_hid_keyboard_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&keyboard_globals,
};

static const mp_rom_map_elem_t gamepad_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_tap), MP_ROM_PTR(&gamepad_tap_obj)},
    {MP_ROM_QSTR(MP_QSTR_press), MP_ROM_PTR(&gamepad_press_obj)},
    {MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&gamepad_release_obj)},
    {MP_ROM_QSTR(MP_QSTR_release_all), MP_ROM_PTR(&gamepad_release_all_obj)},
    {MP_ROM_QSTR(MP_QSTR_button), MP_ROM_PTR(&gamepad_button_obj)},
    {MP_ROM_QSTR(MP_QSTR_buttons), MP_ROM_PTR(&gamepad_buttons_obj)},
    {MP_ROM_QSTR(MP_QSTR_x_axis), MP_ROM_PTR(&gamepad_x_axis_obj)},
    {MP_ROM_QSTR(MP_QSTR_y_axis), MP_ROM_PTR(&gamepad_y_axis_obj)},
    {MP_ROM_QSTR(MP_QSTR_z_axis), MP_ROM_PTR(&gamepad_z_axis_obj)},
    {MP_ROM_QSTR(MP_QSTR_rx_axis), MP_ROM_PTR(&gamepad_rx_axis_obj)},
    {MP_ROM_QSTR(MP_QSTR_ry_axis), MP_ROM_PTR(&gamepad_ry_axis_obj)},
    {MP_ROM_QSTR(MP_QSTR_rz_axis), MP_ROM_PTR(&gamepad_rz_axis_obj)},
    {MP_ROM_QSTR(MP_QSTR_dpad), MP_ROM_PTR(&gamepad_dpad_obj)},
};
MP_DEFINE_CONST_DICT(gamepad_globals, gamepad_globals_table);

const mp_obj_module_t matrixos_hid_gamepad_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&gamepad_globals,
};

static const mp_rom_map_elem_t rawhid_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&rawhid_get_obj)},
    {MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&rawhid_send_obj)},
};
MP_DEFINE_CONST_DICT(rawhid_globals, rawhid_globals_table);

const mp_obj_module_t matrixos_hid_rawhid_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&rawhid_globals,
};

static const mp_rom_map_elem_t hid_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&hid_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&hid_reset_obj)},
    {MP_ROM_QSTR(MP_QSTR_ready), MP_ROM_PTR(&hid_ready_obj)},
    {MP_ROM_QSTR(MP_QSTR_Keyboard), MP_ROM_PTR(&matrixos_hid_keyboard_module)},
    {MP_ROM_QSTR(MP_QSTR_Gamepad), MP_ROM_PTR(&matrixos_hid_gamepad_module)},
    {MP_ROM_QSTR(MP_QSTR_RawHID), MP_ROM_PTR(&matrixos_hid_rawhid_module)},
};
MP_DEFINE_CONST_DICT(hid_globals, hid_globals_table);
} // namespace

extern const mp_obj_module_t matrixos_hid_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&hid_globals,
};
