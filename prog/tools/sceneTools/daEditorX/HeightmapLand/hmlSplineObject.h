#ifndef __GAIJIN_HEIGHTMAPLAND_PLUGIN_SPLINE_OBJECT__
#define __GAIJIN_HEIGHTMAPLAND_PLUGIN_SPLINE_OBJECT__
#pragma once


#include <math/dag_bezier.h>

#include <EditorCore/ec_rendEdObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <generic/dag_ptrTab.h>

#include <libTools/staticGeom/geomObject.h>
#include <util/dag_hierBitMap2d.h>
#include <util/dag_oaHashNameMap.h>
#include <de3_cableSrv.h>
#include <shaders/dag_overrideStateId.h>

#include "common.h"
#include "hmlLayers.h"

class HeightMapStorage;
class LoftObject;
class DagSpline;
class SplinePointObject;
class HmapLandObjectEditor;
class IObjEntity;
class DebugPrimitivesVbuffer;

static constexpr unsigned CID_HMapSplineObject = 0xD7BAAB6Au; // SplineObject


typedef HierBitMap2d<ConstSizeBitMap2d<5>> HmapBitmap;

namespace objgenerator
{
class LandClassData;
}
namespace landclass
{
class AssetData;
}
namespace splineclass
{
class AssetData;
class RoadData;
} // namespace splineclass

enum
{
  MODIF_NONE = 0,
  MODIF_SPLINE,
  MODIF_HMAP,

  LAYER_ORDER_MAX = 16
};

class SplineObject : public RenderableEditableObject
{
protected:
  bool created;

public:
  bool splineInactive = false;

protected:
  ~SplineObject();

public:
  enum
  {
    STAGE_START,
    STAGE_GEN_POLY,
    STAGE_GEN_LOFT,
    STAGE_FINISH = 3
  };

  struct ModifParams
  {
    real width;
    real smooth;
    real offsetPow;
    Point2 offset;
    bool additive;
  };

  struct PolyGeom
  {
    typedef FaceNGr Triangle;

    PolyGeom();
    ~PolyGeom();

    void createMeshes(const char *parent_name, SplineObject &spline);
    static void recalcLighting(GeomObject *go);
    void clear();
    void renderLines();

    GeomObject *mainMesh, *borderMesh;
    Tab<Triangle> centerTri, borderTri;
    Tab<Point3> verts;

    bool altGeom;
    float bboxAlignStep;
  };

  SplineObject(bool make_poly);

