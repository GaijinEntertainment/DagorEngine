//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/daFrameGraph/nodeHandle.h>
#include <daScript/daScript.h>

MAKE_TYPE_FACTORY(NodeHandle, dafg::NodeHandle);

namespace das
{

template <>
struct das_auto_cast_move<dafg::NodeHandle>
{
  __forceinline static dafg::NodeHandle cast(dafg::NodeHandle &&expr) { return eastl::move(expr); }
};

} // namespace das