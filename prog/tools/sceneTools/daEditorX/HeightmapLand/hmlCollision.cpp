// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightMapLand/dag_hmlTraceRay.h>
#include <heightMapLand/dag_hmlGetHeight.h>
#include "hmlPlugin.h"

#include <de3_interface.h>
#include <de3_entityFilter.h>
#include <de3_genHmapData.h>
#include <sceneRay/dag_sceneRay.h>


#define CHECK_DET_VERT_TRACE(PT, NORM, MAXT)                                \
  if (detDivisor && (detRect & Point2(PT.x, PT.z)))                         \
  {                                                                         \
    HMDetTR det_hm;                                                         \
    if (get_height_midpoint_heightmap(det_hm, Point2(PT.x, PT.z), y, NORM)) \
    {                                                                       \
      float t = PT.y - y;                                                   \
      if (t >= 0 && t < MAXT)                                               \
      {                                                                     \
        MAXT = t;                                                           \
        return true;                                                        \
      }                                                                     \
    }                                                                       \
    return false;                                                           \
  }

bool HmapLandPlugin::isColliderVisible() const
{
  if (exportType == EXPORT_PSEUDO_PLANE)
    return false;

  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
  return getVisible() && (st_mask & (hmapSubtypeMask | lmeshSubtypeMask));
}

bool HmapLandPlugin::traceRayPrivate(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm)
{
  HMDetTR det_hm_tr;
  if (exportType == EXPORT_PSEUDO_PLANE)
  {
    if (EDITORCORE->traceRay(p, dir, maxt, NULL))
      return true;

    if (!heightMap.isFileOpened())
      return false;

    if (dir.y < -0.999)
    {
      real y;
      CHECK_DET_VERT_TRACE(p, norm, maxt);
      if (get_height_midpoint_heightmap(*genHmap, Point2(p.x, p.z), y, norm))
      {
        float t = p.y - y;
        if (t >= 0 && t < maxt)
        {
          maxt = t;
          return true;
        }
      }
      return false;
    }

    bool ret = detDivisor ? ::trace_ray_midpoint_heightmap(det_hm_tr, p, dir, maxt, norm) : false;
    return ::trace_ray_midpoint_heightmap(*genHmap, p, dir, maxt, norm) || ret;
  }

  if (noTraceNow || !heightMap.isFileOpened())
    return false;

  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);
  if (exportType == EXPORT_LANDMESH && !landMeshMap.isEmpty() && (st_mask & lmeshSubtypeMask) && !(st_mask & hmapSubtypeMask))
  {
    bool ret = detDivisor ? ::trace_ray_midpoint_heightmap(det_hm_tr, p, dir, maxt, norm) : false;
    return landMeshMap.traceRay(p, dir, maxt, norm) || ret;
  }

  applyHmModifiers(false);

  if (dir.y < -0.999)
  {
    real y;
    CHECK_DET_VERT_TRACE(p, norm, maxt);
    if (get_height_midpoint_heightmap(*this, Point2(p.x, p.z), y, norm))
    {
      float t = p.y - y;
      if (t >= 0 && t < maxt)
      {
        maxt = t;
        return true;
      }
    }
    return false;
  }

  bool ret = detDivisor ? ::trace_ray_midpoint_heightmap(det_hm_tr, p, dir, maxt, norm) : false;
  return ::trace_ray_midpoint_heightmap(*this, p, dir, maxt, norm) || ret;
}

