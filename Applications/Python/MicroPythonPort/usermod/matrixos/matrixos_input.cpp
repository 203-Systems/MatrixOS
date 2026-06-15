// Exposes MatrixOS input events, clusters, and keypad state to MicroPython.
#include "matrixos_common.h"
#include "matrixos_modules.h"

extern "C" {
#include "py/objlist.h"
}

using namespace MatrixOSPython;

namespace
{
const InputCluster* ResolveCluster(mp_obj_t clusterObj) {
  if (mp_obj_is_int(clusterObj))
  {
    return MatrixOS::Input::GetCluster((uint8_t)mp_obj_get_int(clusterObj));
  }

  size_t nameLength = 0;
  const char* nameData = mp_obj_str_get_data(clusterObj, &nameLength);
  string name(nameData, nameLength);
  for (const InputCluster& cluster : MatrixOS::Input::GetClusters())
  {
    if (cluster.name == name)
    {
      return &cluster;
    }
  }

  return nullptr;
}
} // namespace

mp_obj_t input_get_event(size_t argc, const mp_obj_t* args) {
  uint32_t timeoutMs = argc > 0 ? (uint32_t)mp_obj_get_int(args[0]) : 0;
  InputEvent event;
  if (!MatrixOS::Input::Get(&event, timeoutMs))
  {
    return mp_const_none;
  }

  return MakeInputEvent(event);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(input_get_event_obj, 0, 1, input_get_event);

mp_obj_t input_clear() {
  MatrixOS::Input::ClearInputBuffer();
  Device::Input::SuppressActiveInputs();
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(input_clear_obj, input_clear);

mp_obj_t input_function_key() {
  return MakeInputId(InputId::FunctionKey());
}
MP_DEFINE_CONST_FUN_OBJ_0(input_function_key_obj, input_function_key);

mp_obj_t input_try_get_point(mp_obj_t inputIdObj) {
  InputId id = ObjectToInputId(inputIdObj);
  Point point;
  if (!MatrixOS::Input::GetPosition(id, &point))
  {
    return mp_const_none;
  }
  return MakePoint(point);
}
MP_DEFINE_CONST_FUN_OBJ_1(input_try_get_point_obj, input_try_get_point);

mp_obj_t input_clusters() {
  const vector<InputCluster>& clusters = MatrixOS::Input::GetClusters();
  mp_obj_t list = mp_obj_new_list(0, nullptr);
  for (const InputCluster& cluster : clusters)
  {
    mp_obj_list_append(list, MakeInputCluster(cluster));
  }
  return list;
}
MP_DEFINE_CONST_FUN_OBJ_0(input_clusters_obj, input_clusters);

mp_obj_t input_get_cluster(mp_obj_t clusterIdObj) {
  const InputCluster* cluster = ResolveCluster(clusterIdObj);
  if (cluster == nullptr)
  {
    return mp_const_none;
  }
  return MakeInputCluster(*cluster);
}
MP_DEFINE_CONST_FUN_OBJ_1(input_get_cluster_obj, input_get_cluster);

mp_obj_t input_primary_grid_cluster() {
  const InputCluster* cluster = MatrixOS::Input::GetPrimaryGridCluster();
  if (cluster == nullptr)
  {
    return mp_const_none;
  }
  return MakeInputCluster(*cluster);
}
MP_DEFINE_CONST_FUN_OBJ_0(input_primary_grid_cluster_obj, input_primary_grid_cluster);

mp_obj_t input_get_state(mp_obj_t inputIdObj) {
  InputSnapshot snapshot;
  InputId id = ObjectToInputId(inputIdObj);
  if (!MatrixOS::Input::GetState(id, &snapshot))
  {
    return mp_const_none;
  }
  return MakeInputSnapshot(snapshot);
}
MP_DEFINE_CONST_FUN_OBJ_1(input_get_state_obj, input_get_state);

mp_obj_t input_get_inputs_at(mp_obj_t pointObj) {
  vector<InputId> ids;
  MatrixOS::Input::GetInputsAt(ObjectToPoint(pointObj), &ids);
  mp_obj_t list = mp_obj_new_list(0, nullptr);
  for (const InputId& id : ids)
  {
    mp_obj_list_append(list, MakeInputId(id));
  }
  return list;
}
MP_DEFINE_CONST_FUN_OBJ_1(input_get_inputs_at_obj, input_get_inputs_at);

mp_obj_t input_get_input_at(mp_obj_t clusterIdObj, mp_obj_t pointObj) {
  const InputCluster* cluster = ResolveCluster(clusterIdObj);
  if (cluster == nullptr)
  {
    return mp_const_none;
  }

  InputId id;
  if (!MatrixOS::Input::GetInputAt(cluster->clusterId, ObjectToPoint(pointObj), &id))
  {
    return mp_const_none;
  }
  return MakeInputId(id);
}
MP_DEFINE_CONST_FUN_OBJ_2(input_get_input_at_obj, input_get_input_at);

mp_obj_t input_get_keypad_capabilities(mp_obj_t clusterIdObj) {
  const InputCluster* cluster = ResolveCluster(clusterIdObj);
  if (cluster == nullptr)
  {
    return mp_const_none;
  }

  KeypadCapabilities capabilities;
  if (!MatrixOS::Input::GetKeypadCapabilities(cluster->clusterId, &capabilities))
  {
    return mp_const_none;
  }
  return MakeKeypadCapabilities(capabilities);
}
MP_DEFINE_CONST_FUN_OBJ_1(input_get_keypad_capabilities_obj, input_get_keypad_capabilities);

static const mp_rom_map_elem_t input_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_get_event), MP_ROM_PTR(&input_get_event_obj)},
    {MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&input_clear_obj)},
    {MP_ROM_QSTR(MP_QSTR_function_key), MP_ROM_PTR(&input_function_key_obj)},
    {MP_ROM_QSTR(MP_QSTR_try_get_point), MP_ROM_PTR(&input_try_get_point_obj)},
    {MP_ROM_QSTR(MP_QSTR_clusters), MP_ROM_PTR(&input_clusters_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_cluster), MP_ROM_PTR(&input_get_cluster_obj)},
    {MP_ROM_QSTR(MP_QSTR_primary_grid_cluster), MP_ROM_PTR(&input_primary_grid_cluster_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_state), MP_ROM_PTR(&input_get_state_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_inputs_at), MP_ROM_PTR(&input_get_inputs_at_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_input_at), MP_ROM_PTR(&input_get_input_at_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_keypad_capabilities), MP_ROM_PTR(&input_get_keypad_capabilities_obj)},
    {MP_ROM_QSTR(MP_QSTR_STATE_IDLE), MP_ROM_INT((int)KeypadState::Idle)},
    {MP_ROM_QSTR(MP_QSTR_STATE_ACTIVATED), MP_ROM_INT((int)KeypadState::Activated)},
    {MP_ROM_QSTR(MP_QSTR_STATE_PRESSED), MP_ROM_INT((int)KeypadState::Pressed)},
    {MP_ROM_QSTR(MP_QSTR_STATE_HOLD), MP_ROM_INT((int)KeypadState::Hold)},
    {MP_ROM_QSTR(MP_QSTR_STATE_AFTERTOUCH), MP_ROM_INT((int)KeypadState::Aftertouch)},
    {MP_ROM_QSTR(MP_QSTR_STATE_RELEASED), MP_ROM_INT((int)KeypadState::Released)},
};
MP_DEFINE_CONST_DICT(input_globals, input_globals_table);

extern const mp_obj_module_t matrixos_input_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&input_globals,
};
