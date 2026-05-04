//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <de3_objEntity.h>
#include <de3_interface.h>
#include <de3_splineClassData.h>
#include <de3_landClassData.h>
#include <de3_cableSrv.h>
#include <math/dag_bezierPrec.h>
#include <util/dag_roHugeHierBitMap2d.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_hierBitArray.h>

class GeomObject;
class IHeightmap;
class FastIntList;
struct FaceNGr;
namespace objgenerator
{
class LandClassData;
struct WorldHugeBitmask;
} // namespace objgenerator

class ISplineGenObj
{
public:
  static constexpr unsigned HUID = 0x6B0E0FB0u; // ISplineGenObj
  struct SplinePt
  {
    Point3 pt, relIn, relOut;
    short int cornerType = -2; //< -2=def as spline, -1=polyline, 0=smooth 1st deriv., 1=smooth 2nd deriv.
    bool isRealCross = false;
  };

  splineclass::AssetData *splineClass = nullptr;
  Tab<splineclass::SingleEntityPool> entPools;
  GeomObject *loftGeom = nullptr;
  Tab<splineclass::SegData> loftSeg;
  BBox3 loftGeomBox;
  int layerOrder = 0;

public:
  virtual void clear() = 0;
  virtual void clearObject() = 0;
  virtual void clearLoftGeom() = 0;

  virtual void transform(TMatrix &tm) = 0;
  virtual BBox3 calcWABB(bool geom = true, bool entities = true) const = 0;

  virtual void generateObjects(BezierSpline3d &effSpline, int start_seg, int end_seg, int splineSubTypeId, int rndSeed,
    int perInstSeed, Tab<cable_handle_t> *cablesPool) = 0;
  virtual void generateLoftSegments(BezierSpline3d &effSpline, const char *name, int start_idx, int end_idx, bool place_on_collision,
    dag::ConstSpan<splineclass::Attr> splineScales, float scaleTcAlong) = 0;
  virtual void gatherLoftLandPts(Tab<Point3> &loft_pt_cloud, BezierSpline3d &effSpline, BezierSpline2d &splineXZ, const char *name,
    int start_idx, int end_idx, bool place_on_collision, dag::ConstSpan<splineclass::Attr> splineScales,
    Tab<Point3> &water_border_polys, Tab<Point2> &hmap_sweep_polys, bool water_surf, float water_level) = 0;
};

class IPolygonGenObj
{
public:
  static constexpr unsigned HUID = 0xFCA0DB52u; // IPolygonGenObj
  struct Geom
  {
    typedef FaceNGr Triangle;
    GeomObject *mainMesh = nullptr, *borderMesh = nullptr;
    Tab<Triangle> centerTri, borderTri;
    Tab<Point3> verts;
  };
  struct Props
  {
    Point2 objOffs = Point2(0.f, 0.f);
    real objRot = 0.f;
    float curvStrength = 1.f;
    float minStep = 10.f, maxStep = 100.f;
    bool hmapAlign = false, smooth = false;
    bool tiledObjsWorldAnchor = true, plantedObjsWorldAnchor = false;
    bool altGeom = false;
    float bboxAlignStep = 1.f;
  };

  objgenerator::LandClassData *landClass = nullptr;
  IPolygonGenObj::Geom geom;
  BBox3 geomBox;
  int layerOrder = 0;
  bool altGeom = false;

  virtual void clear() = 0;
  virtual void transform(TMatrix &tm) = 0;
  virtual BBox3 calcWABB(bool geom = true, bool entities = true) const = 0;

  virtual void buildSmoothPolyPoints(Tab<Point3> &out_smooth_poly, const BezierSpline3d &bezierSpline, const Props &props) = 0;
  virtual bool triangulatePoly(dag::ConstSpan<Point3> smooth_poly, const Props &props, const char *poly_name,
    const BezierSpline3d *flattenBySpline) = 0;
  virtual void placeObjectsInsidePolygon(dag::Span<Point3> smooth_poly, const Props &props, IHeightmap *hmap = nullptr) = 0;
};


