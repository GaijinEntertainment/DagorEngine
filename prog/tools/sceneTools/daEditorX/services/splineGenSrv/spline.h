// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <de3_splineGenSrv.h>
#include <de3_randomSeed.h>
#include <de3_entityPool.h>
#include <libTools/staticGeom/geomObject.h>
#include <util/dag_fastIntList.h>

class SplineGenEntity : public IObjEntity, public ISplineGenObj, public IRandomSeedHolder
{
public:
  SplineGenEntity(int cls) : IObjEntity(cls)
  {
    rndSeed = 123;
    tm.identity();
  }

  void setTm(const TMatrix &_tm) override { tm = _tm; }
  void getTm(TMatrix &_tm) const override { _tm = tm; }

  void setSubtype(int t) override { subType = t; }
  void setEditLayerIdx(int t) override;

  void destroy() override;

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, ISplineGenObj);
    RETURN_INTERFACE(huid, IRandomSeedHolder);
    return NULL;
  }

  BBox3 getBbox() const override { return BBox3(); }
  BSphere3 getBsph() const override { return BSphere3(); }

  const char *getObjAssetName() const override { return splNames.getName(nameId); }

  void setup(const DagorAsset &asset);
  void copyFrom(const SplineGenEntity &e);

  bool isNonVirtual() const { return pool; }
  bool mayHaveLayer(int layer) const { return layer == 0 || hasNonZeroLayers; }

  bool hasGeom() const { return loftGeom; }
  bool shouldBeTraced() const { return loftGeom ? loftGeom->hasCollision() : false; }
  bool mayHaveFoundationGeom() const { return splineClass && splineClass->genGeom ? splineClass->genGeom->foundationGeom : false; }

  // IRandomSeedHolder interface
  void setSeed(int new_seed) override { rndSeed = new_seed; }
  int getSeed() override { return rndSeed; }
  void setPerInstanceSeed(int seed) override { instSeed = seed; }
  int getPerInstanceSeed() override { return instSeed; }

  // ISplineGenObj interface
  void clearObject() override { clear_and_shrink(entPools); }
  void clearLoftGeom() override { removeLoftGeom(); }
  void clear() override
  {
    clearObject();
    clearLoftGeom();
  }

  void transform(TMatrix &tm) override;
  BBox3 calcWABB(bool geom, bool entities) const override;

  void generateObjects(BezierSpline3d &effSpline, int start_seg, int end_seg, int splineSubTypeId, int rndSeed, int perInstSeed,
    Tab<cable_handle_t> *cablesPool) override;
  void generateLoftSegments(BezierSpline3d &effSpline, const char *name, int start_idx, int end_idx, bool place_on_collision,
    dag::ConstSpan<splineclass::Attr> splineScales, float scaleTcAlong) override;
  void gatherLoftLandPts(Tab<Point3> &loft_pt_cloud, BezierSpline3d &effSpline, BezierSpline2d &splineXZ, const char *name,
    int start_idx, int end_idx, bool place_on_collision, dag::ConstSpan<splineclass::Attr> splineScales,
    Tab<Point3> &water_border_polys, Tab<Point2> &hmap_sweep_polys, bool water_surf, float water_level) override;

  static Point3 getPtEffRelBezierIn(dag::ConstSpan<SplinePt> pts, int arrId, int t);
  static Point3 getPtEffRelBezierOut(dag::ConstSpan<SplinePt> pts, int arrId, int t);

  static void build_corner_spline(BezierSplinePrec3d &loftSpl, BezierSplinePrec3d &baseSpl, dag::ConstSpan<splineclass::SegData> seg,
    int ss, int es);
  static void build_ground_spline(BezierSpline3d &gndSpl, dag::ConstSpan<ISplineGenObj::SplinePt> points, const TMatrix &tm,
    int corner_type, bool poly, bool poly_smooth, bool is_closed);

  static void recalcGeomLighting(StaticGeometryContainer &geom);

