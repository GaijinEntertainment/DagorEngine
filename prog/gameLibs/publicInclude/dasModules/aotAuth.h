//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <EASTL/string.h>

extern void auth_get_country_code(eastl::string &country);

namespace bind_dascript
{

inline const char *get_country_code(das::Context *context, das::LineInfoArg *at)
{
  eastl::string country{};
  auth_get_country_code(country);
  return context->allocateString(country, at);
}

} // namespace bind_dascript
