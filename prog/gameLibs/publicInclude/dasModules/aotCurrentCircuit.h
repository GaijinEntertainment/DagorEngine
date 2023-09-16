//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
#include <EASTL/string_view.h>
#include <cstdint>

namespace circuit
{
eastl::string_view get_name();
}

namespace bind_dascript
{

inline const char *get_circuit_name(das::Context *context)
{
  eastl::string_view name{circuit::get_name()};
  return context->stringHeap->allocateString(name.begin(), uint32_t(name.length()));
}

} // namespace bind_dascript