public:
  enum
  {
    MAX_ENTITIES = 0x7FFFFFFF
  };
  TMatrix tm;
  int rndSeed;
  int instSeed = 0;
  SingleEntityPool<SplineGenEntity> *pool = nullptr;
  unsigned idx = 0xffffffffu;
  int nameId = -1;
  bool hasNonZeroLayers = false;

  static int loftChangesCount;
  static ISplineGenService::LayerIndexList cachedLoftLayers;
  static OAHashNameMap<true> splNames;

  void resetUsedPoolsEntities();
  void deleteUnusedPoolsEntities();

  GeomObject &getClearedLoftGeom();
  void removeLoftGeom();

  void updateLoftBox();
};

class SplineEntity : public IObjEntity, public IRandomSeedHolder, public ISplineEntity
{
public:
  struct SplineData : DObject
  {
    struct Attr
    {
      DagorAsset *splineClass = nullptr;
      splineclass::Attr attr;
    };

    int rndSeed = 0;
    int perInstSeed = 0;

    bool placeOnCollision = false;
    bool maySelfCross = false;
    bool isPoly = false;

    short cornerType = 2; // -1=polyline, 0=smooth 1st deriv., 1=smooth 2nd deriv.
    bool exportable = false;
    bool useForNavMesh = false;
    float navMeshStripeWidth = 0;

    int layerOrder = 0;
    float scaleTcAlong = 1;

    IPolygonGenObj::Props poly;

    Tab<ISplineGenObj::SplinePt> pt;
    Tab<Attr> ptAttr;
    DagorAsset *landClass = nullptr;
    SimpleString splAssetName;
    SimpleString landAssetName;

    BBox3 lbb;
    BSphere3 lbs;

  public:
    bool load(const DataBlock &blk);
    bool save(DataBlock &blk, const TMatrix &tm) const;
  };
  struct GenPart
  {
    IObjEntity *e = nullptr;
    int startIdx = 0, endIdx = 0;
  };

public:
  SplineEntity() : IObjEntity(-1) { tm.identity(); }
  ~SplineEntity();

  bool load(const DataBlock &blk);

  void updateSpline();
  void generateData();

  void setTm(const TMatrix &_tm) override;
  void getTm(TMatrix &_tm) const override { _tm = tm; }

  // void setSubtype(int t) override { subType = t; }
  void setEditLayerIdx(int t) override;
  void setGizmoTranformMode(bool enabled) override;

  void destroy() override { delete this; }

  void *queryInterfacePtr(unsigned huid) override
  {
    RETURN_INTERFACE(huid, ISplineEntity);
    RETURN_INTERFACE(huid, IRandomSeedHolder);
    return NULL;
  }

  BBox3 getBbox() const override { return sd ? sd->lbb : BBox3(); }
  BSphere3 getBsph() const override { return sd ? sd->lbs : BSphere3(); }

  const char *getObjAssetName() const override { return "n/a"; }

  // IRandomSeedHolder interface
  void setSeed(int new_seed) override { rndSeed = new_seed; }
  int getSeed() override { return rndSeed; }
  void setPerInstanceSeed(int seed) override { instSeed = seed; }
  int getPerInstanceSeed() override { return instSeed; }

  // ISplineEntity
  IObjEntity *createSplineEntityInstance() override;
  bool saveSplineTo(DataBlock &out_blk) const override;
  const BezierSpline3d &getBezierSpline() const override { return bezierSpline; }
  BBox3 calcWABB(bool spline_path, bool geom, bool entities) const override;

public:
  TMatrix tm;
  int rndSeed = 123;
  int instSeed = 0;
  Ptr<SplineData> sd;
  Tab<GenPart> ptGen;
  IObjEntity *polyGen = nullptr;
  BezierSpline3d bezierSpline;
  bool gizmoEnabled = false;
};
