// Provides shared MicroPython conversion and callback helpers for MatrixOS bindings.
#include "matrixos_common.h"

#include <cstring>

extern "C" {
#include "py/nlr.h"
#include "py/runtime.h"
}

namespace MatrixOSPython
{
uint32_t ColorToRgb(const Color& color) {
  return color.RGB();
}

mp_obj_t MakeTick(uint64_t value) {
  return MP_OBJ_NEW_SMALL_INT((mp_int_t)(value & 0x3FFFFFFF));
}

Color ObjectToColor(mp_obj_t colorObj) {
  if (mp_obj_is_int(colorObj))
  {
    return Color((uint32_t)mp_obj_get_int_truncated(colorObj));
  }

  mp_obj_t* rgbTuple = nullptr;
  size_t tupleLength = 0;
  mp_obj_get_array(colorObj, &tupleLength, &rgbTuple);
  if (tupleLength < 3 || tupleLength > 4)
  {
    mp_raise_ValueError(MP_ERROR_TEXT("color must be int or (r, g, b[, w])"));
  }

  uint8_t r = (uint8_t)mp_obj_get_int(rgbTuple[0]);
  uint8_t g = (uint8_t)mp_obj_get_int(rgbTuple[1]);
  uint8_t b = (uint8_t)mp_obj_get_int(rgbTuple[2]);
  uint8_t w = tupleLength == 4 ? (uint8_t)mp_obj_get_int(rgbTuple[3]) : 0;
  return Color(r, g, b, w);
}

Dimension ObjectToDimension(mp_obj_t dimensionObj) {
  mp_obj_t* tuple = nullptr;
  size_t length = 0;
  mp_obj_get_array(dimensionObj, &length, &tuple);
  if (length != 2)
  {
    mp_raise_ValueError(MP_ERROR_TEXT("dimension must be (x, y)"));
  }
  return Dimension((uint16_t)mp_obj_get_int(tuple[0]), (uint16_t)mp_obj_get_int(tuple[1]));
}

Point ObjectToPoint(mp_obj_t pointObj) {
  mp_obj_t* tuple = nullptr;
  size_t length = 0;
  mp_obj_get_array(pointObj, &length, &tuple);
  if (length != 2)
  {
    mp_raise_ValueError(MP_ERROR_TEXT("point must be (x, y)"));
  }
  return Point((int16_t)mp_obj_get_int(tuple[0]), (int16_t)mp_obj_get_int(tuple[1]));
}

InputId ObjectToInputId(mp_obj_t inputIdObj) {
  mp_obj_t* idTuple = nullptr;
  size_t tupleLength = 0;
  mp_obj_get_array(inputIdObj, &tupleLength, &idTuple);
  if (tupleLength != 2)
  {
    mp_raise_ValueError(MP_ERROR_TEXT("input id must be (cluster_id, member_id)"));
  }

  return InputId{
      (uint8_t)mp_obj_get_int(idTuple[0]),
      (uint16_t)mp_obj_get_int(idTuple[1]),
  };
}

uint32_t ObjectToHash(mp_obj_t hashObj) {
  if (mp_obj_is_int(hashObj))
  {
    return (uint32_t)mp_obj_get_int_truncated(hashObj);
  }

  size_t length = 0;
  const char* text = mp_obj_str_get_data(hashObj, &length);
  return StringHash(string(text, length));
}

mp_obj_t MakeInputId(const InputId& id) {
  mp_obj_t tuple[2] = {
      mp_obj_new_int(id.clusterId),
      mp_obj_new_int(id.memberId),
  };
  return mp_obj_new_tuple(2, tuple);
}

mp_obj_t MakeInputClassName(InputClass inputClass) {
  const char* name = "unknown";
  switch (inputClass)
  {
  case InputClass::Keypad:
    name = "keypad";
    break;
  case InputClass::Fader:
    name = "fader";
    break;
  case InputClass::Encoder:
    name = "encoder";
    break;
  case InputClass::TouchArea:
    name = "touch_area";
    break;
  case InputClass::Gyro:
    name = "gyro";
    break;
  case InputClass::Accelerometer:
    name = "accelerometer";
    break;
  case InputClass::Temperature:
    name = "temperature";
    break;
  case InputClass::Battery:
    name = "battery";
    break;
  case InputClass::Generic:
    name = "generic";
    break;
  case InputClass::Unknown:
  default:
    break;
  }
  return mp_obj_new_str(name, strlen(name));
}

mp_obj_t MakeInputClusterShapeName(InputClusterShape shape) {
  const char* name = "scalar";
  switch (shape)
  {
  case InputClusterShape::Linear1D:
    name = "linear_1d";
    break;
  case InputClusterShape::Grid2D:
    name = "grid_2d";
    break;
  case InputClusterShape::Area2D:
    name = "area_2d";
    break;
  case InputClusterShape::Scalar:
  default:
    break;
  }
  return mp_obj_new_str(name, strlen(name));
}

mp_obj_t MakePartition(const LEDPartition& partition) {
  mp_obj_t dict = mp_obj_new_dict(5);
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_name), mp_obj_new_str(partition.name.c_str(), partition.name.size()));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_start), mp_obj_new_int(partition.start));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_size), mp_obj_new_int(partition.size));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_type), mp_obj_new_int(partition.type));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_default_multiplier), mp_obj_new_int((int)(partition.default_multiplier * 1000)));
  return dict;
}

