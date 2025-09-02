// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <landMesh/heightmapQuery.h>

MAKE_TYPE_FACTORY(HeightmapQueryResultWrapper, ::HeightmapQueryResultWrapper);

namespace bind_dascript
{
inline int heightmap_query_start(Point3 pos, Point3 grav_dir) { return heightmap_query::query(pos, grav_dir); }
inline GpuReadbackResultState heightmap_query_value(int query_id, HeightmapQueryResultWrapper &value)
{
  return heightmap_query::get_query_result(query_id, value);
}
} // namespace bind_dascript