// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>
#include <gameRes/dag_collisionResource.h>
#include <heightmap/heightmapHandler.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IPoint2.h>
#include <perfMon/dag_statDrv.h>
#include <render/world/occlusionLandMeshManager.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstGen.h>
#include <scene/dag_occlusion.h>
#include <util/dag_convar.h>

CONSOLE_INT_VAL("render", sw_raster_partition_size, 1000, 100, 10000);

void add_sw_rasterization_tasks(rendinst::RIOcclusionData &od,
  Occlusion &occlusion,
  mat44f_cref cullgtm,
  eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &tasks)
{
  mat44f globtm = occlusion.getCurViewProj();
  const int maxRiCount = 2; // we can't just rasterize one. It can be the one behind us.
  rendinst::prepareOcclusionData(cullgtm, occlusion.getCurViewPos(), od, maxRiCount, true);
  const mat44f *mat = nullptr;
  const IPoint2 *indices = nullptr;
  uint32_t cnt = 0;
  iterateOcclusionData(od, mat, indices, cnt); // indices.x is index in mat array

  const uint32_t partition = (uint32_t)sw_raster_partition_size.get();
  for (uint32_t ie = cnt, i = 0; i < ie; ++i, ++indices)
  {
    mat44f_cref ritm = mat[indices->x];
    const uint32_t type = rendinst::mat44_to_ri_type(ritm);
    CollisionResource *collRes = rendinst::getRIGenExtraCollRes(type);
    mat44f worldviewproj;
    v_mat44_mul43(worldviewproj, globtm, ritm);
    CollisionGeometryFeeder::addRasterizationTasks(*collRes, worldviewproj, tasks, partition, /*allow_convex*/ true);
  }
}
