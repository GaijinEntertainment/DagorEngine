// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/array.h>
#include <vecmath/dag_vecMathDecl.h>
#include <heightmap/heightmapHandler.h>
#include <render/occlusion/parallelOcclusionRasterizer.h>
#include <landMesh/lmeshManager.h>
#include <util/dag_convar.h>

extern ConVarT<int, true> sw_raster_partition_size;

class OcclusionLandMeshManager
{
  static const int SIDE_ORDER = 6;
  static const int SIDE = 1 << SIDE_ORDER;
  static const int GRID_SIZE = SIDE * SIDE;

  eastl::array<vec4f, GRID_SIZE> vertices;
  eastl::array<float, GRID_SIZE> heights;
  eastl::array<unsigned short, (SIDE - 1) * (SIDE - 1) * 6> indices;
  IPoint2 lastOrigin = IPoint2(-10000, -10000);
  bbox3f bbox;
  bool heightsPrepared = false;
  void updateHeights(const Point4 &origin, const HeightmapHandler &heightmap, const LandMeshHolesManager *holes_manager)
  {
    IPoint2 currentOrigin = heightmap.posToHmapXZ(Point2::xz(origin));
    if (currentOrigin == lastOrigin)
      return;
    lastOrigin = currentOrigin;

    TIME_PROFILE(update_heights_grid);
    bbox3f occlBbox;
    v_bbox3_init_empty(occlBbox);
    Point2 gridOffset;
    float cellSize = 0;
    if (!heightmap.getHeightsGrid(origin, heights.data(), SIDE, SIDE, gridOffset, cellSize))
    {
      heightsPrepared = false;
      return;
    }
    if (holes_manager)
    {
      Point2 originCur = Point2::xz(origin);
      int leftEdge = originCur.x - cellSize * (SIDE - 1) / 2;
      int bottomEdge = originCur.y - cellSize * (SIDE - 1) / 2;
      int rightEdge = originCur.x + cellSize * (SIDE - 2) / 2;
      int upperEdge = originCur.y + cellSize * (SIDE - 2) / 2;
      for (int y = bottomEdge; y <= upperEdge; ++y)
      {
        for (int x = leftEdge; x <= rightEdge; ++x)
        {
          if (holes_manager->check(IPoint2(x, y)))
            return;
        }
      }
    }
    for (int i = 0; i < GRID_SIZE; ++i)
    {
      const int xIdx = i & (SIDE - 1);
      const int zIdx = i >> SIDE_ORDER;
      alignas(16) Point4 vertex(xIdx * cellSize + gridOffset.x, heights[i] - 2 * heightmap.getMaxUpwardDisplacement(),
        zIdx * cellSize + gridOffset.y, 0);
      vertices[i] = v_ld(&vertex.x);
      v_bbox3_add_pt(occlBbox, vertices[i]);
    }
    bbox = occlBbox;
    heightsPrepared = true;
  }

public:
  OcclusionLandMeshManager() : vertices({})
  {
    for (int i = 0, idx = 0; i < SIDE - 1; ++i)
      for (int j = 0; j < SIDE - 1; ++j)
      {
        indices[idx++] = i * SIDE + j;
        indices[idx++] = i * SIDE + (j + 1);
        indices[idx++] = (i + 1) * SIDE + (j + 1);
        indices[idx++] = (i + 1) * SIDE + j;
        indices[idx++] = i * SIDE + j;
        indices[idx++] = (i + 1) * SIDE + (j + 1);
      }
    v_bbox3_init_empty(bbox);
  }

  void makeRasterizeLandMeshTasks(const HeightmapHandler &heightmap,
    const LandMeshHolesManager *holesManager,
    Occlusion &occlusion,
    eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &tasks)
  {
    Point4 origin;
    v_stu(&origin, occlusion.getCurViewPos());
    updateHeights(origin, heightmap, holesManager);
    if (!heightsPrepared)
      return;
    const uint32_t TRIANGLES_PARTITION = sw_raster_partition_size.get();
    for (uint32_t i = 0, ie = indices.size() / 3; i < ie; i += TRIANGLES_PARTITION)
      tasks.emplace_back(ParallelOcclusionRasterizer::RasterizationTaskData{occlusion.getCurViewProj(), bbox.bmin, bbox.bmax,
        (const float *)vertices.data(), indices.data() + i * 3, min(ie - i, TRIANGLES_PARTITION)});
  }

  void reset()
  {
    heightsPrepared = false;
    lastOrigin = IPoint2(-10000, -10000);
  }
};
