#include "MatrixOS.h"

#if MATRIXOS_WEB
#include "HostIO.h"
#endif

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <string>

extern "C" {
#include "py/builtin.h"
#include "py/lexer.h"
#include "py/misc.h"
#include "py/obj.h"
#include "py/objlist.h"
#include "py/reader.h"
#include "py/runtime.h"
#include "py/stream.h"
}

namespace
{
std::string currentScriptDirectory;

std::string NormalizeSeparators(std::string path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  return path;
}

bool HasScheme(const std::string& path) {
  return path.find(':') != std::string::npos;
}

std::string DirectoryName(const std::string& path) {
  size_t slash = path.find_last_of('/');
  if (slash == std::string::npos)
  {
    return "";
  }
  return path.substr(0, slash);
}

std::string ResolvePath(const std::string& rawPath) {
  std::string path = NormalizeSeparators(rawPath);
  if (path.empty() || path[0] == '/' || HasScheme(path) || currentScriptDirectory.empty())
  {
    return path;
  }
  return currentScriptDirectory + "/" + path;
}

bool ReadFileContents(const std::string& rawPath, std::string* output) {
  if (output == nullptr)
  {
    return false;
  }

  std::string path = ResolvePath(rawPath);

#if MATRIXOS_WEB
  if (MystrixSim::HostIO::GetPythonScript(path, output))
  {
    return true;
  }
#endif

#if DEVICE_STORAGE == 1
  File file = MatrixOS::FileSystem::Open(path, "rb");
  if (!file.Available() && file.Size() == 0)
  {
    return false;
  }

  size_t size = file.Size();
  output->resize(size);
  size_t bytesRead = size > 0 ? file.Read(output->data(), size) : 0;
  output->resize(bytesRead);
  file.Close();
  return bytesRead > 0 || size == 0;
#else
  return false;
#endif
}

bool FileExists(const std::string& rawPath) {
  std::string path = ResolvePath(rawPath);

#if MATRIXOS_WEB
  if (MystrixSim::HostIO::HasPythonScript(path))
  {
    return true;
  }
#endif

#if DEVICE_STORAGE == 1
  return MatrixOS::FileSystem::Exists(path);
#else
  return false;
#endif
}

bool DirectoryExists(const std::string& rawPath) {
  std::string path = ResolvePath(rawPath);

#if MATRIXOS_WEB
  if (MystrixSim::HostIO::HasPythonDirectory(path))
  {
    return true;
  }
#endif

#if DEVICE_STORAGE == 1
  return MatrixOS::FileSystem::Exists(path);
#else
  return false;
#endif
}

enum class FileBackend : uint8_t {
  MatrixOS,
  Host,
};

typedef struct _matrixos_file_obj_t {
  mp_obj_base_t base;
  FileBackend backend;
  File* file;
  std::string* path;
  std::string* contents;
  size_t position;
  bool readable;
  bool writable;
  bool append;
  bool closed;
  bool text;
} matrixos_file_obj_t;

extern const mp_obj_type_t matrixos_file_type;

void CloseFile(matrixos_file_obj_t* self) {
  if (self == nullptr || self->closed)
  {
    return;
  }

  if (self->backend == FileBackend::Host)
  {
#if MATRIXOS_WEB
    if (self->writable && self->path != nullptr && self->contents != nullptr)
    {
      MystrixSim::HostIO::WritePythonScriptFile(*self->path, *self->contents);
    }
#endif
  }
  else if (self->file != nullptr)
  {
    if (self->writable)
    {
      self->file->Flush();
    }
    self->file->Close();
  }

  delete self->file;
  delete self->path;
  delete self->contents;
  self->file = nullptr;
  self->path = nullptr;
  self->contents = nullptr;
  self->closed = true;
}

mp_uint_t file_read(mp_obj_t obj, void* buffer, mp_uint_t size, int* errcode) {
  matrixos_file_obj_t* self = static_cast<matrixos_file_obj_t*>(MP_OBJ_TO_PTR(obj));
  if (self->closed || !self->readable)
  {
    *errcode = self->closed ? MP_EINVAL : MP_EACCES;
    return MP_STREAM_ERROR;
  }

  if (self->backend == FileBackend::Host)
  {
    size_t available = self->contents != nullptr && self->position < self->contents->size() ? self->contents->size() - self->position : 0;
    size_t bytesRead = std::min<size_t>(size, available);
    if (bytesRead > 0)
    {
      memcpy(buffer, self->contents->data() + self->position, bytesRead);
      self->position += bytesRead;
    }
    return bytesRead;
  }

  return self->file != nullptr ? self->file->Read(buffer, size) : 0;
}

mp_uint_t file_write(mp_obj_t obj, const void* buffer, mp_uint_t size, int* errcode) {
  matrixos_file_obj_t* self = static_cast<matrixos_file_obj_t*>(MP_OBJ_TO_PTR(obj));
  if (self->closed || !self->writable)
  {
    *errcode = self->closed ? MP_EINVAL : MP_EACCES;
    return MP_STREAM_ERROR;
  }

  if (self->backend == FileBackend::Host)
  {
    if (self->contents == nullptr)
    {
      *errcode = MP_EINVAL;
      return MP_STREAM_ERROR;
    }

    if (self->append)
    {
      self->position = self->contents->size();
    }
    if (self->position > self->contents->size())
    {
      self->contents->resize(self->position, '\0');
    }
    if (self->position + size > self->contents->size())
    {
      self->contents->resize(self->position + size);
    }
    memcpy(self->contents->data() + self->position, buffer, size);
    self->position += size;
    return size;
  }

  return self->file != nullptr ? self->file->Write(buffer, size) : 0;
}

mp_uint_t file_ioctl(mp_obj_t obj, mp_uint_t request, uintptr_t arg, int* errcode) {
  matrixos_file_obj_t* self = static_cast<matrixos_file_obj_t*>(MP_OBJ_TO_PTR(obj));
  if (self->closed && request != MP_STREAM_CLOSE)
  {
    *errcode = MP_EINVAL;
    return MP_STREAM_ERROR;
  }

  switch (request)
  {
    case MP_STREAM_FLUSH:
      if (self->backend == FileBackend::Host)
      {
#if MATRIXOS_WEB
        if (self->writable && self->path != nullptr && self->contents != nullptr)
        {
          MystrixSim::HostIO::WritePythonScriptFile(*self->path, *self->contents);
        }
#endif
        return 0;
      }
      return self->file != nullptr && self->file->Flush() ? 0 : MP_STREAM_ERROR;

    case MP_STREAM_SEEK:
    {
      mp_stream_seek_t* seek = reinterpret_cast<mp_stream_seek_t*>(arg);
      size_t base = 0;
      if (seek->whence == MP_SEEK_CUR)
      {
        base = self->backend == FileBackend::Host ? self->position : self->file->Position();
      }
      else if (seek->whence == MP_SEEK_END)
      {
        base = self->backend == FileBackend::Host ? self->contents->size() : self->file->Size();
      }

      mp_off_t target = seek->whence == MP_SEEK_SET ? seek->offset : static_cast<mp_off_t>(base) + seek->offset;
      if (target < 0)
      {
        *errcode = MP_EINVAL;
        return MP_STREAM_ERROR;
      }

      if (self->backend == FileBackend::Host)
      {
        self->position = static_cast<size_t>(target);
        seek->offset = target;
        return 0;
      }

      if (self->file != nullptr && self->file->Seek(static_cast<size_t>(target)))
      {
        seek->offset = static_cast<mp_off_t>(self->file->Position());
        return 0;
      }
      *errcode = MP_EINVAL;
      return MP_STREAM_ERROR;
    }

    case MP_STREAM_CLOSE:
      CloseFile(self);
      return 0;

    default:
      *errcode = MP_EINVAL;
      return MP_STREAM_ERROR;
  }
}

static const mp_stream_p_t file_stream_p = {
    .read = file_read,
    .write = file_write,
    .ioctl = file_ioctl,
    .is_text = true,
};

static const mp_rom_map_elem_t file_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj)},
    {MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj)},
    {MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj)},
    {MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj)},
    {MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj)},
    {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj)},
    {MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj)},
    {MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&mp_stream___exit___obj)},
};
MP_DEFINE_CONST_DICT(file_locals_dict, file_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    matrixos_file_type,
    MP_QSTR_File,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    protocol, &file_stream_p,
    locals_dict, &file_locals_dict);
} // namespace

