// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "helpers.h"
#include <math/dag_color.h>
#include <shaders/dag_shaders.h>
#include <landMesh/lmeshManager.h>
#include <heightmap/heightmapHandler.h>

namespace dacoll
{
extern LandMeshManager *get_lmesh();
}

LandMeshManager *getLmeshMgr() { return dacoll::get_lmesh(); }

Point4 getScaleOffset(int texture_size)
{
  const LandMeshManager *lmeshMgr = getLmeshMgr();
  if (!lmeshMgr)
    return Point4(-1, -1, -1, -1);
  const HeightmapHandler *hmapHnd = lmeshMgr->getHmapHandler();
  if (!hmapHnd)
    return Point4(-1, -1, -1, -1);

  Point2 hmapWorldSize = hmapHnd->getWorldSize();
  Point3 hmapOffset = hmapHnd->getHeightmapOffset();

  // Aligning texels centers with heightmap points.
  const float eps = 0.01f;
  Point2 samplingStep = Point2(getSamplingStepDist(hmapWorldSize.x, texture_size), getSamplingStepDist(hmapWorldSize.y, texture_size));
  Point2 groundHolesOffset = Point2(hmapOffset.x, hmapOffset.z) - (0.5 - eps) * samplingStep;

  return Point4(1.f / hmapWorldSize.x, 1.f / hmapWorldSize.y, -groundHolesOffset.x / hmapWorldSize.x,
    -groundHolesOffset.y / hmapWorldSize.y);
}

Point2 getApproxHeightmapMinMax(const BBox2 &box)
{
  const LandMeshManager *lmeshMgr = dacoll::get_lmesh();
  if (!lmeshMgr)
    return Point2(-1e5, 1e5);
  const HeightmapHandler *hmapHnd = lmeshMgr->getHmapHandler();
  if (!hmapHnd)
    return Point2(-1e5, 1e5);

  Point2 box_width = box.width();
  float patch_size = max(box_width.x, box_width.y);
  float lod = hmapHnd->heightmapHeightCulling->getLod(patch_size);

  Point2 return_point(-1e5, 1e5);
  hmapHnd->heightmapHeightCulling->getMinMax(lod, box.center() - Point2(patch_size / 2.f, patch_size / 2.f), patch_size,
    return_point.x, return_point.y);
  return return_point;
}

Point2 getApproxHeightmapMinMax(const BBox3 &box)
{
  BBox2 projection_XZ(Point2(box.lim[0].x, box.lim[0].z), Point2(box.lim[1].x, box.lim[1].z));
  return getApproxHeightmapMinMax(projection_XZ);
}

float getSamplingStepDist(float length, int tex_size) { return length / (TEX_CELL_RES * tex_size); }

bool hasGroundHoleManager()
{
  const LandMeshManager *lmeshMgr = dacoll::get_lmesh();
  return lmeshMgr && lmeshMgr->getHolesManager();
}