//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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

inline const char *get_circuit_name(das::Context *context, das::LineInfoArg *at)
{
  eastl::string_view name{circuit::get_name()};
  return context->allocateString(name.begin(), uint32_t(name.length()), at);
}

} // namespace bind_dascript
