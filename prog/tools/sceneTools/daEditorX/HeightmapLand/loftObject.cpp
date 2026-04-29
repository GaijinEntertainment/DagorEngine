// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "hmlPlugin.h"
#include <de3_splineClassData.h>
#include <de3_genObjData.h>
#include <de3_lightService.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include <math/dag_bezierPrec.h>

using splineclass::Attr;

void SplineObject::generateLoftSegments(int start_idx, int end_idx)
{
  if (splineInactive)
    return;
  ISplineGenObj *gen = points[start_idx]->getSplineGen();
  if (!gen)
    return;

  Tab<Attr> splineScales(midmem);
  splineScales.reserve(end_idx - start_idx + 1);
  for (int i = start_idx; i <= end_idx; i++)
    splineScales.push_back(points[i % points.size()]->getProps().attr);

  if (gen->loftGeom)
    HmapLandObjectEditor::geomBuildCntLoft++;

  gen->generateLoftSegments(getBezierSpline(), name, start_idx, end_idx, (props.modifType == MODIF_SPLINE), splineScales,
    props.scaleTcAlong);
  if (gen->loftGeom)
    HmapLandObjectEditor::geomBuildCntLoft++;
  updateLoftBox();
}

void SplineObject::gatherLoftLandPts(Tab<Point3> &loft_pt_cloud, Tab<Point3> &water_border_polys, Tab<Point2> &hmap_sweep_polys)
{
  if (splineInactive)
    return;

  bool place_on_collision = (props.modifType == MODIF_SPLINE);
  bool water_surf = HmapLandPlugin::self->hasWaterSurf();
  float water_level = HmapLandPlugin::self->getWaterSurfLevel();
  int start_idx = 0;
  int end_idx = points.size() + (poly ? 1 : 0);
  int pcnt = points.size();

  Tab<Attr> splineScales(midmem);
  splineScales.reserve(end_idx - start_idx + 1);
  for (int i = start_idx; i <= end_idx; i++)
    splineScales.push_back(points[i % points.size()]->getProps().attr);

  BezierSpline2d splineXZ;
  getSplineXZ(splineXZ);

  for (int pi = start_idx + 1; pi < end_idx; pi++)
  {
    if (points[pi % pcnt]->getSplineClass() && points[pi % pcnt]->getSplineClass() == points[start_idx]->getSplineClass())
    {
      if (pi < end_idx - 1)
        continue;
      else
        pi++;
    }

    if (ISplineGenObj *gen = points[start_idx]->getSplineGen())
      gen->gatherLoftLandPts(loft_pt_cloud, getBezierSpline(), splineXZ, name, start_idx, pi - 1, place_on_collision,
        make_span_const(splineScales).subspan(start_idx, pi - start_idx), water_border_polys, hmap_sweep_polys, water_surf,
        water_level);

    if (pi < points.size())
      start_idx = pi;
  }

  landclass::PolyGeomGenData *genGeom = NULL;
  if (auto *lc = getLandClass())
    if (lc->data)
      genGeom = lc->data->genGeom;

  if (genGeom && genGeom->waterSurface && water_surf)
  {
    Tab<Point3> pts(tmpmem);
    getSmoothPoly(pts);

    water_border_polys.reserve(pts.size() + 1);
    water_border_polys.push_back().set(1.1e12f, pts.size(), genGeom->waterSurfaceExclude);
    for (int i = 0; i < pts.size(); i++)
      water_border_polys.push_back().set_xVz(pts[i], water_level);
  }
}
