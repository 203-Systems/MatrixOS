#pragma once

#include "Framework.h"
#include "File.h"

namespace MatrixOS::FileSystem
{
  // System initialization
  void Init();

  // Filesystem operations
  bool Available(void);
  bool Exists(const string& path);
  bool MakeDir(const string& path);
  File Open(const string& path, const string& mode);
  bool Remove(const string& path);
  bool RemoveDir(const string& path);
  vector<string> ListDir(const string& path);
}