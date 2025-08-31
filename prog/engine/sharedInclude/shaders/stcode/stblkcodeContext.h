// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <cstdint>

namespace stcode::cpp
{

struct StblkcodeContext
{
  void *bindlessContext; // BindlessStblkcodeContext *
  void *blockDest;       // ShaderStateBlock *
  const void *vars;
  const int32_t codeId;
  int32_t pad_;
};

} // namespace stcode::cpp