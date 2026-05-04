// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <de3_splineGenSrv.h>
#include <de3_entityPool.h>
#include <de3_genObjData.h>
#include <libTools/staticGeom/geomObject.h>
#include <math/dag_mesh.h>
#include <util/dag_fastIntList.h>

class PolygonGenEntity : public IObjEntity, public IPolygonGenObj
{
public:
  PolygonGenEntity(int cls) : IObjEntity(cls) { tm.identity(); }

  void setTm(const TMatrix &_tm) override { tm = _tm; }
  void getTm(TMatrix &_tm) const override { _tm = tm; }

  void setSubtype(int t) override { subType = t; }
  void setEditLayerIdx(int t) override;

  void destroy() override;

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, IPolygonGenObj);
    return NULL;
  }

  BBox3 getBbox() const override { return BBox3(); }
  BSphere3 getBsph() const override { return BSphere3(); }

  const char *getObjAssetName() const override { return polyNames.getName(nameId); }

  void setup(const DagorAsset &asset);
  void copyFrom(const PolygonGenEntity &e);

  bool isNonVirtual() const { return pool; }

  bool hasGeom() const { return geom.mainMesh || geom.borderMesh; }
  bool shouldBeTraced() const { return true; }
  bool mayHaveFoundationGeom() const
  {
    return landClass && landClass->data && landClass->data->genGeom ? landClass->data->genGeom->foundationGeom : false;
  }

  void updateGeomBox()
  {
    if (geom.mainMesh)
      geomBox = geom.mainMesh->getBoundBox();
    else
      geomBox.setempty();
    if (geom.borderMesh)
      geomBox += geom.borderMesh->getBoundBox();
  }

  // IPolygonGenObj interface
  void clear() override;
  void transform(TMatrix &tm) override;
  BBox3 calcWABB(bool geom, bool entities) const override;

  void buildSmoothPolyPoints(Tab<Point3> &out_smooth_poly, const BezierSpline3d &bezierSpline, const Props &props) override
  {
    build_smooth_poly(out_smooth_poly, bezierSpline, props);
  }
  bool triangulatePoly(dag::ConstSpan<Point3> smooth_poly, const Props &props, const char *poly_name,
    const BezierSpline3d *flattenBySpline) override;
  void placeObjectsInsidePolygon(dag::Span<Point3> smooth_poly, const Props &props, IHeightmap *hmap) override;

  static void clear_poly_geom(Geom &g);
  static void build_smooth_poly(Tab<Point3> &out_pts, const BezierSpline3d &bezierSpline, const Props &props);
  static bool build_inner_spline(Tab<Point3> &out_pts, dag::ConstSpan<Point3> smooth_poly, float offset, float pts_y, float eps);

  static bool triangulate_poly(Geom &g, dag::ConstSpan<Point3> smooth_poly, landclass::AssetData *land, const Props &props,
    const char *poly_name, const BezierSpline3d *flattenBySpline);
  static void place_objects_inside_polygon(objgenerator::LandClassData &landClass, dag::Span<Point3> smooth_poly, const Props &props,
    int editLayerIdx, IHeightmap *hmap);

public:
  enum
  {
    MAX_ENTITIES = 0x7FFFFFFF
  };
  TMatrix tm;
  SingleEntityPool<PolygonGenEntity> *pool = nullptr;
  unsigned idx = 0xffffffffu;
  int nameId = -1;

  static OAHashNameMap<true> polyNames;
};
