// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "poly.h"
#include <de3_genObjInsidePoly.h>
#include <de3_genObjByDensGridMask.h>
#include <de3_rasterizePoly.h>

OAHashNameMap<true> PolygonGenEntity::polyNames;

void PolygonGenEntity::setEditLayerIdx(int t)
{
  editLayerIdx = t;
  if (landClass)
  {
    for (auto &p : landClass->poolTiled)
      for (auto *e : p.entPool)
        if (e)
          e->setEditLayerIdx(t);
    for (auto &p : landClass->poolPlanted)
      for (auto *e : p.entPool)
        if (e)
          e->setEditLayerIdx(t);
  }
}

void PolygonGenEntity::destroy()
{
  pool->delEntity(this);
  clear();
  del_it(geom.mainMesh);
  del_it(geom.borderMesh);
  del_it(landClass);
  tm.identity();
}

void PolygonGenEntity::setup(const DagorAsset &asset)
{
  nameId = polyNames.addNameId(asset.getName());
  landClass = new objgenerator::LandClassData(asset.getName());
  geom.mainMesh = new GeomObject;
  geom.borderMesh = new GeomObject;
  geomBox.setempty();
}
void PolygonGenEntity::copyFrom(const PolygonGenEntity &e)
{
  nameId = e.nameId;
  landClass = new objgenerator::LandClassData(polyNames.getName(nameId));
  geom.mainMesh = new GeomObject;
  geom.borderMesh = new GeomObject;
  geomBox.setempty();
}

void PolygonGenEntity::clear()
{
  PolygonGenEntity::clear_poly_geom(geom);
  if (landClass)
    landClass->clearObjects();
  geomBox.setempty();
}
void PolygonGenEntity::transform(TMatrix &tm)
{
  if (!landClass)
    return;

  TMatrix m;
  for (auto &p : landClass->poolTiled)
    for (auto *e : p.entPool)
      if (e)
      {
        e->getTm(m);
        e->setTm(tm * m);
      }
  for (auto &p : landClass->poolPlanted)
    for (auto *e : p.entPool)
      if (e)
      {
        e->getTm(m);
        e->setTm(tm * m);
      }
}
BBox3 PolygonGenEntity::calcWABB(bool add_geom, bool add_entities) const
{
  BBox3 bb;
  if (add_geom)
    bb = geomBox;
  if (add_entities)
  {
    TMatrix m;
    for (auto &p : landClass->poolTiled)
      for (auto *e : p.entPool)
        if (e)
        {
          e->getTm(m);
          bb += m * e->getBbox();
        }
    for (auto &p : landClass->poolPlanted)
      for (auto *e : p.entPool)
        if (e)
        {
          e->getTm(m);
          bb += m * e->getBbox();
        }
  }
  return bb;
}

bool PolygonGenEntity::triangulatePoly(dag::ConstSpan<Point3> smooth_poly, const Props &props, const char *poly_name,
  const BezierSpline3d *flattenBySpline)
{
  altGeom = props.altGeom;
  bool res = landClass ? triangulate_poly(geom, smooth_poly, landClass->data, props, poly_name, flattenBySpline) : false;
  updateGeomBox();
  return res;
}
void PolygonGenEntity::placeObjectsInsidePolygon(dag::Span<Point3> smooth_poly, const Props &props, IHeightmap *hmap)
{
  if (landClass)
    place_objects_inside_polygon(*landClass, smooth_poly, props, editLayerIdx, hmap);
}