  virtual void update(real dt) {}
  virtual void beforeRender() {}
  virtual void render() {}
  virtual void renderTrans() {}
  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const;
  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const;
  virtual bool getWorldBox(BBox3 &box) const;
  virtual void fillProps(PropertyContainerControlBase &op, DClassID for_class_id, dag::ConstSpan<RenderableEditableObject *> objects);
  virtual void onPPChange(int pid, bool edit_finished, PropPanel2 &panel, dag::ConstSpan<RenderableEditableObject *> objects);
  virtual void onPPBtnPressed(int pid, PropPanel2 &panel, dag::ConstSpan<RenderableEditableObject *> objects);
  virtual void moveObject(const Point3 &delta, IEditorCoreEngine::BasisType basis);
  virtual void rotateObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis);
  virtual void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis);
  virtual void putMoveUndo();
  virtual void onRemove(ObjectEditor *);
  virtual void onAdd(ObjectEditor *objEditor);
  virtual Point3 getPos() const { return splineCenter(); }

  EO_IMPLEMENT_RTTI(CID_HMapSplineObject);


  void getSpline();
  void getSpline(DagSpline &spline);
  void getSplineXZ(BezierSpline2d &spline2d);

  void updateRoadBox();
  void updateLoftBox();

  void getSmoothPoly(Tab<Point3> &pts);

  void gatherStaticGeometry(StaticGeometryContainer &cont, int flags, bool collision, int layer, int stage);
  static void gatherStaticGeom(StaticGeometryContainer &cont, const StaticGeometryContainer &geom, int flags, int id, int name_idx1,
    int stage);
  static void gatherStaticGeomLayered(StaticGeometryContainer &cont, const StaticGeometryContainer &geom, int flags, int id,
    int name_idx1, int layer, int stage);

  void gatherLoftLandPts(Tab<Point3> &loft_pt_cloud, Tab<Point3> &water_border_polys, Tab<Point2> &hmap_sweep_polys);

  void renderLines(bool opaque_pass, const Frustum &frustum);
  void regenerateVBuf();

  void render(DynRenderBuffer *db, const TMatrix4 &gtm, const Point2 &s, const Frustum &frustum, int &cnt);

  bool getPosOnSpline(IGenViewportWnd *vp, int x, int y, float max_dist, int *out_segid = NULL, float *out_local_t = NULL,
    Point3 *out_p = NULL) const;

  void onPointRemove(int id);
  void addPoint(SplinePointObject *pt);

  void refine(int seg_id, real loc_t, Point3 &p_pos);
  void split(int pt_id);

  void save(DataBlock &blk);
  void load(const DataBlock &blk, bool use_undo);

  void regenerateObjects();
  void onCreated(bool gen = true);
  void reverse();
  void renderGeom(bool opaque, int layer, const Frustum &);
  bool isSelfCross();
  void updateFullLoft();
  BBox3 getGeomBox();
  BBox3 getGeomBoxChanges();

  void gatherPolyGeomLoftTags(OAHashNameMap<true> &loft_tags);

  real getTotalLength();

  void putObjTransformUndo();

  void prepareSplineClassInPoints();

  void pointChanged(int pt_idx);
  void markModifChanged();
  void markModifChangedWhenUsed()
  {
    if (isAffectingHmap())
      markModifChanged();
  }
  void markAssetChanged(int start_pt_idx);
  void invalidateSplineCurve()
  {
    pointChanged(-1);
    getSpline();
  }

  void updateSpline(int stage);
  void updateChangedSegmentsRoadGeom();
  void updateChangedSegmentsLoftGeom();

  bool pointInsidePoly(const Point2 &p);
  void applyHmapModifier(HeightMapStorage &hm, Point2 hm_ofs, float hm_cell_size, IBBox2 &out_dirty, const IBBox2 &dirty_clip,
    HmapBitmap *bmp = NULL);

  Point3 getProjPoint(const Point3 &p, real *proj_t = NULL);
  Point3 getPolyClosingProjPoint(Point3 &p, real *proj_t = NULL);
  SplinePointObject *getNearestPoint(Point3 &p);
  real getSplineTAtPoint(int id);
  bool lineIntersIgnoreY(Point3 &p1, Point3 &p2);

  void movePointByNormal(Point3 &p, int id, SplineObject *flatt_spl);
  void triangulatePoly();
  bool calcPolyPlane(Point4 &plane);

  void splitOnTwoPolys(int pt1, int pt2);
  void buildInnerSpline(Tab<Point3> &out_pts, float offset, float pts_y, float eps);

  bool isDirReversed(Point3 &p1, Point3 &p2);

  SplineObject *clone();

  inline bool isClosed() { return points.size() > 2 && points[0].get() == points.back().get(); }
  inline bool isPoly() { return poly; }
  inline bool isCreated() { return created; }
  inline bool isPolyHmapAlign() { return props.poly.hmapAlign; }
  inline int getModifType() { return props.modifType; }
  inline int getEffModifType() { return poly ? (props.poly.hmapAlign ? MODIF_HMAP : MODIF_NONE) : props.modifType; }
  inline BezierSpline3d &getBezierSpline() { return bezierSpline; }
  inline const BezierSpline3d &getBezierSpline() const { return bezierSpline; }
  inline const char *getBlkGenName() { return props.blkGenName; }
  inline real getPolyObjRot() { return props.poly.objRot; }
  inline Point2 getPolyObjOffs() { return props.poly.objOffs; }
  inline bool isExportable() { return props.exportable; }
  inline bool isAffectingHmap() { return isPoly() ? isPolyHmapAlign() : getModifType() == MODIF_HMAP; }

  inline void setPolyHmapAlign(bool a) { props.poly.hmapAlign = a; }
  inline void setModifType(int t) { props.modifType = t; }
  inline void setBlkGenName(const char *n) { props.blkGenName = n; }
  inline void setPolyObjRot(real rot) { props.poly.objRot = rot; }
  inline void setPolyObjOffs(Point2 offs) { props.poly.objOffs = offs; }
  inline void setExportable(bool ex) { props.exportable = ex; }
  inline void setCornerType(int t) { props.cornerType = t; }
  inline void setRandomSeed(int seed) { props.rndSeed = seed; }

  void loadModifParams(const DataBlock &blk);
  void attachTo(SplineObject *s, int to_idx = -1);

  Point3 splineCenter() const;
  bool intersects(const BBox2 &r, bool mark_segments);
  const BBox3 &getSplineBox() const { return splBox; }

  void reApplyModifiers(bool apply_now = true, bool force = false);

  bool onAssetChanged(landclass::AssetData *data);
  bool onAssetChanged(splineclass::AssetData *data);
  UndoRedoObject *makePointListUndo();

  void resetSplineClass();
  void changeAsset(const char *asset_name, bool put_undo);
  const objgenerator::LandClassData *getLandClass() const { return landClass; }

  void makeMonoUp();
  void makeMonoDown();
  void makeLinearHt();
  void applyCatmull(bool xz, bool y);

  void setEditLayerIdx(int idx);
  int getEditLayerIdx() const { return editLayerIdx; }
  int lpIndex() const { return poly ? EditLayerProps::PLG : EditLayerProps::SPL; };

  static int makeSplinesCrosses(dag::ConstSpan<SplineObject *> spls);

  PtrTab<SplinePointObject> points;
  PolyGeom polyGeom;
  IObjEntity *csgGen;
  IBBox2 lastModifArea, lastModifAreaDet;

  Tab<cable_handle_t> cablesPool;

  bool splineChanged;
  bool modifChanged;
  shaders::OverrideStateId zFuncLessStateId;

