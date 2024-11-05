// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/integer/dag_IPoint2.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <scene/dag_occlusion.h>
#include <util/dag_convar.h>
#include <gameRes/dag_collisionResource.h>
#include <perfMon/dag_statDrv.h>
#include <heightmap/heightmapHandler.h>
#include <math/dag_Point4.h>
#include <render/world/occlusionLandMeshManager.h>

CONSOLE_INT_VAL("render", sw_raster_partition_size, 1000, 100, 10000);

void make_rasterization_tasks_by_collision(const CollisionResource *coll_res,
  mat44f_cref worldviewproj,
  eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &tasks,
  bool allow_convex)
{
  vec4f bmin = v_ld(&coll_res->boundingBox[0].x), bmax = v_ldu(&coll_res->boundingBox[1].x);
  for (int ni = 0; ni < coll_res->getAllNodes().size(); ++ni)
  {
    const CollisionNode *node = coll_res->getNode(ni);
    if (
      !node->indices.size() || !(node->type == COLLISION_NODE_TYPE_MESH || (allow_convex && node->type == COLLISION_NODE_TYPE_CONVEX)))
      continue;
    if (!node->checkBehaviorFlags(CollisionNode::TRACEABLE))
      continue;
    const uint32_t TRIANGLES_PARTITION = sw_raster_partition_size.get();
    if ((node->flags & (node->IDENT | node->TRANSLATE)) == node->IDENT)
    {
      for (uint32_t i = 0, ie = node->indices.size() / 3; i < ie; i += TRIANGLES_PARTITION)
        tasks.emplace_back(ParallelOcclusionRasterizer::RasterizationTaskData{worldviewproj, bmin, bmax,
          (const float *)node->vertices.data(), node->indices.data() + i * 3, min(ie - i, TRIANGLES_PARTITION)});
    }
    else
    {
      mat44f nodeTm;
      v_mat44_make_from_43ca(nodeTm, node->tm[0]);
      v_mat44_mul43(nodeTm, worldviewproj, nodeTm);
      for (uint32_t i = 0, ie = node->indices.size() / 3; i < ie; i += TRIANGLES_PARTITION)
        tasks.emplace_back(
          ParallelOcclusionRasterizer::RasterizationTaskData{nodeTm, v_ld(&node->modelBBox[0].x), v_ldu(&node->modelBBox[1].x),
            (const float *)node->vertices.data(), node->indices.data() + i * 3, min(ie - i, TRIANGLES_PARTITION)});
    }
  }
}

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

  for (uint32_t ie = cnt, i = 0; i < ie; ++i, ++indices)
  {
    mat44f_cref ritm = mat[indices->x];
    const uint32_t type = rendinst::mat44_to_ri_type(ritm);
    CollisionResource *collRes = rendinst::getRIGenExtraCollRes(type);
    // if (collRes->boundingSphere.r<2)//should not happen. We assume it is large occluder
    //   continue;
    mat44f worldviewproj;
    v_mat44_mul43(worldviewproj, globtm, ritm);
    make_rasterization_tasks_by_collision(collRes, worldviewproj, tasks, true);
  }
}