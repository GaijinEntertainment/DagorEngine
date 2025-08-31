//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <cstdio>

namespace spirv
{

struct DXCContext;

DXCContext *setupDXC(const char *path, const dag::Vector<String> &extra_params,
  int (*error_reporter)(const char *const, ...) = &printf);
void shutdownDXC(DXCContext *ctx);
const char *getDXCVerString(const DXCContext *ctx);

} // namespace spirv