public:
  struct Props
  {
    String blkGenName;
    int rndSeed;
    int perInstSeed = 0;

    ModifParams modifParams;
    short modifType;
    bool maySelfCross;

    struct PolyProps
    {
      Point2 objOffs;
      real objRot;
      float curvStrength;
      float minStep, maxStep;
      bool hmapAlign, smooth;
    } poly;

    short cornerType; // -1=polyline, 0=smooth 1st deriv., 1=smooth 2nd deriv.
    bool exportable;
    bool useForNavMesh;
    float navMeshStripeWidth;

    String notes;
    int layerOrder;
    float scaleTcAlong;
  };
  const Props &getProps() const { return props; }
  const int getLayer() const { return min(LAYER_ORDER_MAX - 1, props.layerOrder); }

  static int polygonSubtypeMask;
  static int splineSubtypeMask;
  static int tiledByPolygonSubTypeId;
  static int splineSubTypeId;
  static int roadsSubtypeMask;

  static bool isSplineCacheValid;

protected:
  Props props;
  int editLayerIdx = 0;

  void generateRoadSegments(int start_idx, int end_idx, const splineclass::RoadData *asset_prev, splineclass::RoadData *asset,
    const splineclass::RoadData *asset_next);
  void generateLoftSegments(int start_idx, int end_idx);

  HmapLandObjectEditor &getObjEd() const { return *(HmapLandObjectEditor *)getObjEditor(); }
  static HmapLandObjectEditor &getObjEd(ObjectEditor *oe) { return *(HmapLandObjectEditor *)oe; }

  void placeObjectsInsidePolygon();

  void onLayerOrderChanged();

  BezierSpline3d bezierSpline;

  Tab<BSphere3> segSph;
  bool poly;
  bool firstApply;

  SplineObject *flattenBySpline;

  BSphere3 splSph;
  BBox3 splBox, splBoxPrev;
  BBox3 loftBox, roadBox;
  BBox3 geomBoxPrev;
  float splStep;

  objgenerator::LandClassData *landClass;

  DebugPrimitivesVbuffer *bezierBuf;
};

struct SplineObjectRec
{
  SplineObject *s;
  Tab<SplinePointObject *> p1, p2;

  int k;

  SplineObjectRec() : p1(tmpmem), p2(tmpmem) { k = 0; }
};

#endif //__GAIJIN_HEIGHTMAPLAND_PLUGIN_SPLINE_OBJECT__
