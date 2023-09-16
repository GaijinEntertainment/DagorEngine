//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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

inline const char *get_country_code(das::Context *context)
{
  eastl::string country{auth::get_country_code()};
  return context->stringHeap->allocateString(country.begin(), uint32_t(country.length()));
}

} // namespace bind_dascript
