// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <breakpad/binder.h>
#include <EASTL/utility.h>

namespace breakpad
{

struct Context
{
  Context(Product &&prod, Configuration &&cfg) : product(eastl::move(prod)), configuration(eastl::move(cfg)) {}

  Configuration configuration;
  Product product;
  CrashInfo crash;
}; // struct Context

} // namespace breakpad