extern "C" void matrixos_micropython_set_script_path(const char* sourceName) {
  currentScriptDirectory = sourceName == nullptr ? "" : DirectoryName(NormalizeSeparators(sourceName));

#if MICROPY_PY_SYS && MICROPY_PY_SYS_PATH
  if (!currentScriptDirectory.empty())
  {
    size_t pathCount = 0;
    mp_obj_t* pathItems = nullptr;
    mp_obj_get_array(mp_sys_path, &pathCount, &pathItems);
    for (size_t index = 0; index < pathCount; index++)
    {
      size_t itemLength = 0;
      const char* item = mp_obj_str_get_data(pathItems[index], &itemLength);
      if (currentScriptDirectory.size() == itemLength && memcmp(item, currentScriptDirectory.data(), itemLength) == 0)
      {
        return;
      }
    }
    mp_obj_list_append(mp_sys_path, mp_obj_new_str(currentScriptDirectory.c_str(), currentScriptDirectory.size()));
  }
#endif
}

extern "C" mp_import_stat_t mp_import_stat(const char* path) {
  if (path == nullptr)
  {
    return MP_IMPORT_STAT_NO_EXIST;
  }
  if (FileExists(path))
  {
    return MP_IMPORT_STAT_FILE;
  }
  if (DirectoryExists(path))
  {
    return MP_IMPORT_STAT_DIR;
  }
  return MP_IMPORT_STAT_NO_EXIST;
}

