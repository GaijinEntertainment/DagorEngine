// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <math/dag_Point3.h>
#include <math/integer/dag_IPoint3.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_postFxRenderer.h>
#include <daGI2/daGI2.h>

struct WorldSDF;

struct DaGISurfaceCache
{
  // one cell level per world SDF clip, a level's cell is max(cellSize, voxelScale * that clip's SDF voxel);
  // true when it cleared the cache, so the world SDF clear must fill the heightmap voxels again
  bool init(const DaGISettings::SurfaceCache &s, const WorldSDF &sdf, uint32_t sdf_w, uint32_t sdf_h);
  ~DaGISurfaceCache();
  void afterReset() { cleared = false; }
  // updates the radiance voxels before the world SDF clear fills them; it marks the cells of cleared voxels dead between these two;
  // store_light gates the light of the clear and the raster, not this voxel update
  void beginSdfUpdate(const Point3 &center, bool store_light);
  // after each clip's clear: a coarser clear zeroes the voxels a finer one fills
  void sdfClipCleared() { barrier(RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL); }
  void endSdfUpdate();
  // once a frame, before the traces
  void updateVoxels();
  // gbuf_pixels feed the cells (0 = none);
  // cells of 16+ relights and screen cells refresh_age frames old take refresh relights, at chance 1/refresh_age a frame (0 = never)
  void update(uint32_t gbuf_pixels, uint32_t refresh_age);
  // request_threads light the cells the probe traces missed, then those they asked to refresh;
  // the screen probe texels that missed are traced again between it and clearRequests
  void relight(uint32_t request_threads);
  // the voxel pass needs its list count zeroed
  void clearRequests();
  void debugRender();
  UniqueBufWithShaderVar dagi_surface_cache, dagi_surface_cache_requests;
  eastl::unique_ptr<ComputeShaderElement> from_gbuf_cs, relight_cs, voxels_cs, voxels_update_cs;
  PostFxRenderer dagi_world_scene_surface_cache_debug;
  uint32_t entries = 0, frame = 0, levels = 0, refreshAge = 0, voxels = 0, statsFails = 0, statsFramesLeft = 0;
  float cell0 = 0, voxelCell0 = 0;
  IPoint3 voxelsRes0 = {0, 0, 0};
  Point3 prevCenter = {0, 0, 0};
  bool cleared = false, voxelsMeaned = false;

private:
  void meanVoxels();
  void setParams();
  void barrier(ResourceBarrier rb);
  void logStats();
};
