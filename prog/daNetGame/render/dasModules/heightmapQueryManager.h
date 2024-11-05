// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <landMesh/heightmapQuery.h>

MAKE_TYPE_FACTORY(HeightmapQueryResult, ::HeightmapQueryResult);

namespace bind_dascript
{
inline int heightmap_query_start(Point2 pos) { return heightmap_query::query(pos); }
inline GpuReadbackResultState heightmap_query_value(int query_id, HeightmapQueryResult &value)
{
  return heightmap_query::get_query_result(query_id, value);
}
} // namespace bind_dascript