mp_obj_t MakePoint(const Point& point) {
  mp_obj_t tuple[2] = {
      mp_obj_new_int(point.x),
      mp_obj_new_int(point.y),
  };
  return mp_obj_new_tuple(2, tuple);
}

mp_obj_t MakeKeypadInfo(const KeypadInfo& keypad) {
  mp_obj_t dict = mp_obj_new_dict(7);
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_state), mp_obj_new_int((int)keypad.state));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_pressed), mp_obj_new_bool(keypad.state == KeypadState::Pressed));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_released), mp_obj_new_bool(keypad.state == KeypadState::Released));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_aftertouch), mp_obj_new_bool(keypad.state == KeypadState::Aftertouch));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_hold), mp_obj_new_bool(keypad.Hold()));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_pressure), mp_obj_new_int(keypad.pressure.value));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_velocity), mp_obj_new_int(keypad.velocity.value));
  return dict;
}

void StoreKeypadInfo(mp_obj_t dict, const KeypadInfo& keypad) {
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_keypad), MakeKeypadInfo(keypad));
}

mp_obj_t MakeInputEvent(const InputEvent& event) {
  mp_obj_t dict = mp_obj_new_dict(5);
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_id), MakeInputId(event.id));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_input_class), mp_obj_new_int((int)event.inputClass));

  Point point;
  if (MatrixOS::Input::GetPosition(event.id, &point))
  {
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_point), MakePoint(point));
  }
  else
  {
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_point), mp_const_none);
  }

  if (event.inputClass == InputClass::Keypad)
  {
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_keypad), MakeKeypadInfo(event.keypad));
  }

  return dict;
}

mp_obj_t MakeInputCluster(const InputCluster& cluster) {
  mp_obj_t dict = mp_obj_new_dict(10);
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_id), mp_obj_new_int(cluster.clusterId));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_name), mp_obj_new_str(cluster.name.c_str(), cluster.name.size()));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_input_class), mp_obj_new_int((int)cluster.inputClass));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_input_class_name), MakeInputClassName(cluster.inputClass));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_shape), mp_obj_new_int((int)cluster.shape));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_shape_name), MakeInputClusterShapeName(cluster.shape));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_root_point), cluster.HasRootPoint() ? MakePoint(cluster.rootPoint) : mp_const_none);
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_dimension), MakePoint(Point(cluster.dimension.x, cluster.dimension.y)));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_input_count), mp_obj_new_int(cluster.inputCount));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_has_coordinates), mp_obj_new_bool(cluster.HasCoordinates()));
  return dict;
}

mp_obj_t MakeInputSnapshot(const InputSnapshot& snapshot) {
  mp_obj_t dict = mp_obj_new_dict(5);
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_id), MakeInputId(snapshot.id));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_input_class), mp_obj_new_int((int)snapshot.inputClass));

  Point point;
  if (MatrixOS::Input::GetPosition(snapshot.id, &point))
  {
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_point), MakePoint(point));
  }
  else
  {
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_point), mp_const_none);
  }

  if (snapshot.inputClass == InputClass::Keypad)
  {
    StoreKeypadInfo(dict, snapshot.keypad);
  }

  return dict;
}

mp_obj_t MakeKeypadCapabilities(const KeypadCapabilities& capabilities) {
  mp_obj_t dict = mp_obj_new_dict(4);
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_has_pressure), mp_obj_new_bool(capabilities.hasPressure));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_has_aftertouch), mp_obj_new_bool(capabilities.hasAftertouch));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_has_velocity), mp_obj_new_bool(capabilities.hasVelocity));
  mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_has_position), mp_obj_new_bool(capabilities.hasPosition));
  return dict;
}

mp_obj_t ProtectedCall(size_t argc, const mp_obj_t* args) {
  nlr_buf_t nlr;
  if (nlr_push(&nlr) == 0)
  {
    mp_obj_t result = mp_call_function_n_kw(args[0], argc - 1, 0, args + 1);
    nlr_pop();
    return result;
  }

  mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
  return mp_const_none;
}

Color CallColorCallback(mp_obj_t callback, Color fallback) {
  if (callback == mp_const_none)
  {
    return fallback;
  }

  mp_obj_t args[] = {callback};
  mp_obj_t result = ProtectedCall(1, args);
  if (result == mp_const_none)
  {
    return fallback;
  }
  return ObjectToColor(result);
}

Color CallIndexedColorCallback(mp_obj_t callback, uint16_t index, Color fallback) {
  if (callback == mp_const_none)
  {
    return fallback;
  }

  mp_obj_t args[] = {callback, mp_obj_new_int(index)};
  mp_obj_t result = ProtectedCall(2, args);
  if (result == mp_const_none)
  {
    return fallback;
  }
  return ObjectToColor(result);
}

int32_t CallIntCallback(mp_obj_t callback, int32_t fallback) {
  if (callback == mp_const_none)
  {
    return fallback;
  }

  mp_obj_t args[] = {callback};
  mp_obj_t result = ProtectedCall(1, args);
  return result == mp_const_none ? fallback : mp_obj_get_int(result);
}

string CallIndexedStringCallback(mp_obj_t callback, uint16_t index, const string& fallback) {
  if (callback == mp_const_none)
  {
    return fallback;
  }

  mp_obj_t args[] = {callback, mp_obj_new_int(index)};
  mp_obj_t result = ProtectedCall(2, args);
  if (result == mp_const_none)
  {
    return fallback;
  }

  size_t length = 0;
  const char* text = mp_obj_str_get_data(result, &length);
  return string(text, length);
}
} // namespace MatrixOSPython