class ISplineEntity
{
public:
  static constexpr unsigned HUID = 0xD8624EA9u; // ISplineEntity

  virtual IObjEntity *createSplineEntityInstance() = 0;
  virtual bool saveSplineTo(DataBlock &out_blk) const = 0;
  virtual const BezierSpline3d &getBezierSpline() const = 0;
  virtual BBox3 calcWABB(bool spline_path = true, bool geom = true, bool entities = true) const = 0;
};

class ISplineGenService
{
public:
  static constexpr unsigned HUID = 0x29780D3Eu; // ISplineGenService

  typedef HierBitMap2d<HierConstSizeBitMap2d<4, ConstSizeBitMap2d<5>>> EditableHugeBitmask;
  struct LayerIndexList : public ConstSizeBitArrayBase<32>
  {
    LayerIndexList() { reset(); }
    LayerIndexList(unsigned bit)
    {
      reset();
      set(bit);
    }

    unsigned getMask() const { return bits[0]; }
    void add(const LayerIndexList &other) { bits[0] |= other.bits[0]; }

    template <typename CB>
    inline void iterate_layers(CB cb /*(unsigned layer_idx)*/)
    {
      unsigned layer = 0;
      for (auto m = bits[0]; m; layer++, m >>= 1)
        if (m & 1)
          cb(layer);
    }
  };

  virtual void build_corner_spline(BezierSplinePrec3d &loftSpl, BezierSplinePrec3d &baseSpl, dag::ConstSpan<splineclass::SegData> seg,
    int ss, int es) const = 0;
  virtual void build_ground_spline(BezierSpline3d &gndSpl, dag::ConstSpan<ISplineGenObj::SplinePt> points, const TMatrix &tm,
    int corner_type, bool poly, bool poly_smooth, bool is_closed) const = 0;

  virtual void recalcLighting(GeomObject *go) = 0;

  virtual IObjEntity *createVirtualSplineEntity(const DataBlock &blk) = 0;

  virtual void gatherLoftLayers(LayerIndexList &loft_layers, bool only_visible) const = 0;
  virtual void gatherGeneratedGeomOrderLayers(LayerIndexList &order_layers) const = 0;
  virtual void gatherGeneratedGeomTags(OAHashNameMap<true> &loft_tags) const = 0;
  virtual void gatherStaticGeometry(StaticGeometryContainer &cont, int flags, bool collision, int layer, int stage, int layer_order,
    LayerHiddenMask lh_mask = LayerHiddenMask()) const = 0;

  virtual void renderGeneratedGeom(int layer, bool opaque, const Frustum &frustum, int layer_order, bool asHeightmapPatch = false) = 0;
  virtual bool traceRayFoundationLoftGeom(int layer, const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) const = 0;
  virtual bool traceRayFoundationPolyGeom(int layer, const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) const = 0;
  virtual bool shadowRayFoundationLoftGeomHitTest(int layer, const Point3 &p, const Point3 &dir, real &maxt) const = 0;
  virtual bool shadowRayFoundationPolyGeomHitTest(int layer, const Point3 &p, const Point3 &dir, real &maxt) const = 0;

  virtual void setSweepMaskForSplines(const objgenerator::WorldHugeBitmask &bm) = 0;
  virtual void setSweepMaskForPolygons(const objgenerator::WorldHugeBitmask &bm) = 0;
  virtual void resetSweepMasks() = 0;

  virtual void buildSmoothPolyPoints(Tab<Point3> &out_pts, const BezierSpline3d &bezierSpline, const IPolygonGenObj::Props &props) = 0;
  virtual bool buildInnerSpline(Tab<Point3> &out_pts, dag::ConstSpan<Point3> smooth_poly, float offset, float pts_y, float eps) = 0;
  virtual void renderDebugPolyEdges(const IPolygonGenObj::Geom &g) = 0;
};
