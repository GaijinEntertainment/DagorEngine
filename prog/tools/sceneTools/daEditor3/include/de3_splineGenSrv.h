//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <de3_objEntity.h>
#include <de3_interface.h>
#include <de3_splineClassData.h>
#include <de3_cableSrv.h>
#include <math/dag_bezierPrec.h>
#include <util/dag_roHugeHierBitMap2d.h>
#include <util/dag_oaHashNameMap.h>

class GeomObject;
class FastIntList;
struct EditableHugeBitMap2d;

class ISplineGenObj
{
public:
  static constexpr unsigned HUID = 0x6B0E0FB0u; // ISplineGenObj
  struct SplinePt
  {
    Point3 pt, relIn, relOut;
    int cornerType = -2;
  }; // -2=def as spline, -1=polyline, 0=smooth 1st deriv., 1=smooth 2nd deriv.

  splineclass::AssetData *splineClass = nullptr;
  Tab<splineclass::SingleEntityPool> entPools;
  GeomObject *loftGeom = nullptr;
  Tab<splineclass::SegData> loftSeg;
  BBox3 loftGeomBox;
  int splineLayer = 0;

public:
  virtual void clear() = 0;
  virtual void clearObject() = 0;
  virtual void clearLoftGeom() = 0;

  virtual void transform(TMatrix &tm) = 0;

  virtual void generateObjects(BezierSpline3d &effSpline, int start_seg, int end_seg, int splineSubTypeId, int editLayerIdx,
    int rndSeed, int perInstSeed, Tab<cable_handle_t> *cablesPool) = 0;
  virtual void generateLoftSegments(BezierSpline3d &effSpline, const char *name, int start_idx, int end_idx, bool place_on_collision,
    dag::ConstSpan<splineclass::Attr> splineScales, float scaleTcAlong) = 0;
  virtual void gatherLoftLandPts(Tab<Point3> &loft_pt_cloud, BezierSpline3d &effSpline, BezierSpline2d &splineXZ, const char *name,
    int start_idx, int end_idx, bool place_on_collision, dag::ConstSpan<splineclass::Attr> splineScales,
    Tab<Point3> &water_border_polys, Tab<Point2> &hmap_sweep_polys, bool water_surf, float water_level) = 0;
};

class ISplineEntity
{
public:
  static constexpr unsigned HUID = 0xD8624EA9u; // ISplineEntity

  virtual IObjEntity *createSplineEntityInstance() = 0;
  virtual bool saveSplineTo(DataBlock &out_blk) const = 0;
};

class ISplineGenService
{
public:
  static constexpr unsigned HUID = 0x29780D3Eu; // ISplineGenService

  typedef HierBitMap2d<HierConstSizeBitMap2d<4, ConstSizeBitMap2d<5>>> EditableHugeBitmask;

  virtual void build_corner_spline(BezierSplinePrec3d &loftSpl, BezierSplinePrec3d &baseSpl, dag::ConstSpan<splineclass::SegData> seg,
    int ss, int es) const = 0;
  virtual void build_ground_spline(BezierSpline3d &gndSpl, dag::ConstSpan<ISplineGenObj::SplinePt> points, const TMatrix &tm,
    int corner_type, bool poly, bool poly_smooth, bool is_closed) const = 0;

  virtual IObjEntity *createVirtualSplineEntity(const DataBlock &blk) = 0;

  virtual void gatherLoftLayers(FastIntList &loft_layers, bool only_visible) const = 0;
  virtual void gatherLoftTags(OAHashNameMap<true> &loft_tags) const = 0;
  virtual void gatherStaticGeometry(StaticGeometryContainer &cont, int flags, bool collision, int layer, int stage, int spl_layer,
    uint64_t lh_mask = 0) const = 0;

  virtual void renderLoftGeom(int layer, bool opaque, const Frustum &frustum, int spl_layer, bool asHeightmapPatch = false) = 0;
  virtual bool traceRayFoundationGeom(int layer, const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) const = 0;
  virtual bool shadowRayFoundationGeomHitTest(int layer, const Point3 &p, const Point3 &dir, real &maxt) const = 0;

  virtual void setSweepMask(const EditableHugeBitmask *bm, float ox, float oz, float scale) = 0;
};
