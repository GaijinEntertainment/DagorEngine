//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daBfg/nodeHandle.h>
#include <daScript/daScript.h>

MAKE_TYPE_FACTORY(NodeHandle, dabfg::NodeHandle);

namespace das
{

template <>
struct das_auto_cast_move<dabfg::NodeHandle>
{
  __forceinline static dabfg::NodeHandle cast(dabfg::NodeHandle &&expr) { return eastl::move(expr); }
};

} // namespace das