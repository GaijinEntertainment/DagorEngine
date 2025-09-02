// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shTargetStorage.h"
#include "shcode.h"

static void clear_ptrs(auto &cont)
{
  for (auto *p : cont)
    if (p)
      delete p;
}

ShaderTargetStorage::~ShaderTargetStorage()
{
  clear_ptrs(shaderClass);
  clear_ptrs(shadersCompProg);
}