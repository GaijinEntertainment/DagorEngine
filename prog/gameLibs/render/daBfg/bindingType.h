#pragma once

#include <util/dag_stdint.h>

namespace dabfg
{

enum class BindingType : uint8_t
{
  ShaderVar,
  ViewMatrix,
  ProjMatrix,
  Invalid,
};

}
