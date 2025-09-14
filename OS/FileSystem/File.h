#pragma once

#include "Framework.h"
#include "ff.h"

namespace MatrixOS::File
{
  typedef FIL Handle;

  bool Available();
  Handle* Open(string path, uint8_t mode);
  bool Close(Handle* file);
  size_t Read(Handle* file, void* buffer, size_t length);
  size_t Write(Handle* file, const void* buffer, size_t length);
  bool Exists(string path);
  bool Delete(string path);
  bool CreateDir(string path);
  vector<string> ListDir(string path);
}