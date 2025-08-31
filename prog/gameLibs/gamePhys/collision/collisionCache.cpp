// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/collision/collisionCache.h>
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/phys/physDebugDraw.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <gameMath/traceUtils.h>
#include <perfMon/dag_statDrv.h>
#include <rendInst/rendInstCollision.h>
#include <landMesh/lmeshManager.h>
#include <landMesh/landRayTracer.h>
#include <heightmap/heightmapHandler.h>
#include <sceneRay/dag_sceneRay.h>
#include <sceneRay/dag_cachedRtVecFaces.h>
#include <physMap/physMap.h>
#include <physMap/physMatSwRenderer.h>
#include "collisionGlobals.h"

static bool can_use_trace_cache(const bbox3f &query_box, const TraceMeshFaces *handle)
{
  return !handle->isDirty && v_bbox3_test_box_inside(v_ldu_bbox3(handle->box), query_box);
}

bool dacoll::try_use_trace_cache(const bbox3f &query_box, const TraceMeshFaces *handle)
{
  TRACE_HANDLE_DEBUG_STAT(handle, numCasts++);
  bool res = can_use_trace_cache(query_box, handle);
  if (!res)
    TRACE_HANDLE_DEBUG_STAT(handle, numFullMisses++);
  return res;
}

void dacoll::validate_trace_cache_oobb(const TMatrix &tm, const bbox3f &oobb, const vec3f &expands, float physmap_expands,
  TraceMeshFaces *handle)
{
  mat44f tm44;
  v_mat44_make_from_43cu_unsafe(tm44, tm.m[0]);
  bbox3f aabb;
  v_bbox3_init(aabb, tm44, oobb);
  validate_trace_cache(aabb, expands, physmap_expands, handle);
}

void dacoll::validate_trace_cache(const bbox3f &query_box, const vec3f &expands, float physmap_expands, TraceMeshFaces *handle,
  float rel_shift_threshold /* = 0.2f */)
{
  bbox3f extBox = query_box;
  v_bbox3_extend(extBox, expands);
  float shiftThreshold = v_extract_x(v_length3_x(v_bbox3_size(query_box))) * rel_shift_threshold;
  vec3f queryBoxcenter = v_bbox3_center(query_box);
  vec3f cacheBoxCenter = v_bbox3_center(v_ldu_bbox3(handle->box));
  bool validated = v_extract_x(v_length3_x(v_sub(queryBoxcenter, cacheBoxCenter))) < shiftThreshold;

  if (validated)
  {
    // We do have RIs in this cache, check if world has changed and
    // if so, re-initialize cached RI data.
    // Note that we're only doing it when the cache is not overflowed,
    // if it gets overflowed then it doesn't make sense, wait until next box
    // update with fewer RIs.
    if (handle->isRendinstsValid && !rendinst::checkCachedRiData(handle))
    {
      TIME_PROFILE(validate_trace_cache_ri);
      handle->isRendinstsValid = rendinst::initializeCachedRiData(handle);
      TRACE_HANDLE_DEBUG_STAT(handle, numRIInvalidates++);
      gamephys::draw_trace_handle_debug_invalidate(handle, true);
    }
    return;
  }

  TIME_PROFILE(validate_trace_cache);

  handle->isDirty = false;

  int left = handle->MAX_TRIANGLES;
  int trianglesAdded = 0;

  v_stu_bbox3(handle->box, extBox);

  if (LandRayTracer *landTracer = dacoll::get_lmesh() ? dacoll::get_lmesh()->getLandTracer() : nullptr)
  {
    int leftForLandmesh = TraceMeshFaces::MAX_TRIANGLES_PER_SYSTEM;
    handle->isLandmeshValid = landTracer->getFaces(extBox, handle->triangles.data(), leftForLandmesh);
    int numTriangles = (TraceMeshFaces::MAX_TRIANGLES_PER_SYSTEM - leftForLandmesh);
    if (handle->isLandmeshValid)
    {
      trianglesAdded += numTriangles;
      left -= numTriangles;
    }
  }
  else
    handle->isLandmeshValid = false;

  if (const StaticSceneRayTracer *sceneRay = dacoll::get_frt())
  {
    int leftForSceneRay = TraceMeshFaces::MAX_TRIANGLES_PER_SYSTEM;
    handle->isStaticValid = sceneRay->getVecFacesCached(extBox, handle->triangles.data() + trianglesAdded * 3, leftForSceneRay);
    int numTriangles = (TraceMeshFaces::MAX_TRIANGLES_PER_SYSTEM - leftForSceneRay);
    if (handle->isStaticValid)
    {
      trianglesAdded += numTriangles;
      left -= numTriangles;
    }
  }
  else
    handle->isStaticValid = false;
  handle->trianglesCount = handle->MAX_TRIANGLES - left;

  if (HeightmapHandler *heightMap = dacoll::get_lmesh() ? dacoll::get_lmesh()->getHmapHandler() : nullptr)
  {
    if (!heightMap->getHeightsGrid(extBox, handle->heightmap.heights.data(), handle->heightmap.heights.size(),
          handle->heightmap.gridOffset, handle->heightmap.gridElemSize, handle->heightmap.width, handle->heightmap.height))
      handle->heightmap.width = handle->heightmap.height = -1;
  }
  else
    handle->heightmap.width = handle->heightmap.height = -1;

  handle->isRendinstsValid = rendinst::initializeCachedRiData(handle);
  // only object groups are left outside, so WT _almost_ can use it, instead of copypasta

  Point3_vec4 boxMin, boxMax;
  v_st(&boxMin.x, query_box.bmin);
  v_st(&boxMax.x, query_box.bmax);
  BBox2 rayBox2d(Point2::xz(boxMin), Point2::xz(boxMax));
  const PhysMap *physMap = dacoll::get_lmesh_phys_map();
  if ((!handle->matMapCache.valid || !non_empty_boxes_inclusive(rayBox2d, handle->matMapCache.box)) && physMap)
  {
    TIME_PROFILE(physmat_map_cache_render);
    BBox2 region = rayBox2d;
    region.inflate(physmap_expands);

    constexpr int width = 64;
    constexpr int height = 64;
    handle->matMapCache.matIds.resize(width * height);
    RenderDecalMaterials<width, height> decalRenderer(make_span(handle->matMapCache.matIds));
    decalRenderer.renderPhysMap(*physMap, region);

    handle->matMapCache.setBox(width, height, region);
  }

  TRACE_HANDLE_DEBUG_STAT(handle, numRIInvalidates++);
  TRACE_HANDLE_DEBUG_STAT(handle, numFullInvalidates++);
  gamephys::draw_trace_handle_debug_invalidate(handle, false);
}
