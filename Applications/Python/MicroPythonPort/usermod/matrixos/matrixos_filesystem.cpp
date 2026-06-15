// Exposes MatrixOS filesystem helpers and lightweight file operations to MicroPython.
#include "matrixos_common.h"

extern "C" {
#include "py/obj.h"
#include "py/runtime.h"
}

using namespace MatrixOSPython;

namespace
{
string ObjectToString(mp_obj_t obj) {
  size_t length = 0;
  const char* text = mp_obj_str_get_data(obj, &length);
  return string(text, length);
}

mp_obj_t filesystem_available() {
  return mp_obj_new_bool(MatrixOS::FileSystem::Available());
}
MP_DEFINE_CONST_FUN_OBJ_0(filesystem_available_obj, filesystem_available);

mp_obj_t filesystem_exists(mp_obj_t pathObj) {
  return mp_obj_new_bool(MatrixOS::FileSystem::Exists(ObjectToString(pathObj)));
}
MP_DEFINE_CONST_FUN_OBJ_1(filesystem_exists_obj, filesystem_exists);

mp_obj_t filesystem_mkdir(mp_obj_t pathObj) {
  return mp_obj_new_bool(MatrixOS::FileSystem::MakeDir(ObjectToString(pathObj)));
}
MP_DEFINE_CONST_FUN_OBJ_1(filesystem_mkdir_obj, filesystem_mkdir);

mp_obj_t filesystem_remove(mp_obj_t pathObj) {
  return mp_obj_new_bool(MatrixOS::FileSystem::Remove(ObjectToString(pathObj)));
}
MP_DEFINE_CONST_FUN_OBJ_1(filesystem_remove_obj, filesystem_remove);

mp_obj_t filesystem_rmdir(mp_obj_t pathObj) {
  return mp_obj_new_bool(MatrixOS::FileSystem::RemoveDir(ObjectToString(pathObj)));
}
MP_DEFINE_CONST_FUN_OBJ_1(filesystem_rmdir_obj, filesystem_rmdir);

mp_obj_t filesystem_rename(mp_obj_t fromObj, mp_obj_t toObj) {
  return mp_obj_new_bool(MatrixOS::FileSystem::Rename(ObjectToString(fromObj), ObjectToString(toObj)));
}
MP_DEFINE_CONST_FUN_OBJ_2(filesystem_rename_obj, filesystem_rename);

mp_obj_t filesystem_list_dir(size_t argc, const mp_obj_t* args) {
  string path = argc > 0 ? ObjectToString(args[0]) : "/";
  vector<string> entries = MatrixOS::FileSystem::ListDir(path);
  mp_obj_t list = mp_obj_new_list(0, nullptr);
  for (const string& entry : entries)
  {
    mp_obj_list_append(list, mp_obj_new_str(entry.c_str(), entry.size()));
  }
  return list;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(filesystem_list_dir_obj, 0, 1, filesystem_list_dir);

mp_obj_t filesystem_read_bytes(mp_obj_t pathObj) {
  string path = ObjectToString(pathObj);
  if (!MatrixOS::FileSystem::Exists(path))
  {
    return mp_const_none;
  }

  File file = MatrixOS::FileSystem::Open(path, "rb");
  size_t size = file.Size();
  vstr_t vstr;
  vstr_init_len(&vstr, size);
  size_t bytesRead = size > 0 ? file.Read(vstr.buf, size) : 0;
  file.Close();
  vstr.len = bytesRead;
  return mp_obj_new_bytes_from_vstr(&vstr);
}
MP_DEFINE_CONST_FUN_OBJ_1(filesystem_read_bytes_obj, filesystem_read_bytes);

mp_obj_t filesystem_write_bytes(mp_obj_t pathObj, mp_obj_t dataObj) {
  mp_buffer_info_t buffer;
  mp_get_buffer_raise(dataObj, &buffer, MP_BUFFER_READ);

  File file = MatrixOS::FileSystem::Open(ObjectToString(pathObj), "wb");
  size_t bytesWritten = buffer.len > 0 ? file.Write(buffer.buf, buffer.len) : 0;
  bool ok = (bytesWritten == buffer.len);
  if (ok)
  {
    ok = file.Flush();
  }
  file.Close();
  return mp_obj_new_bool(ok);
}
MP_DEFINE_CONST_FUN_OBJ_2(filesystem_write_bytes_obj, filesystem_write_bytes);

mp_obj_t filesystem_read_text(size_t argc, const mp_obj_t* args) {
  (void)argc;
  string path = ObjectToString(args[0]);
  if (!MatrixOS::FileSystem::Exists(path))
  {
    return mp_const_none;
  }

  File file = MatrixOS::FileSystem::Open(path, "rb");
  size_t size = file.Size();
  vstr_t vstr;
  vstr_init_len(&vstr, size);
  size_t bytesRead = size > 0 ? file.Read(vstr.buf, size) : 0;
  file.Close();
  vstr.len = bytesRead;
  return mp_obj_new_str_from_vstr(&vstr);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(filesystem_read_text_obj, 1, 2, filesystem_read_text);

mp_obj_t filesystem_write_text(size_t argc, const mp_obj_t* args) {
  (void)argc;
  return filesystem_write_bytes(args[0], args[1]);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(filesystem_write_text_obj, 2, 3, filesystem_write_text);

static const mp_rom_map_elem_t filesystem_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_available), MP_ROM_PTR(&filesystem_available_obj)},
    {MP_ROM_QSTR(MP_QSTR_exists), MP_ROM_PTR(&filesystem_exists_obj)},
    {MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&filesystem_mkdir_obj)},
    {MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&filesystem_remove_obj)},
    {MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&filesystem_rmdir_obj)},
    {MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&filesystem_rename_obj)},
    {MP_ROM_QSTR(MP_QSTR_list_dir), MP_ROM_PTR(&filesystem_list_dir_obj)},
    {MP_ROM_QSTR(MP_QSTR_read_bytes), MP_ROM_PTR(&filesystem_read_bytes_obj)},
    {MP_ROM_QSTR(MP_QSTR_write_bytes), MP_ROM_PTR(&filesystem_write_bytes_obj)},
    {MP_ROM_QSTR(MP_QSTR_read_text), MP_ROM_PTR(&filesystem_read_text_obj)},
    {MP_ROM_QSTR(MP_QSTR_write_text), MP_ROM_PTR(&filesystem_write_text_obj)},
};
MP_DEFINE_CONST_DICT(filesystem_globals, filesystem_globals_table);
} // namespace

extern const mp_obj_module_t matrixos_filesystem_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&filesystem_globals,
};
