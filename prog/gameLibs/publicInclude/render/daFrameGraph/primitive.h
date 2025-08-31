//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_consts_base.h>
#include <EASTL/utility.h>

namespace dafg
{

enum class DrawPrimitive : uint8_t
{
  POINT_LIST = PRIM_POINTLIST,
  LINE_LIST = PRIM_LINELIST,
  LINE_STRIP = PRIM_LINESTRIP,
  TRIANGLE_LIST = PRIM_TRILIST,
  TRIANGLE_STRIP = PRIM_TRISTRIP,
  TRIANGLE_FAN = PRIM_TRIFAN,
};

inline int get_d3d_primitive(DrawPrimitive primitive) { return eastl::to_underlying(primitive); }

} // namespace dafg