#pragma once

#include "Framework.h"

#include "FatFS/ff.h"

namespace MatrixOS::File
{
  typedef FIL File;

  void Init();
}