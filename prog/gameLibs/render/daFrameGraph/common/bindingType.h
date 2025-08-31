// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>

namespace dafg
{

enum class BindingType : uint8_t
{
  ShaderVar,
  ViewMatrix,
  ProjMatrix,
  Invalid,
};

}
