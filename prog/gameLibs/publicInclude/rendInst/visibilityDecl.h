//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_bitFlagsMask.h>


struct RiGenVisibility;

namespace rendinst
{

enum class VisibilityRenderingFlag
{
  None = 0,
  Static = 1 << 0,
  Dynamic = 1 << 1,
  All = Static | Dynamic,
};
using VisibilityRenderingFlags = BitFlagsMask<VisibilityRenderingFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(VisibilityRenderingFlag);

} // namespace rendinst