static void buildSegment(Tab<Point3> &out_pts, const BezierSplinePrecInt3d &seg, float seg_len, bool is_end_segment, float min_step,
  float max_step, float curvature_strength)
{
  if (min_step < 0.001f)
    min_step = 0.001f;

  if (seg_len < min_step)
  {
    out_pts.push_back(seg.point(0));
    if (is_end_segment)
      out_pts.push_back(seg.point(1));
    return;
  }

  // calculate spline points
  const Point3 endPoint = seg.point(1.f);
  const float step = min_step / 10;

  bool curvatureStop = false;
  float min_cosn = 1.f - (0.01f / curvature_strength);

  Point3 lastPt = seg.point(0.f);
  float lastS = 0;
  out_pts.push_back(lastPt);

  for (float s = step; s < seg_len; s += step)
  {
    const float curT = seg.getTFromS(s);
    Point3 current = seg.point(curT);
    const Point3 tang = normalize(seg.tang(curT));

    float targetLength = s - lastS;

    Point3 t2 = normalize(current - lastPt);

    float cosn = tang * t2;
    if (cosn < min_cosn)
      curvatureStop = 1;

    if (targetLength >= max_step || (curvatureStop && targetLength >= min_step))
    {
      out_pts.push_back(current);
      lastPt = current;
      lastS = s;

      curvatureStop = false;
    }
  }

  if (is_end_segment)
    out_pts.push_back(seg.point(1));
}

void PolygonGenEntity::build_smooth_poly(Tab<Point3> &poly_pts, const BezierSpline3d &bezierSpline, const IPolygonGenObj::Props &props)
{
  if (!props.smooth)
  {
    poly_pts.reserve(bezierSpline.segs.size());
    for (auto &s : bezierSpline.segs)
      poly_pts.push_back(s.point(0.f));
    return;
  }

  const BezierSplinePrec3d &path = ::toPrecSplineConst(bezierSpline);
  for (int i = 0; i < path.segs.size(); i++)
    buildSegment(poly_pts, path.segs[i], path.segs[i].len, false, props.minStep, props.maxStep, props.curvStrength);
}

void PolygonGenEntity::place_objects_inside_polygon(objgenerator::LandClassData &landClass, dag::Span<Point3> smooth_poly,
  const Props &props, int editLayerIdx, IHeightmap *hmap)
{
  static const int tiledByPolygonSubTypeId = IDaEditor3Engine::get().registerEntitySubTypeId("poly_tile");
  Point2 hmofs;
  if (hmap)
    hmofs.set_xz(hmap->getHeightmapOffset());
  landClass.beginGenerate();

  if (landClass.data && landClass.data->tiled)
  {
    Point2 entOfs = props.objOffs + hmofs;
    if (!props.tiledObjsWorldAnchor)
      entOfs += Point2::xz(smooth_poly[0]);
    objgenerator::generateTiledEntitiesInsidePoly(*landClass.data->tiled, tiledByPolygonSubTypeId, editLayerIdx, hmap,
      make_span(landClass.poolTiled), smooth_poly, DEG_TO_RAD * props.objRot, entOfs);
  }

  if (landClass.data && landClass.data->planted)
  {
    typedef HierBitMap2d<ConstSizeBitMap2d<5>> HierBitmap32;
    HierBitmap32 bmp;
    float ofs_x, ofs_z, cell;

    cell = rasterize_poly(bmp, ofs_x, ofs_z, smooth_poly);
    float mid_y = 0;
    for (int i = 0; i < smooth_poly.size(); i++)
      mid_y += smooth_poly[i].y / smooth_poly.size();

    Point2 w0(0.f, 0.f), entOfs(ofs_x, ofs_z);
    if (props.plantedObjsWorldAnchor)
      w0.set(ofs_x + props.objOffs.x, ofs_z + props.objOffs.y), entOfs.set(-props.objOffs.x, -props.objOffs.y);
    objgenerator::generatePlantedEntitiesInMaskedRect(*landClass.data->planted, tiledByPolygonSubTypeId, editLayerIdx, nullptr,
      make_span(landClass.poolPlanted), bmp, 1.0 / cell, w0.x, w0.y, bmp.getW() * cell, bmp.getH() * cell, entOfs.x, entOfs.y, mid_y);
  }

  landClass.endGenerate();
}
