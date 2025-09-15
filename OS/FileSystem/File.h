#pragma once

#include "Framework.h"
#include "ff.h"

namespace MatrixOS::File
{
  typedef FIL File;

  void Init();
  bool Available();
  File* Open(string path, uint8_t mode);
  bool Close(File* file);
  size_t Read(File* file, void* buffer, size_t length);
  size_t Write(File* file, const void* buffer, size_t length);
  bool Exists(string path);
  bool Delete(string path);
  bool CreateDir(string path);
  vector<string> ListDir(string path);
}