extern "C" void mp_reader_new_file(mp_reader_t* reader, qstr filename) {
  std::string contents;
  const char* path = qstr_str(filename);
  if (!ReadFileContents(path, &contents))
  {
    mp_raise_OSError_with_filename(MP_ENOENT, path);
  }

  char* data = m_new(char, contents.size());
  if (!contents.empty())
  {
    memcpy(data, contents.data(), contents.size());
  }
  mp_reader_new_mem(reader, reinterpret_cast<const byte*>(data), contents.size(), contents.size());
}

extern "C" mp_lexer_t* mp_lexer_new_from_file(qstr filename) {
  mp_reader_t reader;
  mp_reader_new_file(&reader, filename);
  return mp_lexer_new(filename, reader);
}

extern "C" mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
  if (n_args < 1)
  {
    mp_raise_TypeError(MP_ERROR_TEXT("expected path"));
  }

  std::string path = ResolvePath(mp_obj_str_get_str(args[0]));
  std::string mode = n_args >= 2 ? mp_obj_str_get_str(args[1]) : "r";
  if (kwargs != nullptr && kwargs->used > 0)
  {
    mp_map_elem_t* modeElement = mp_map_lookup(kwargs, MP_OBJ_NEW_QSTR(MP_QSTR_mode), MP_MAP_LOOKUP);
    if (modeElement != nullptr)
    {
      mode = mp_obj_str_get_str(modeElement->value);
    }
    if (kwargs->used > (modeElement == nullptr ? 0u : 1u))
    {
      mp_raise_TypeError(MP_ERROR_TEXT("unexpected keyword argument"));
    }
  }

  bool read = mode.find('r') != std::string::npos || mode.find('+') != std::string::npos;
  bool write = mode.find('w') != std::string::npos || mode.find('a') != std::string::npos || mode.find('+') != std::string::npos;
  bool append = mode.find('a') != std::string::npos;
  bool truncate = mode.find('w') != std::string::npos;
  bool text = mode.find('b') == std::string::npos;

  matrixos_file_obj_t* self = mp_obj_malloc(matrixos_file_obj_t, &matrixos_file_type);
  self->backend = FileBackend::MatrixOS;
  self->file = nullptr;
  self->path = new std::string(path);
  self->contents = nullptr;
  self->position = 0;
  self->readable = read;
  self->writable = write;
  self->append = append;
  self->closed = false;
  self->text = text;

#if MATRIXOS_WEB
  if (path.rfind("host:/python/", 0) == 0)
  {
    std::string contents;
    if (!truncate)
    {
      MystrixSim::HostIO::GetPythonScript(path, &contents);
    }
    else if (!write)
    {
      mp_raise_OSError_with_filename(MP_EACCES, path.c_str());
    }
    if (read && !write && !MystrixSim::HostIO::HasPythonScript(path))
    {
      delete self->path;
      m_del_obj(matrixos_file_obj_t, self);
      mp_raise_OSError_with_filename(MP_ENOENT, path.c_str());
    }

    self->backend = FileBackend::Host;
    self->contents = new std::string(contents);
    self->position = append ? self->contents->size() : 0;
    return MP_OBJ_FROM_PTR(self);
  }
#endif

#if DEVICE_STORAGE == 1
  self->file = new File(MatrixOS::FileSystem::Open(path, mode));
  if (!self->file->Available() && self->file->Size() == 0 && read && !write)
  {
    CloseFile(self);
    m_del_obj(matrixos_file_obj_t, self);
    mp_raise_OSError_with_filename(MP_ENOENT, path.c_str());
  }
  return MP_OBJ_FROM_PTR(self);
#else
  CloseFile(self);
  m_del_obj(matrixos_file_obj_t, self);
  mp_raise_OSError_with_filename(MP_ENOENT, path.c_str());
#endif
}

extern "C" MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);
