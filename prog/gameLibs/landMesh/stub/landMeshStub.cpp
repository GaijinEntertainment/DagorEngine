// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/biomeQuery.h>
namespace biome_query
{
int query(const Point3 &, const float) { G_ASSERT_RETURN(false, {}); }
GpuReadbackResultState get_query_result(int, BiomeQueryResult &) { G_ASSERT_RETURN(false, {}); }
int get_biome_group_id(const char *) { G_ASSERT_RETURN(false, {}); }
const char *get_biome_group_name(int) { G_ASSERT_RETURN(false, {}); }
int get_num_biome_groups() { G_ASSERT_RETURN(false, 0); }
} // namespace biome_query

#include <landMesh/lmeshHoles.h>
bool LandMeshHolesCell::check(const Point2 &, const HeightmapHandler *) const { return false; }

#include <landMesh/heightmapQuery.h>
int heightmap_query::query(const Point3 &, const Point3 &) { G_ASSERT_RETURN(false, -1); }
GpuReadbackResultState heightmap_query::get_query_result(int, HeightmapQueryResult &)
{
  G_ASSERT_RETURN(false, GpuReadbackResultState::SYSTEM_NOT_INITIALIZED);
}
GpuReadbackResultState heightmap_query::get_query_result(int, HeightmapQueryResultWrapper &)
{
  G_ASSERT_RETURN(false, GpuReadbackResultState::SYSTEM_NOT_INITIALIZED);
}
