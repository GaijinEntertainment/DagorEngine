//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <EASTL/string.h>
#include <cstdint>

namespace auth
{
eastl::string get_country_code();
}


namespace bind_dascript
{

inline const char *get_country_code(das::Context *context, das::LineInfoArg *at)
{
  eastl::string country{auth::get_country_code()};
  return context->allocateString(country, at);
}

} // namespace bind_dascript