bool HmapLandPlugin::traceRay(const Point3 &pt, const Point3 &dir, real &maxt, Point3 *norm)
{
  if (!heightMap.isFileOpened() || noTraceNow || exportType == EXPORT_PSEUDO_PLANE)
    return false;

  int st_mask = DAEDITOR3.getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_COLLISION);
  if (!(st_mask & (hmapSubtypeMask | lmeshSubtypeMask)))
    return false;

  HMDetTR det_hm_tr;
  if (exportType == EXPORT_LANDMESH && !landMeshMap.isEmpty())
  {
    bool ret = detDivisor ? ::trace_ray_midpoint_heightmap(det_hm_tr, pt, dir, maxt, norm) : false;
    return landMeshMap.traceRay(pt, dir, maxt, norm) || ret;
  }

  applyHmModifiers(false);

  if (dir.y < -0.999)
  {
    real y;
    CHECK_DET_VERT_TRACE(pt, norm, maxt);
    if (get_height_midpoint_heightmap(*this, Point2(pt.x, pt.z), y, norm))
    {
      float t = pt.y - y;
      if (t >= 0 && t < maxt)
      {
        maxt = t;
        return true;
      }
    }
    return false;
  }

  bool ret = detDivisor ? ::trace_ray_midpoint_heightmap(det_hm_tr, pt, dir, maxt, norm) : false;
  return ::trace_ray_midpoint_heightmap(*this, pt, dir, maxt, norm) || ret;
}

bool HmapLandPlugin::shadowRayHitTest(const Point3 &pt, const Point3 &dir, real maxt)
{
  if (!heightMap.isFileOpened() || !isVisible || exportType == EXPORT_PSEUDO_PLANE || calculating_shadows)
    return false;

  applyHmModifiers(false);
  return ray_hit_midpoint_heightmap_approximate(*this, pt, dir, maxt);
}


bool HmapLandPlugin::getHeightmapCell5Pt(const IPoint2 &cell, real &h0, real &hx, real &hy, real &hxy, real &hmid) const
{
  if (exportType == EXPORT_PSEUDO_PLANE)
    return false;

  //== TODO: check holes?
  if (detDivisor && insideDetRectC(cell * detDivisor))
    return false;
  if (cell.x < 0 || cell.y < 0 || cell.x + 1 >= heightMap.getMapSizeX() || cell.y + 1 >= heightMap.getMapSizeY())
    return false;

  h0 = heightMap.getFinalData(cell.x, cell.y);
  hx = heightMap.getFinalData(cell.x + 1, cell.y);
  hy = heightMap.getFinalData(cell.x, cell.y + 1);
  hxy = heightMap.getFinalData(cell.x + 1, cell.y + 1);

  hmid = (h0 + hx + hy + hxy) * 0.25f;

  return true;
}

bool HmapLandPlugin::getHeightmapHeight(const IPoint2 &cell, real &ht) const
{
  if (exportType == EXPORT_PSEUDO_PLANE)
    return false;

  //== TODO: check holes?
  if (detDivisor && insideDetRectC(cell * detDivisor))
    return false;
  if (cell.x < 0 || cell.y < 0 || cell.x >= heightMap.getMapSizeX() || cell.y >= heightMap.getMapSizeY())
    return false;

  ht = heightMap.getFinalData(cell.x, cell.y);

  return true;
}


bool HmapLandPlugin::getHeightmapPointHt(Point3 &inout_p, Point3 *out_norm) const
{
  if (exportType == EXPORT_PSEUDO_PLANE)
    return false;

  if (detDivisor && (detRect & Point2::xz(inout_p)))
  {
    HMDetTR det_hm;
    return get_height_midpoint_heightmap(det_hm, Point2::xz(inout_p), inout_p.y, out_norm);
  }
  if (exportType == EXPORT_LANDMESH && !landMeshMap.isEmpty())
  {
    return landMeshMap.getHeight(Point2::xz(inout_p), inout_p.y, out_norm);
  }

  return get_height_midpoint_heightmap(*this, Point2(inout_p.x, inout_p.z), inout_p.y, out_norm);
}

bool HmapLandPlugin::getHeightmapOnlyPointHt(Point3 &inout_p, Point3 *out_norm) const
{
  if (exportType == EXPORT_PSEUDO_PLANE)
    return false;

  if (detDivisor && (detRect & Point2::xz(inout_p)))
  {
    HMDetTR det_hm;
    return get_height_midpoint_heightmap(det_hm, Point2::xz(inout_p), inout_p.y, out_norm);
  }
  return get_height_midpoint_heightmap(*this, Point2(inout_p.x, inout_p.z), inout_p.y, out_norm);
}
