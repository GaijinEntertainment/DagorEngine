// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_bezier.h>
#include <de3_splineGenSrv.h>

#include <EditorCore/ec_rendEdObject.h>
#include <libTools/staticGeom/staticGeometryContainer.h>
#include <generic/dag_ptrTab.h>

#include <libTools/staticGeom/geomObject.h>
#include <util/dag_hierBitMap2d.h>
#include <util/dag_oaHashNameMap.h>
#include <de3_cableSrv.h>
#include <shaders/dag_overrideStateId.h>

#include "hmlLayers.h"

class HeightMapStorage;
class LoftObject;
class DagSpline;
class SplinePointObject;
class HmapLandObjectEditor;
class IObjEntity;
class DebugPrimitivesVbuffer;
class DagorAsset;

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
  MODIF_SPLINE = 1,
  MODIF_HMAP = 2,
  MODIF_HEIGHTBAKE = 3,

  LAYER_ORDER_MAX = 16
};

class SplineObject : public RenderableEditableObject
{
protected:
  bool created;

public:
  bool splineInactive = false;

protected:
  ~SplineObject() override;

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
    bool usePerPointWidth;
    real maxWidthScale; // cached max of per-point scale_w, updated on edit/load
  };

  SplineObject(bool make_poly);

  void update(real dt) override {}
  void beforeRender() override {}
  void render() override {}
  void renderTrans() override {}
  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override;
  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override;
  bool getWorldBox(BBox3 &box) const override;
  void fillProps(PropPanel::ContainerPropertyControl &op, DClassID for_class_id,
    dag::ConstSpan<RenderableEditableObject *> objects) override;
  void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;
  void onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override;
  bool setName(const char *nm) override;
  void moveObject(const Point3 &delta, IEditorCoreEngine::BasisType basis) override;
  void rotateObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis) override;
  void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis) override;
  void putMoveUndo() override;
  void onRemove(ObjectEditor *) override;
  void onAdd(ObjectEditor *objEditor) override;
  Point3 getPos() const override { return splineCenter(); }

  EO_IMPLEMENT_RTTI(CID_HMapSplineObject);


  // rebuild bezierSpline / spline2d; both reconcile fillets first, so they may insert or remove generated points and renumber arrId:
  // do not hold points[] indices, iterators or sizes across these calls
  void getSpline();
  void getSplineXZ(BezierSpline2d &spline2d);
  // read-only export of current knots, no reconcile. A filleted spline exports its blend knots, so the DAG carries the
  // curve as drawn and a re-import gives that shape in real points, the way baking would
  void getSpline(DagSpline &spline);

  // knot whose handles the curve type collapses: a corner of a plain polygon, or a road junction. The fillet reconcile shares
  // this with getSpline() so a rule change cannot put the cuts off the rebuilt curve
  bool isPlainKnotType(const SplinePointObject *pt) const;
  // control knots a curve is built from: one per point, plus the closure duplicate of a polygon
  int knotCount() const { return points.size() + (poly ? 1 : 0); }
  enum KnotSrc
  {
    KNOT_CURVE,  // knots as drawn: a filleted corner reads the middle of its blend
    KNOT_SOURCE, // raw source handles: the base curve a fillet cuts into
  };
  // the three bezier control entries of one knot, collapsed on a plain knot. Every curve build shares it,
  // so the layout the fillet cuts are measured on cannot drift from the layout getSpline() rebuilds
  void getKnotControls(const SplinePointObject *pt, KnotSrc src, Point3 &out_in, Point3 &out_pos, Point3 &out_out) const;
  // curve segment that degenerates to a straight line, so the curve lies on the control polygon and drawing both hides it
  bool isStraightSeg(const SplinePointObject *a, const SplinePointObject *b) const;

  void updateRoadBox();
  void updateLoftBox();

  void getSmoothPoly(Tab<Point3> &pts);

  void gatherRoadsGeometry(StaticGeometryContainer &cont, int flags, bool collision, int stage);
  static void gatherStaticGeom(StaticGeometryContainer &cont, const StaticGeometryContainer &geom, int flags, int id, int name_idx1,
    int stage);

  void gatherLoftLandPts(Tab<Point3> &loft_pt_cloud, Tab<Point3> &water_border_polys, Tab<Point2> &hmap_sweep_polys);

  void renderLines(bool opaque_pass, const Frustum &frustum);
  void regenerateVBuf();

  void render(DynRenderBuffer *db, const TMatrix4 &gtm, const Point2 &s, const Frustum &frustum, int &cnt);

  bool getPosOnSpline(IGenViewportWnd *vp, int x, int y, float max_dist, int *out_segid = NULL, float *out_local_t = NULL,
    Point3 *out_p = NULL) const;

  void onPointRemove(int id);
  void addPoint(SplinePointObject *pt);

  // false when baking the fillets of the segment reshaped points[] and no point was inserted: seg_id means another
  // segment now, and so does every index measured on the old curve
  bool refine(int seg_id, real loc_t, Point3 &p_pos);
  void split(int pt_id);

  enum FilletSave
  {
    FILLET_KEEP, // source points and their filletR, for a reader that reconciles
    FILLET_BAKE, // the knots the curve runs through, for one that does not, like the composite spline service
  };
  void save(DataBlock &blk, FilletSave fillets = FILLET_KEEP);
  void load(const DataBlock &blk, bool use_undo);

  void regenerateObjects();
  void onCreated(bool gen = true);
  void reverse();
  bool shouldRenderRoadsGeom(const Frustum &) const;
  void renderRoadsGeom(bool opaque, const Frustum &);
  bool isSelfCross();
  void updateFullLoft();
  BBox3 getGeomBox();
  BBox3 getGeomBoxChanges();

  real getTotalLength();

  void putObjTransformUndo();

  void prepareSplineClassInPoints(bool report_missing_splcls = false);

  void pointChanged(int pt_idx);
  void recalcMaxPerPointWidth();
  void markModifChanged();
  void markModifChangedWhenUsed()
  {
    if (isAffectingHmap() || isHeightBake())
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

  static bool pointInsidePoly(const Point2 &p, dag::ConstSpan<Point3> points);
  bool pointInsidePoly(const Point2 &p);
  void applyHmapModifier(HeightMapStorage &hm, Point2 hm_ofs, float hm_cell_size, IBBox2 &out_dirty, const IBBox2 &dirty_clip,
    HmapBitmap *bmp = NULL);

  Point3 getProjPoint(const Point3 &p, real *proj_t = NULL);
  Point3 getPolyClosingProjPoint(Point3 &p, real *proj_t = NULL);
  SplinePointObject *getNearestPoint(Point3 &p);
  real getSplineTAtPoint(int id);

  void triangulatePoly();
  void splitOnTwoPolys(int pt1, int pt2);

  bool isDirReversed(Point3 &p1, Point3 &p2);

  SplineObject *clone();

  inline bool isClosed() const { return points.size() > 2 && points[0].get() == points.back().get(); }
  inline bool isPoly() const { return poly; }
  inline bool isCreated() const { return created; }
  inline bool isPolyHmapAlign() const { return props.poly.hmapAlign; }
  inline int getModifType() const { return props.modifType; }
  inline int getEffModifType() const { return poly ? (props.poly.hmapAlign ? MODIF_HMAP : MODIF_NONE) : props.modifType; }
  inline BezierSpline3d &getBezierSpline() { return bezierSpline; }
  inline const BezierSpline3d &getBezierSpline() const { return bezierSpline; }
  inline const char *getBlkGenName() const { return props.blkGenName; }
  inline real getPolyObjRot() const { return props.poly.objRot; }
  inline Point2 getPolyObjOffs() const { return props.poly.objOffs; }
  inline bool isExportable() const { return props.exportable; }
  inline bool isAffectingHmap() const { return isPoly() ? isPolyHmapAlign() : getModifType() == MODIF_HMAP; }
  inline bool isHeightBake() const { return !isPoly() && getModifType() == MODIF_HEIGHTBAKE; }

  inline void setPolyHmapAlign(bool a) { props.poly.hmapAlign = a; }
  inline void setModifType(int t) { props.modifType = t; }
  inline void setModifWidth(real w) { props.modifParams.width = w; }
  inline void setBlkGenName(const char *n) { props.blkGenName = n; }
  inline void setPolyObjRot(real rot) { props.poly.objRot = rot; }
  inline void setPolyObjOffs(Point2 offs) { props.poly.objOffs = offs; }
  inline void setExportable(bool ex) { props.exportable = ex; }
  inline void setCornerType(int t) { props.cornerType = t; }
  inline void setRandomSeed(int seed) { props.rndSeed = seed; }
  inline void setNavmeshIdx(int navmesh_idx) { props.navmeshIdx = navmesh_idx; }
  inline void setPolyAltGeom(bool a) { props.poly.altGeom = a; }
  inline void setPolyBboxAlignStep(float s) { props.poly.bboxAlignStep = s; }

  void loadModifParams(const DataBlock &blk);
  enum AttachAt
  {
    ATTACH_APPEND,
    ATTACH_BEFORE_LAST, // splice in front of the target's last point, the one being placed
  };
  // where to splice, not which index: attachTo strips the derived points of both splines first, so a slot measured
  // by the caller would address the array as it was before that
  void attachTo(SplineObject *s, AttachAt at = ATTACH_APPEND);

  Point3 splineCenter() const;
  bool intersects(const BBox2 &r, bool mark_segments);
  const BBox3 &getSplineBox() const { return splBox; }

  void reApplyModifiers(bool apply_now = true, bool force = false);

  bool onAssetChanged(landclass::AssetData *data);
  bool onAssetChanged(splineclass::AssetData *data);
  UndoRedoObject *makePointListUndo();

  void resetSplineClass();
  void changeAsset(const char *asset_name, bool put_undo);
  IPolygonGenObj *getPolyGen() const { return polyGenObj ? polyGenObj->queryInterface<IPolygonGenObj>() : nullptr; }
  const objgenerator::LandClassData *getLandClass() const { return getPolyGen() ? getPolyGen()->landClass : nullptr; }

  DagorAsset *getMaterialAsset(int idx) const;
  bool isUsingMaterial(const char *mat_name_to_find) const;

  void makeMonoUp();
  void makeMonoDown();
  void makeLinearHt();
  void applyCatmull(bool xz, bool y);

  // Non-destructive per-point fillet/chamfer: derived (generated) points are reconciled from filletR of source points before the curve
  // is built. Generated points live only in this->points (not in ObjectEditor) and never reach the scene save:
  // they are recomputed after any edit, undo or load.
  void updateFilletPoints();
  // strip before any structural edit of points[]: a split, a merge, a reverse or a point list swap addresses the array as
  // the user's point list and the indices it holds were counted on it. The curve rebuild that follows derives them again.
  // The strip lives here alone, but taking this route is the caller's discipline:
  // an index-addressing edit that skips it counts generated points as user points
  static void beforeStructuralEdit(SplineObject *s1, SplineObject *s2 = nullptr);
  bool hasFilletPoints() const;
  // converts the fillet of pt into real points, and rewrites the outer neighbours' handles to keep the shape.
  // Records undo itself, so the caller has to hold an open transaction: outside one every put() runs its restore at once.
  // The shape is kept, except an arc on a polygon, and at a smooth curvature neighbour, which cannot hold the subdivided
  // handle without becoming explicit:
  // real polygon points are plain knots, so it degrades to the polyline through them and says so in the console
  void bakeFilletPoint(SplinePointObject *pt);
  // bakes the fillets a segment touches so the segment can be edited in place;
  // keeps points[] layout, so a segment index stays valid across the call
  void bakeFilletsAtSegment(int seg_id);
  // unique user points: no generated fillet points, no closed-spline duplicate;
  // safe to transform while the points array is reshuffled by reconciliation
  void gatherSourcePoints(PtrTab<SplinePointObject> &out_pts) const;
  // points[] index <-> ordinal counted over non-generated entries. The ordinal skips the closure duplicate, the index
  // resolves onto it: re-adding the last point of a closed spline lands in front of the duplicate, which is where it belongs
  int sourcePointOrdinal(const SplinePointObject *pt) const;
  int sourcePointIndex(int ordinal) const;
  int sourcePointCount() const;
  // nearest source point in the given direction, or nullptr past the end of the array. Generated points sit between real
  // ones: they never form a cross, and they must not pull the auto tangent of a user point
  SplinePointObject *nextRealPoint(int from, int dir) const;
  // move undo for whole-spline ops; only source points have undoable state and an ObjectEditor to put the undo object into
  void putPointsMoveUndo();

private:
  void removeFilletPoints();

public:
  void setEditLayerIdx(int idx);
  int getEditLayerIdx() const { return editLayerIdx; }
  int lpIndex() const { return poly ? EditLayerProps::PLG : EditLayerProps::SPL; };

  int getRenderLayerIdx() const;

  // Moves the internal, generated entities to the appropriate layer.
  // Call it after generating them, and after anything that changes isHidden().
  void applyLayerIdxToEntities(bool use_render_layer = true);

  static void changeAsset(ObjectEditor &object_editor, dag::ConstSpan<RenderableEditableObject *> objects,
    const char *initially_selected_asset_name, bool is_poly);
  static int makeSplinesCrosses(dag::ConstSpan<SplineObject *> spls);

  PtrTab<SplinePointObject> points;
  IObjEntity *polyGenObj = nullptr;
  IObjEntity *csgGen;
  IBBox2 lastModifArea, lastModifAreaDet;

  Tab<cable_handle_t> cablesPool;

  bool splineChanged;
  bool modifChanged;
  bool forceDebugDrawWhenHidden = false;
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

    IPolygonGenObj::Props poly;

    short cornerType; // -1=polyline, 0=smooth 1st deriv., 1=smooth 2nd deriv.
    bool exportable;
    bool useForNavMesh;
    float navMeshStripeWidth;
    int navmeshIdx;

    String notes;
    int layerOrder;
    float scaleTcAlong;
  };
  const Props &getProps() const { return props; }
  const int getLayer() const { return min(LAYER_ORDER_MAX - 1, props.layerOrder); }

  bool hasCurve() const { return points.size() >= 2; }
  bool reachesGame() const { return isExportable() && !points.empty() && EditLayerProps::layerProps[editLayerIdx].exp; }
  bool isGameExportBlocked() const { return isExportable() && !points.empty() && !EditLayerProps::layerProps[editLayerIdx].exp; }
  static bool isGameSplinesViewOn();

  static int splineSubtypeMask;
  static int tiledByPolygonSubTypeId;
  static int splineSubTypeId;
  static int roadsSubtypeMask;
  static int gameSplineSubtypeMask;

  static bool isSplineCacheValid;
  static bool objectWasMoved, objectWasRotated, objectWasScaled;
  static bool isSplineObjectsAreLoading;

protected:
  Props props;
  int editLayerIdx = 0;

  void createMaterialControls(PropPanel::ContainerPropertyControl &op);

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
  bool updatingFillets = false;

  SplineObject *flattenBySpline;

  BSphere3 splSph;
  BBox3 splBox, splBoxPrev;
  BBox3 loftBox, roadBox;
  BBox3 geomBoxPrev;
  float splStep;

  DebugPrimitivesVbuffer *bezierBuf;
};
