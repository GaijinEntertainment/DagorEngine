// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "roadUtil.h"
#include <de3_entityFilter.h>

#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <generic/dag_sort.h>
#include <EditorCore/ec_IEditorCore.h>
#include <debug/dag_debug.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <math/dag_math2d.h>
#include "common.h"

#include "hmlPlugin.h"
#include "hmlCm.h"
#include <de3_genObjData.h>
#include <de3_genObjAlongSpline.h>
#include <de3_genObjInsidePoly.h>
#include <de3_rasterizePoly.h>
#include <de3_genObjByDensGridMask.h>
#include <de3_splineClassData.h>
#include <de3_hmapService.h>
// #include <util/dag_hierBitMap2d.h>
#include <libTools/dagFileRW/splineShape.h>
#include <math/dag_sphereVis.h>
#include <util/dag_fastIntList.h>

#include "hmlSplineUndoRedo.h"

#include <propPanel/control/container.h>
#include <propPanel/control/panelWindow.h>

#include <stdlib.h>
#include <shaders/dag_overrideStates.h>

#include <assets/asset.h>
#include <obsolete/dag_cfg.h>

using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;

#define RENDER_SPLINE_POINTS              1000
#define RENDER_SPLINE_POINTS_MIN          10
#define RENDER_SPLINE_POINTS_LENGTH_COEFF 5

class RenderableInstanceLodsResource;
class RenderableInstanceResource;

enum
{
  PID_GENBLKNAME = 1,
  PID_GENBLKNAME2,

  PID_SPLINE_LAYERS,

  PID_NOTES,
  PID_SPLINE_LEN,
  PID_TC_ALONG_MUL,
  PID_LAYER_ORDER,
  PID_MAY_SELF_CROSS,

  PID_EXPORTABLE,
  PID_ALT_GEOM,
  PID_BBOX_ALIGN_STEP,

  PID_CORNER_TYPE,

  PID_MODIF_GRP,
  PID_MODIFTYPE_S,
  PID_MODIFTYPE_P,
  PID_MODIF_WIDTH,
  PID_MODIF_SMOOTH,
  PID_MODIF_CT_OFFSET,
  PID_MODIF_SD_OFFSET,
  PID_MODIF_OFFSET_POW,
  PID_MODIF_ADDITIVE,

  PID_POLY_OFFSET,
  PID_POLY_ROTATE,
  PID_POLY_SMOOTH,
  PID_POLY_CURV,
  PID_POLY_MINSTEP,
  PID_POLY_MAXSTEP,

  PID_MONOTONOUS_Y_UP,
  PID_MONOTONOUS_Y_DOWN,
  PID_LINEARIZE_Y,
  PID_CATMUL_XYZ,
  PID_CATMUL_XZ,
  PID_CATMUL_Y,

  PID_FLATTEN_Y,
  PID_FLATTEN_AVERAGE,
  PID_FLATTEN_SPLINE_GROUP,
  PID_FLATTEN_SPLINE,
  PID_FLATTEN_SPLINE_APPLY,

  PID_OPACGEN_BASE,
  PID_OPACGEN_VAR,
  PID_OPACGEN_APPLY,

  PID_USE_FOR_NAVMESH,
  PID_NAVMESH_STRIPE_WIDTH,

  PID_PERINST_SEED,
  PID_PER_MATERIAL_CONTROLS_BEGIN,
};

enum
{
  MATERIAL_PID_GROUP,
  MATERIAL_PID_COLOR,
  MATERIAL_PID_SAVEMAT_BTN,
  MATERIAL_PID_COUNT
};

#define DEF_USE_FOR_NAVMESH      false
#define DEF_NAVMESH_STRIPE_WIDTH 20.0f

int SplineObject::polygonSubtypeMask = -1;
int SplineObject::splineSubtypeMask = -1;
int SplineObject::tiledByPolygonSubTypeId = -1;
int SplineObject::roadsSubtypeMask = -1;
int SplineObject::splineSubTypeId = -1;
bool SplineObject::isSplineCacheValid = false;

static float opac_gen_base = 1, opac_gen_var = 0;

static int spline_unique_buf_idx = 0;

//==================================================================================================
SplineObject::SplineObject(bool make_poly) : points(tmpmem), poly(make_poly), landClass(NULL), segSph(midmem)
{
  isSplineCacheValid = false;
  csgGen = NULL;
  splineChanged = true;
  modifChanged = false;
  roadBox.setempty();
  loftBox.setempty();

  clear_and_shrink(points);
  clear_and_shrink(segSph);

  created = false;

  props.rndSeed = grnd();
  props.perInstSeed = 0;
  props.maySelfCross = false;
  props.modifType = MODIF_NONE;
  lastModifArea.setEmpty();
  lastModifAreaDet.setEmpty();

  props.modifParams.offset = Point2(-0.05, 0);
  props.modifParams.width = 60;
  props.modifParams.smooth = 10;
  props.modifParams.offsetPow = 2;
  props.modifParams.additive = false;

  props.poly.hmapAlign = false;
  props.poly.objRot = 0;
  props.poly.objOffs = Point2(0, 0);
  props.poly.smooth = false;
  props.poly.curvStrength = 1;
  props.poly.minStep = 10;
  props.poly.maxStep = 100;

  props.cornerType = 0;
  props.exportable = false;
  props.layerOrder = 0;
  props.scaleTcAlong = 1;
  props.useForNavMesh = DEF_USE_FOR_NAVMESH;
  props.navMeshStripeWidth = DEF_NAVMESH_STRIPE_WIDTH;
  props.navmeshIdx = -1;

  firstApply = true;
  flattenBySpline = NULL;

  if (polygonSubtypeMask == -1)
    polygonSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("poly_tile");
  if (splineSubtypeMask == -1)
    splineSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("spline_cls");
  // debug ( "polygonSubtypeMask=%p splineSubtypeMask=%p", polygonSubtypeMask, splineSubtypeMask);
  if (tiledByPolygonSubTypeId == -1)
    tiledByPolygonSubTypeId = IDaEditor3Engine::get().registerEntitySubTypeId("poly_tile");

  if (splineSubTypeId == -1)
    splineSubTypeId = IDaEditor3Engine::get().registerEntitySubTypeId("spline_cls");

  if (roadsSubtypeMask == -1)
    roadsSubtypeMask = 1 << IDaEditor3Engine::get().registerEntitySubTypeId("road_obj");

  shaders::OverrideState zFuncLessState;
  zFuncLessState.set(shaders::OverrideState::Z_FUNC);
  zFuncLessState.zFunc = CMPF_LESS;
  zFuncLessStateId = dagGeom->create(zFuncLessState);

  bezierBuf = dagRender->newDebugPrimitivesVbuffer(String(0, "bezier_splines_%d_vbuf", spline_unique_buf_idx++), midmem);
}

SplineObject::~SplineObject()
{
  dagGeom->destroy(zFuncLessStateId);
  destroy_it(csgGen);
  del_it(landClass);
  clear_and_shrink(points);
  polyGeom.clear();

  segSph.clear();
  isSplineCacheValid = false;

  dagRender->deleteDebugPrimitivesVbuffer(bezierBuf);
}

void SplineObject::prepareSplineClassInPoints()
{
  if (poly)
  {
    if (props.blkGenName.empty())
    {
      destroy_it(csgGen);
      del_it(landClass);
      polyGeom.clear();
    }
    else if (!landClass)
      landClass = new objgenerator::LandClassData(props.blkGenName);
  }

  if (points.size() < 1)
    return;

  const char *splineClassAssetName = !poly ? props.blkGenName.str() : NULL;

  if (poly && landClass && landClass->data && landClass->data->splineClassAssetName.length())
    splineClassAssetName = landClass->data->splineClassAssetName;

  splineclass::AssetData *a = points[0]->prepareSplineClass(splineClassAssetName, NULL);
  int pnum = isClosed() ? points.size() - 1 : points.size();
  for (int pi = 1; pi < pnum; ++pi)
    a = points[pi]->prepareSplineClass(NULL, a);

  // erase old objects in last segment (only when spline is not circular!)
  if (!isClosed())
    (points.back())->clearSegment();
}

void SplineObject::resetSplineClass() { del_it(landClass); }

//==================================================================================================
void SplineObject::getSpline()
{
  PropPanel::ContainerPropertyControl *pw = ((HmapLandObjectEditor *)getObjEditor())->getCurrentPanelFor(this);
  if (!points.size())
  {
    const char *str = "Length: 0.0 m";
    if (pw && strcmp(pw->getText(PID_SPLINE_LEN), str) != 0)
      pw->setText(PID_SPLINE_LEN, str);
    return;
  }

  isSplineCacheValid = false;

  int pts_num = points.size() + (poly ? 1 : 0);
  if (poly && props.poly.smooth)
    applyCatmull(true, false);

  SmallTab<Point3, TmpmemAlloc> pts;
  clear_and_resize(pts, pts_num * 3);

  for (int pi = 0; pi < pts_num; ++pi)
  {
    int wpi = pi % points.size();
    pts[pi * 3 + 1] = points[wpi]->getPt();
    if ((poly && !props.poly.smooth) || (points[wpi]->isCross && points[wpi]->isRealCross))
      pts[pi * 3 + 2] = pts[pi * 3] = pts[pi * 3 + 1];
    else
    {
      pts[pi * 3] = points[wpi]->getBezierIn();
      pts[pi * 3 + 2] = points[wpi]->getBezierOut();
    }
  }

  bezierSpline.calculate(pts.data(), pts.size(), false);

  float sln = bezierSpline.getLength();
  splStep = sln ? 0.1 / sln : 1.0;
  if (splStep < 0.004)
    splStep = 0.004;

  segSph.resize(bezierSpline.segs.size());

  for (int i = 0; i < pts_num - 1; i++)
  {
    real startT = 0, endT = 0;

    for (int j = 0; j < i; j++)
      startT += bezierSpline.segs[j].len;

    for (int j = 0; j < i + 1; j++)
      endT += bezierSpline.segs[j].len;

    BBox3 bb;
    for (real t = startT; t < endT; t += sln / RENDER_SPLINE_POINTS)
      bb += bezierSpline.get_pt(t);

    segSph[i] = bb;
  }

  splBox.setempty();
  for (int i = 0; i < points.size(); i++)
    splBox += points[i]->getPt();
  for (int i = 0; i < segSph.size(); i++)
    splBox += segSph[i];
  splBox[0] -= Point3(1, 1, 1);
  splBox[1] += Point3(1, 1, 1);

  splSph = splBox;
  if (pw)
  {
    String str(128, "Length: %.1f m", bezierSpline.getLength());
    if (strcmp(pw->getText(PID_SPLINE_LEN), str) != 0)
      pw->setText(PID_SPLINE_LEN, str);
  }
}

void SplineObject::getSplineXZ(BezierSpline2d &spline2d)
{
  if (!points.size())
    return;
  int pts_num = points.size() + (poly ? 1 : 0);
  if (poly && props.poly.smooth)
    applyCatmull(true, false);

  SmallTab<Point2, TmpmemAlloc> pts;
  clear_and_resize(pts, pts_num * 3);

  for (int pi = 0; pi < pts_num; ++pi)
  {
    int wpi = pi % points.size();
    Point3 p = points[wpi]->getPt();
    pts[pi * 3 + 1] = Point2(p.x, p.z);
    if ((poly && !props.poly.smooth) || (points[wpi]->isCross && points[wpi]->isRealCross))
      pts[pi * 3 + 2] = pts[pi * 3] = pts[pi * 3 + 1];
    else
    {
      Point3 pin = points[wpi]->getBezierIn();
      Point3 pout = points[wpi]->getBezierOut();
      pts[pi * 3] = Point2(pin.x, pin.z);
      pts[pi * 3 + 2] = Point2(pout.x, pout.z);
    }
  }

  spline2d.calculate(pts.data(), pts.size(), false);
}

void SplineObject::getSpline(DagSpline &spline)
{
  spline.closed = 0;
  spline.knot.resize(points.size());

  for (int i = 0; i < points.size(); ++i)
    if (points[i])
    {
      spline.knot[i].i = points[i]->getBezierIn();
      spline.knot[i].p = points[i]->getPt();
      spline.knot[i].o = points[i]->getBezierOut();
    }
}


static void gatherGeomTags(GeomObject *g, OAHashNameMap<true> &tags)
{
  int nnum = g->getGeometryContainer()->nodes.size();
  for (int ni = 0; ni < nnum; ++ni)
    if (StaticGeometryNode *node = g->getGeometryContainer()->nodes[ni])
      if (const char *t = node->script.getStr("layerTag", NULL))
        tags.addNameId(t);
}
void SplineObject::gatherPolyGeomLoftTags(OAHashNameMap<true> &loft_tags)
{
  if (splineInactive)
    return;
  if (polyGeom.mainMesh)
    gatherGeomTags(polyGeom.mainMesh, loft_tags);
  if (polyGeom.borderMesh)
    gatherGeomTags(polyGeom.borderMesh, loft_tags);
}

//==================================================================================================
void SplineObject::updateLoftBox()
{
  loftBox.setempty();
  for (int i = 0; i < points.size(); ++i)
    loftBox += points[i]->getGeomBox(true, false);
}
void SplineObject::updateRoadBox()
{
  roadBox.setempty();
  for (int i = 0; i < points.size(); ++i)
    roadBox += points[i]->getGeomBox(false, true);
}

BBox3 SplineObject::getGeomBox()
{
  BBox3 worldBox;
  if (created)
    worldBox = splBox;
  else
    worldBox.setempty();
  worldBox += loftBox;
  worldBox += roadBox;
  if (geomBoxPrev != worldBox)
    geomBoxPrev = worldBox;
  return worldBox;
}

BBox3 SplineObject::getGeomBoxChanges()
{
  BBox3 prev = geomBoxPrev;
  BBox3 worldBox = getGeomBox();
  geomBoxPrev = worldBox;
  worldBox += prev;
  return worldBox;
}

static inline int hide_non_stage0_nodes(GeomObject &gobj)
{
  StaticGeometryContainer *cont = gobj.getGeometryContainer();
  if (!cont)
    return 0;
  int nnum = cont->nodes.size();
  bool changed = false;

  for (int ni = 0; ni < nnum; ++ni)
    if (StaticGeometryNode *node = cont->nodes[ni])
      if (node->script.getInt("stage", 0) != 0)
      {
        changed = true;
        node->flags |= StaticGeometryNode::FLG_OPERATIVE_HIDE;
      }
  return changed ? nnum : 0;
}
static inline void reset_hide_non_stage0_nodes(GeomObject &gobj, int nnum)
{
  if (!nnum)
    return;
  StaticGeometryContainer *cont = gobj.getGeometryContainer();
  for (int ni = 0; ni < nnum; ++ni)
    if (StaticGeometryNode *node = cont->nodes[ni])
      node->flags &= ~StaticGeometryNode::FLG_OPERATIVE_HIDE;
}

void SplineObject::renderGeom(bool opaque, int layer, const Frustum &frustum)
{
  if (EditLayerProps::layerProps[getEditLayerIdx()].hide)
    return;

  int st_mask = IDaEditor3Engine::get().getEntitySubTypeMask(IObjEntityFilter::STMASK_TYPE_RENDER);

  if (poly && landClass && landClass->data && landClass->data->genGeom && (st_mask & polygonSubtypeMask))
  {
    if (polyGeom.mainMesh)
    {
      int nnum = hide_non_stage0_nodes(*polyGeom.mainMesh);
      if (opaque)
        dagGeom->geomObjectRender(*polyGeom.mainMesh);
      else
        dagGeom->geomObjectRenderTrans(*polyGeom.mainMesh);
      reset_hide_non_stage0_nodes(*polyGeom.mainMesh, nnum);
    }

    if (polyGeom.borderMesh)
    {
      int nnum = hide_non_stage0_nodes(*polyGeom.borderMesh);
      if (opaque)
        dagGeom->geomObjectRender(*polyGeom.borderMesh);
      else
        dagGeom->geomObjectRenderTrans(*polyGeom.borderMesh);
      reset_hide_non_stage0_nodes(*polyGeom.borderMesh, nnum);
    }
  }

  if (!created)
    return;

  if (!(st_mask & (splineSubtypeMask | roadsSubtypeMask)))
    return;

  for (int i = 0; i < points.size(); ++i)
  {
    if (!frustum.testBoxB(points[i]->getGeomBox(false, st_mask & roadsSubtypeMask)))
      continue;
    if (st_mask & roadsSubtypeMask)
      points[i]->renderRoadGeom(opaque);
  }
}


void SplineObject::renderLines(bool opaque_pass, const Frustum &frustum)
{
  if (!((HmapLandObjectEditor *)getObjEditor())->isEditingSpline(this) && !frustum.testBoxB(getGeomBox()))
    return;
  if (splineInactive)
    return;
  if (!points.size())
    return;
  if (EditLayerProps::layerProps[getEditLayerIdx()].hide)
    return;
  bool lock = (EditLayerProps::layerProps[getEditLayerIdx()].lock);

  if (HmapLandPlugin::self->renderDebugLines && opaque_pass && !lock)
    if (created && landClass && landClass->data && landClass->data->genGeom)
      polyGeom.renderLines();

  E3DCOLOR col;
  if (isSelected())
    col = E3DCOLOR(255, 255, 255, 255);
  else if (lock)
    col = E3DCOLOR(128, 128, 128, 255);
  else
  {
    col = poly ? (props.navmeshIdx >= 0 ? E3DCOLOR(14, 9, 245, 255) : E3DCOLOR(0, 200, 0, 255)) : E3DCOLOR(170, 170, 170, 255);
    for (int i = 0; i < points.size(); i++)
      if (points[i]->isSelected())
      {
        col = E3DCOLOR(170, 200, 200, 255);
        break;
      }
  }

  if (opaque_pass && points.size() > 1 && (isSelected() || (poly && !props.poly.smooth) || props.cornerType == -1))
  {
    E3DCOLOR col2 = E3DCOLOR(200, 0, 0, 255);
    if ((poly && !props.poly.smooth) || props.cornerType == -1)
      col2 = col;
    else if (poly && props.poly.smooth)
      col2 = E3DCOLOR(0, 100, 0, 255);

    for (int i = 0; i < (int)points.size() - 1; i++)
      dagRender->renderLine(points[i]->getPt(), points[i + 1]->getPt(), col2);

    if (poly && !props.poly.smooth && created)
      dagRender->renderLine(points.back()->getPt(), points[0]->getPt(), col);
  }

  if (!poly && opaque_pass && props.cornerType == 1 && !lock)
    for (int i = 0; i < points.size(); i++)
      if (points[i]->isSelected())
        dagRender->renderLine(points[i]->getBezierIn(), points[i]->getBezierOut(), E3DCOLOR(0, 200, 100, 255));

  if ((!poly || props.poly.smooth) && props.cornerType != -1)
  {
    if (dagRender->isLinesVbufferValid(*bezierBuf))
    {
      if (!opaque_pass)
        col = E3DCOLOR(col.r * 3 / 5, col.g * 3 / 5, col.b * 3 / 5, 64);
      dagRender->renderLinesFromVbuffer(*bezierBuf, col);
    }
  }
}

void SplineObject::regenerateVBuf()
{
  dagRender->invalidateDebugPrimitivesVbuffer(*bezierBuf);
  real splineLen = bezierSpline.getLength();

  int renderSplinePoints = int(splineLen * RENDER_SPLINE_POINTS_LENGTH_COEFF);
  if (renderSplinePoints > RENDER_SPLINE_POINTS)
    renderSplinePoints = RENDER_SPLINE_POINTS;
  else if (renderSplinePoints < RENDER_SPLINE_POINTS_MIN)
    renderSplinePoints = RENDER_SPLINE_POINTS_MIN;

  Tab<Point3> splinePoly(tmpmem);
  splinePoly.reserve(renderSplinePoints + 1);

  for (real t = 0.0; t < splineLen; t += splineLen / renderSplinePoints)
    splinePoly.push_back(bezierSpline.get_pt(t));

  E3DCOLOR col(255, 255, 255, 255);
  dagRender->beginDebugLinesCacheToVbuffer(*bezierBuf);
  for (int i = 0; i < (int)splinePoly.size() - 1; ++i)
    dagRender->addLineToVbuffer(*bezierBuf, splinePoly[i], splinePoly[i + 1], col);
  dagRender->endDebugLinesCacheToVbuffer(*bezierBuf);
}

void SplineObject::render(DynRenderBuffer *db, const TMatrix4 &gtm, const Point2 &s, const Frustum &frustum, int &cnt)
{
  if (splineInactive)
    return;
  if (!points.size())
    return;
  if (EditLayerProps::layerProps[getEditLayerIdx()].hide)
    return;
  if (EditLayerProps::layerProps[getEditLayerIdx()].lock)
    return;

  if (!((HmapLandObjectEditor *)getObjEditor())->isEditingSpline(this) && !frustum.testBoxB(getGeomBox()))
    return;

  bool ortho = DAGORED2->getRenderViewport()->isOrthogonal();
  Point3 cpos = dagRender->curView().pos;
  float dist2 = getObjEd().maxPointVisDist;
  dist2 *= dist2;

  cnt++;
  if (ortho || lengthSq(points[0]->getPt() - cpos) < dist2)
    points[0]->renderPts(*db, gtm, s, !poly);

  for (int i = 1; i < points.size(); i++)
  {
    if (!ortho && lengthSq(points[i]->getPt() - cpos) > dist2)
      continue;

    points[i]->renderPts(*db, gtm, s);

    cnt++;
    if (cnt >= 4096)
    {
      dagGeom->set(zFuncLessStateId);
      dagGeom->shaderGlobalSetInt(SplinePointObject::ptRenderPassId, 0);
      dagRender->dynRenderBufferFlushToBuffer(*db, SplinePointObject::texPt);
      dagGeom->reset_override();

      dagGeom->shaderGlobalSetInt(SplinePointObject::ptRenderPassId, 1);
      dagRender->dynRenderBufferFlushToBuffer(*db, SplinePointObject::texPt);
      dagRender->dynRenderBufferClearBuf(*db);
      cnt = 0;
    }
  }
}


void SplineObject::fillProps(PropPanel::ContainerPropertyControl &op, DClassID for_class_id,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  PropPanel::ContainerPropertyControl *commonGrp = op.getContainerById(PID_COMMON_GROUP);

  int one_type_idx = -1;
  int one_layer = -1;

  bool targetLayerEnabled = true;
  for (int i = 0; i < objects.size(); ++i)
    if (SplineObject *s = RTTI_cast<SplineObject>(objects[i]))
    {
      int type_idx = s->isPoly() ? EditLayerProps::PLG : EditLayerProps::SPL;
      if (one_type_idx == type_idx || one_type_idx == -1)
        one_type_idx = type_idx;
      else
      {
        one_type_idx = -2;
        targetLayerEnabled = false;
      }

      if (one_layer == -1)
        one_layer = s->getEditLayerIdx();
      else if (one_layer != s->getEditLayerIdx())
        one_layer = -2;
    }
    else
    {
      one_layer = -2;
      one_type_idx = -2;
      targetLayerEnabled = false;
      break;
    }

  int targetLayerValue = 0;
  Tab<String> targetLayers(tmpmem);
  if (one_type_idx >= 0)
  {
    targetLayerValue = EditLayerProps::findTypeIdx(one_type_idx, one_layer);
    getObjEditor()->getLayerNames(one_type_idx, targetLayers);
    targetLayerEnabled = targetLayers.size() > 1;
  }
  if (one_layer < 0 || one_type_idx < 0)
  {
    targetLayerValue = targetLayers.size();
    targetLayers.push_back(String("-- (mixed) --"));
  }

  commonGrp->createStatic(-1, "Edit layer:");
  commonGrp->createCombo(PID_SPLINE_LAYERS, "", targetLayers, targetLayerValue, targetLayerEnabled);

  if (one_type_idx >= 0)
  {
    commonGrp->createStatic(PID_SPLINE_LEN, String(128, "Length: %.1f m", bezierSpline.getLength()));
    commonGrp->createEditBox(PID_NOTES, "Notes", props.notes);
    commonGrp->createEditFloat(PID_TC_ALONG_MUL, "Scale TC along spline", props.scaleTcAlong);

    Tab<String> lorders(tmpmem);
    for (int i = 0; i < LAYER_ORDER_MAX; ++i)
      lorders.push_back(String(8, "%d", i));
    commonGrp->createCombo(PID_LAYER_ORDER, "Place layer order", lorders, props.layerOrder);

    String title(poly ? "Land class asset" : "Spline class asset");

    commonGrp->createIndent();
    commonGrp->createStatic(-1, title);
    commonGrp->createButton(PID_GENBLKNAME, ::dd_get_fname(props.blkGenName));
    if (poly)
    {
      commonGrp->createStatic(-1, "Border spline class asset");
      commonGrp->createButton(PID_GENBLKNAME2, ::dd_get_fname(points[0]->getProps().blkGenName));
    }

    if (!poly)
    {
      commonGrp->createIndent();
      commonGrp->createCheckBox(PID_MAY_SELF_CROSS, "Self cross allowed", props.maySelfCross);
    }

    if (poly)
    {
      commonGrp->createCheckBox(PID_ALT_GEOM, "Alternative geom export", polyGeom.altGeom);
      commonGrp->createEditFloat(PID_BBOX_ALIGN_STEP, "geom bbox align step", polyGeom.bboxAlignStep);
      commonGrp->setEnabledById(PID_BBOX_ALIGN_STEP, polyGeom.altGeom);
      commonGrp->createSeparator(0);
    }
    commonGrp->createCheckBox(PID_EXPORTABLE, "Export to game", props.exportable);

    commonGrp->createCheckBox(PID_USE_FOR_NAVMESH, "Use for NavMesh", props.useForNavMesh);
    commonGrp->createEditFloat(PID_NAVMESH_STRIPE_WIDTH, "Width", props.navMeshStripeWidth);

    createMaterialControls(op);

    PropPanel::ContainerPropertyControl &cornerGrp = *commonGrp->createRadioGroup(PID_CORNER_TYPE, "Spline knots corner type");
    cornerGrp.createRadio(-1, "Corner");
    cornerGrp.createRadio(0, "Smooth tangent");
    cornerGrp.createRadio(1, "Smooth curvature");
    commonGrp->setInt(PID_CORNER_TYPE, props.cornerType);

    if (objects.size() == 1)
    {
      PropPanel::ContainerPropertyControl *seedGrp = getObjEditor()->createPanelGroup(RenderableEditableObject::PID_SEED_GROUP);
      seedGrp->createTrackInt(PID_PERINST_SEED, "Per-instance seed", props.perInstSeed, 0, 32767, 1);
    }

    PropPanel::ContainerPropertyControl *modGroup = op.createGroup(PID_MODIF_GRP, "Modify");

    if (!poly)
    {
      Tab<String> names(tmpmem);
      names.resize(3);

      names[0] = "No modify";
      names[1] = "Modify self";
      names[2] = "Modify heightmap";

      modGroup->createCombo(PID_MODIFTYPE_S, "Modify type", names, props.modifType);
    }
    else
    {
      Tab<String> names(tmpmem);
      names.resize(2);

      names[0] = "No modify";
      names[1] = "Modify heightmap";

      modGroup->createCombo(PID_MODIFTYPE_P, "Modify type", names, props.poly.hmapAlign ? 1 : 0);
    }

    modGroup->createEditFloat(PID_MODIF_WIDTH, "Width", props.modifParams.width);
    modGroup->createEditFloat(PID_MODIF_SMOOTH, "Smooth", props.modifParams.smooth);
    modGroup->createEditFloat(PID_MODIF_CT_OFFSET, "Ht center ofs", props.modifParams.offset[0]);
    modGroup->createEditFloat(PID_MODIF_SD_OFFSET, "Ht side ofs", props.modifParams.offset[1]);
    modGroup->createEditFloat(PID_MODIF_OFFSET_POW, "Ht ofs function", props.modifParams.offsetPow);
    modGroup->createCheckBox(PID_MODIF_ADDITIVE, "Preserve land details", props.modifParams.additive);

    if (poly)
    {
      op.createPoint2(PID_POLY_OFFSET, "Gen. objects offset", props.poly.objOffs);
      op.createEditFloat(PID_POLY_ROTATE, "Gen. objects rotation", props.poly.objRot);
      op.createCheckBox(PID_POLY_SMOOTH, "Smooth contour", props.poly.smooth);
      op.createEditFloat(PID_POLY_CURV, "Curv. strength", props.poly.curvStrength);
      op.createEditFloat(PID_POLY_MINSTEP, "Min step", props.poly.minStep);
      op.createEditFloat(PID_POLY_MAXSTEP, "Max step", props.poly.maxStep);
    }

    op.createButton(PID_MONOTONOUS_Y_UP, "Monotonous points height (up)");
    op.createButton(PID_MONOTONOUS_Y_DOWN, "Monotonous points height (down)");
    op.createButton(PID_LINEARIZE_Y, "Linearize points height");
    op.createButton(PID_CATMUL_XYZ, "Catmull-Rom smooth (XYZ)");
    op.createButton(PID_CATMUL_XZ, "Catmull-Rom smooth (XZ)");
    op.createButton(PID_CATMUL_Y, "Catmull-Rom smooth (Y)");

    op.createButton(PID_FLATTEN_Y, "Flatten Y");
    op.createButton(PID_FLATTEN_AVERAGE, "Flatten average");

    if (poly)
    {
      PropPanel::ContainerPropertyControl *flGroup = op.createGroup(PID_FLATTEN_SPLINE_GROUP, "Flatten by spline");
      Tab<SplineObject *> spls(tmpmem);

      for (int i = 0; i < ((HmapLandObjectEditor *)getObjEditor())->splinesCount(); i++)
      {
        SplineObject *ss = ((HmapLandObjectEditor *)getObjEditor())->getSpline(i);
        if (!ss->poly && ss->created)
          spls.push_back(ss);
      }

      if (spls.size())
      {
        Tab<String> flatSplines(tmpmem);
        flatSplines.resize(spls.size() + 1);

        flatSplines[0] = "none";
        String curs(flatSplines[0]);
        int idx = 0;

        for (int i = 0; i < spls.size(); i++)
        {
          flatSplines[i + 1] = spls[i]->getName();
          if (spls[i] == flattenBySpline)
            curs = flatSplines[i + 1];
          idx = i + 1;
        }

        flGroup->createCombo(PID_FLATTEN_SPLINE, "Spline", flatSplines, idx);
        flGroup->createButton(PID_FLATTEN_SPLINE_APPLY, "Apply");
      }
    }

    op.createSeparator(0);
    op.createStatic(-1, "Opacity generation formula:");
    op.createStatic(-1, "   base + variation * Rnd(-1..+1)");
    op.createEditFloat(PID_OPACGEN_BASE, "Base opacity", opac_gen_base);
    op.createEditFloat(PID_OPACGEN_VAR, "Max opacity variation", opac_gen_var);
    op.createButton(PID_OPACGEN_APPLY, "Generate opacity in points!");
  }

  for (int i = 0; i < objects.size(); ++i)
  {
    SplineObject *o = RTTI_cast<SplineObject>(objects[i]);
    if (!o)
      continue;

    if (o->props.notes != props.notes)
      op.setText(PID_NOTES, "");
    if (stricmp(o->props.blkGenName, props.blkGenName) != 0)
      op.setCaption(PID_GENBLKNAME, "");
    if (o->poly != poly || stricmp(o->points[0]->getProps().blkGenName, points[0]->getProps().blkGenName) != 0)
      op.setCaption(PID_GENBLKNAME2, "");
    if (o->getEffModifType() != getEffModifType())
      op.resetById(poly ? PID_MODIFTYPE_P : PID_MODIFTYPE_S);
    if (o->props.cornerType != props.cornerType)
      op.resetById(PID_CORNER_TYPE);
  }
}

void SplineObject::setEditLayerIdx(int idx)
{
  editLayerIdx = idx;
  for (SplinePointObject *p : points)
    p->setEditLayerIdx(editLayerIdx);
}

void SplineObject::onLayerOrderChanged()
{
  for (SplinePointObject *p : points)
    if (auto *gen = p->getSplineGen())
      gen->splineLayer = props.layerOrder;
}

void SplineObject::onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  if (!edit_finished)
    return;

#define CHANGE_VAL(type, pname, getfunc)                     \
  {                                                          \
    type val(panel.getfunc(pid));                            \
    for (int i = 0; i < objects.size(); ++i)                 \
    {                                                        \
      SplineObject *o = RTTI_cast<SplineObject>(objects[i]); \
      if (!o || o->pname == val)                             \
        continue;                                            \
      o->pname = val;                                        \
    }                                                        \
  }
#define CHANGE_VAL_FUNC(type, pname, getfunc, func)          \
  {                                                          \
    type val(panel.getfunc(pid));                            \
    for (int i = 0; i < objects.size(); ++i)                 \
    {                                                        \
      SplineObject *o = RTTI_cast<SplineObject>(objects[i]); \
      if (!o || o->pname == val)                             \
        continue;                                            \
      o->pname = val;                                        \
      o->func;                                               \
    }                                                        \
  }

  if (pid == PID_NOTES)
  {
    CHANGE_VAL(String, props.notes, getText)
    DAGORED2->invalidateViewportCache();
  }

  if (pid == PID_SPLINE_LAYERS)
  {
    int oneTypeIdx = -1;
    for (int i = 0; i < objects.size(); ++i)
    {
      if (SplineObject *s = RTTI_cast<SplineObject>(objects[i]))
      {
        int typeIdx = s->isPoly() ? EditLayerProps::PLG : EditLayerProps::SPL;
        if (oneTypeIdx == typeIdx || oneTypeIdx == -1)
        {
          oneTypeIdx = typeIdx;
          continue;
        }
      }
      break;
    }

    int selectedLayer = panel.getInt(PID_SPLINE_LAYERS);
    int layerIdx = EditLayerProps::findLayerByTypeIdx(oneTypeIdx, selectedLayer);
    if (layerIdx != -1)
    {
      dag::Vector<RenderableEditableObject *> objectsToMove;
      objectsToMove.set_capacity(objects.size());
      objectsToMove.insert(objectsToMove.end(), objects.begin(), objects.end());
      HmapLandPlugin::self->moveObjectsToLayer(layerIdx, make_span(objectsToMove));

      DAGORED2->invalidateViewportCache();
      getObjEditor()->invalidateObjectProps();
    }
  }

  if (pid == PID_TC_ALONG_MUL)
  {
    CHANGE_VAL_FUNC(float, props.scaleTcAlong, getFloat, invalidateSplineCurve());
    DAGORED2->invalidateViewportCache();
    HmapLandPlugin::hmlService->invalidateClipmap(true);
  }

  if (pid == PID_LAYER_ORDER)
  {
    CHANGE_VAL_FUNC(int, props.layerOrder, getInt, onLayerOrderChanged())
    // rebuild splines here
    DAGORED2->invalidateViewportCache();
    HmapLandPlugin::hmlService->invalidateClipmap(true);
  }

  if (pid == PID_PERINST_SEED)
  {
    CHANGE_VAL_FUNC(int, props.perInstSeed, getInt, regenerateObjects())
    DAGORED2->repaint();
  }

  if (pid > PID_PER_MATERIAL_CONTROLS_BEGIN)
  {
    int materialPid = (pid - PID_PER_MATERIAL_CONTROLS_BEGIN) % MATERIAL_PID_COUNT;
    int materialIdx = (pid - PID_PER_MATERIAL_CONTROLS_BEGIN) / MATERIAL_PID_COUNT;

    if (materialPid == MATERIAL_PID_COLOR)
    {
      DagorAsset *materialAsset = getMaterialAsset(materialIdx);
      if (!materialAsset)
        return;

      E3DCOLOR color = panel.getColor(pid);
      if (materialAsset->props.paramExists("script"))
      {
        const char *matScript = materialAsset->props.getStr("script");

        char colorString[128];
        sprintf(colorString, "%hhu,%hhu,%hhu,%hhu", color.r, color.g, color.b, color.a);

        CfgReader c;
        c.getdiv_text(String(128, "[q]\r\n%s\r\n", matScript), "q");

        bool colorWritten = false;

        String newMatScript;
        for (int i = 0; i < c.div[c.curdiv].var.size(); i++)
        {
          CfgVar &var = c.div[c.curdiv].var[i];

          char *currentVal = var.val;
          if (strcmp(var.id, "color_mul_add") == 0)
          {
            currentVal = colorString;
            colorWritten = true;
          }
          newMatScript += String(128, "%s=%s%s", var.id, currentVal, i == c.div[c.curdiv].var.size() - 1 ? "" : "\r\n");
        }

        if (!colorWritten)
        {
          newMatScript += String(128, "\r\ncolor_mul_add=%s", colorString);
        }

        materialAsset->props.setStr("script", newMatScript.c_str());
      }
      else
      {
        materialAsset->props.setStr("script",
          String(128, "color_mul_add=%hhu,%hhu,%hhu,%hhu\r\n", color.r, color.g, color.b, color.a).c_str());
      }

      // udpate splines that use this material
      const int splineCount = ((HmapLandObjectEditor *)getObjEditor())->splinesCount();
      for (int iSpline = 0; iSpline < splineCount; iSpline++)
      {
        SplineObject *spline = ((HmapLandObjectEditor *)getObjEditor())->getSpline(iSpline);
        if (!spline)
          continue;

        if (spline->isUsingMaterial(materialAsset->getName()))
        {
          for (int iPoint = 0; iPoint < spline->points.size(); iPoint++)
          {
            spline->points[iPoint]->resetSplineClass();
            spline->markAssetChanged(iPoint);
          }
        }
      }
    }
  }

  if (pid == PID_MODIFTYPE_S || pid == PID_MODIFTYPE_P)
  {
    for (int i = 0; i < objects.size(); ++i)
    {
      SplineObject *o = RTTI_cast<SplineObject>(objects[i]);
      if (!o)
        continue;

      int modif = panel.getInt(pid);
      if (pid == PID_MODIFTYPE_P && modif == 1)
        modif = MODIF_HMAP;

      if (modif == o->getEffModifType())
        continue;

      if (!o->poly)
      {
        if (props.modifType == MODIF_SPLINE || modif == MODIF_SPLINE)
        {
          o->splineChanged = true;
          for (int i = 0; i < o->points.size(); i++)
            o->points[i]->markChanged();
        }
        o->props.modifType = modif;
      }
      else
        o->props.poly.hmapAlign = modif == MODIF_HMAP;

      o->reApplyModifiers(false, true);
      o->markModifChanged();
    }
  }
  else if (pid == PID_MAY_SELF_CROSS)
  {
    CHANGE_VAL_FUNC(bool, props.maySelfCross, getBool, pointChanged(-1))
    if (getObjEditor())
      getObjEd().updateCrossRoads(NULL);
    DAGORED2->repaint();
  }
  else if (pid == PID_POLY_OFFSET)
  {
    CHANGE_VAL_FUNC(Point2, props.poly.objOffs, getPoint2, regenerateObjects())
    DAGORED2->repaint();
  }
  else if (pid == PID_POLY_ROTATE)
  {
    CHANGE_VAL_FUNC(real, props.poly.objRot, getFloat, regenerateObjects())
    DAGORED2->repaint();
  }
  else if (pid == PID_POLY_SMOOTH)
  {
    CHANGE_VAL_FUNC(bool, props.poly.smooth, getBool, invalidateSplineCurve())
    DAGORED2->repaint();
  }
  else if (pid == PID_POLY_CURV)
  {
    CHANGE_VAL_FUNC(real, props.poly.curvStrength, getFloat, invalidateSplineCurve())
    DAGORED2->repaint();
  }
  else if (pid == PID_POLY_MINSTEP)
  {
    CHANGE_VAL_FUNC(real, props.poly.minStep, getFloat, invalidateSplineCurve())
    DAGORED2->repaint();
  }
  else if (pid == PID_POLY_MAXSTEP)
  {
    CHANGE_VAL_FUNC(real, props.poly.maxStep, getFloat, invalidateSplineCurve())
    DAGORED2->repaint();
  }
  else if (pid == PID_ALT_GEOM)
  {
    CHANGE_VAL(bool, polyGeom.altGeom, getBool)
    panel.setEnabledById(PID_BBOX_ALIGN_STEP, polyGeom.altGeom);
  }
  else if (pid == PID_BBOX_ALIGN_STEP)
  {
    CHANGE_VAL(float, polyGeom.bboxAlignStep, getFloat)
  }
  else if (pid == PID_EXPORTABLE)
  {
    CHANGE_VAL(bool, props.exportable, getBool)
  }
  else if (pid == PID_USE_FOR_NAVMESH)
  {
    CHANGE_VAL(bool, props.useForNavMesh, getBool)
  }
  else if (pid == PID_NAVMESH_STRIPE_WIDTH)
  {
    CHANGE_VAL(float, props.navMeshStripeWidth, getFloat)
  }
  else if (pid == PID_CORNER_TYPE)
  {
    CHANGE_VAL_FUNC(int, props.cornerType, getInt, invalidateSplineCurve())
  }
  else if (pid == PID_MODIF_WIDTH)
  {
    CHANGE_VAL_FUNC(real, props.modifParams.width, getFloat, markModifChangedWhenUsed())
    DAGORED2->repaint();
  }
  else if (pid == PID_MODIF_SMOOTH)
  {
    for (int i = 0; i < objects.size(); ++i)
    {
      SplineObject *o = RTTI_cast<SplineObject>(objects[i]);
      if (!o)
        continue;

      o->props.modifParams.smooth = panel.getFloat(pid);
      if (o->props.modifParams.smooth < 0)
        o->props.modifParams.smooth = 0;

      o->markModifChangedWhenUsed();
    }
    DAGORED2->repaint();
  }
  else if (pid == PID_MODIF_ADDITIVE)
    CHANGE_VAL_FUNC(bool, props.modifParams.additive, getBool, markModifChangedWhenUsed())
  else if (pid == PID_MODIF_CT_OFFSET)
  {
    CHANGE_VAL_FUNC(real, props.modifParams.offset[0], getFloat, markModifChangedWhenUsed())
    DAGORED2->repaint();
  }
  else if (pid == PID_MODIF_SD_OFFSET)
  {
    CHANGE_VAL_FUNC(real, props.modifParams.offset[1], getFloat, markModifChangedWhenUsed())
    DAGORED2->repaint();
  }
  else if (pid == PID_MODIF_OFFSET_POW)
  {
    for (int i = 0; i < objects.size(); ++i)
    {
      SplineObject *o = RTTI_cast<SplineObject>(objects[i]);
      if (!o)
        continue;

      o->props.modifParams.offsetPow = panel.getFloat(pid);
      if (o->props.modifParams.offsetPow < 0)
        o->props.modifParams.offsetPow = 0;
      o->markModifChangedWhenUsed();
    }
    DAGORED2->repaint();
  }
  else if (pid == PID_OPACGEN_BASE)
    opac_gen_base = panel.getFloat(pid);
  else if (pid == PID_OPACGEN_VAR)
    opac_gen_var = panel.getFloat(pid);
#undef CHANGE_VAL
}

void SplineObject::changeAsset(const char *asset_name, bool _undo)
{
  if (_undo)
    getObjEditor()->getUndoSystem()->put(new UndoChangeAsset(this, -1));
  props.blkGenName = asset_name ? asset_name : "";

  if (!poly)
    points[0]->resetSplineClass();
  else
    del_it(landClass);

  markAssetChanged(0);
  on_object_entity_name_changed(*this);
}

void SplineObject::changeAsset(ObjectEditor &object_editor, dag::ConstSpan<RenderableEditableObject *> objects,
  const char *initially_selected_asset_name, bool is_poly)
{
  const char *asset_name = DAEDITOR3.selectAssetX(initially_selected_asset_name, is_poly ? "Select landclass" : "Select splineclass",
    is_poly ? "land" : "spline");

  object_editor.getUndoSystem()->begin();

  for (int i = 0; i < objects.size(); i++)
  {
    SplineObject *o = RTTI_cast<SplineObject>(objects[i]);
    if (!o)
      continue;
    o->changeAsset(asset_name, true);
  }

  object_editor.getUndoSystem()->accept("Change entity");

  PropPanel::ContainerPropertyControl *pw = static_cast<HmapLandObjectEditor &>(object_editor).getObjectPropertiesPanel();
  if (pw)
    pw->setText(PID_GENBLKNAME, asset_name);

  IEditorCoreEngine::get()->invalidateViewportCache();
}

void SplineObject::onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  if (!points.size())
    return;

  if (pid == PID_GENBLKNAME)
  {
    changeAsset(*objEditor, objects, props.blkGenName, poly);
  }
  else if (pid == PID_GENBLKNAME2)
  {
    const char *asset_name = DAEDITOR3.selectAssetX(points[0]->getProps().blkGenName, "Select border splineclass", "spline");

    for (int i = 0; i < objects.size(); i++)
    {
      SplineObject *o = RTTI_cast<SplineObject>(objects[i]);
      if (!o)
        continue;
      o->points[0]->setBlkGenName(asset_name);
      o->points[0]->setEffectiveAsset(asset_name, true, 0);
    }

    panel.setText(pid, asset_name);
    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  else if (pid == PID_MONOTONOUS_Y_UP || pid == PID_MONOTONOUS_Y_DOWN)
  {
    getObjEditor()->getUndoSystem()->begin();
    for (int si = 0; si < objects.size(); si++)
    {
      SplineObject *o = RTTI_cast<SplineObject>(objects[si]);
      if (!o)
        continue;

      o->putObjTransformUndo();
      for (int i = 0; i < o->points.size(); i++)
        o->points[i]->putMoveUndo();

      if (pid == PID_MONOTONOUS_Y_UP)
        o->makeMonoUp();
      else
        o->makeMonoDown();
      if (o->props.cornerType > 0)
        o->applyCatmull(false, true);
    }
    getObjEditor()->getUndoSystem()->accept("Monotonous spline(s) height");
  }
  else if (pid == PID_LINEARIZE_Y)
  {
    getObjEditor()->getUndoSystem()->begin();
    for (int si = 0; si < objects.size(); si++)
    {
      SplineObject *o = RTTI_cast<SplineObject>(objects[si]);
      if (!o || o->isPoly())
        continue;

      o->putObjTransformUndo();
      for (int i = 0; i < o->points.size(); i++)
        o->points[i]->putMoveUndo();

      o->makeLinearHt();
      if (o->props.cornerType >= 0)
        o->applyCatmull(false, true);
    }
    getObjEditor()->getUndoSystem()->accept("Linearize spline(s) height");
  }
  else if (pid == PID_CATMUL_XYZ || pid == PID_CATMUL_XZ || pid == PID_CATMUL_Y)
  {
    getObjEditor()->getUndoSystem()->begin();
    for (int si = 0; si < objects.size(); si++)
    {
      SplineObject *o = RTTI_cast<SplineObject>(objects[si]);
      if (!o)
        continue;

      o->putObjTransformUndo();
      for (int i = 0; i < o->points.size(); i++)
        o->points[i]->putMoveUndo();

      o->applyCatmull(pid == PID_CATMUL_XYZ || pid == PID_CATMUL_XZ, pid == PID_CATMUL_XYZ || pid == PID_CATMUL_Y);
      o->getSpline();
    }
    getObjEditor()->getUndoSystem()->accept("Smooth Catmull-Rom");
  }
  else if (pid == PID_FLATTEN_Y)
  {
    getObjEditor()->getUndoSystem()->begin();
    putObjTransformUndo();
    for (int i = 0; i < points.size(); i++)
      points[i]->putMoveUndo();

    flattenBySpline = NULL;

    real midY = 0;
    for (int i = 0; i < points.size(); i++)
      midY += points[i]->getPt().y;

    midY /= points.size();

    for (int i = 0; i < points.size(); i++)
    {
      Point3 npos = points[i]->getPt();
      npos.y = midY;
      points[i]->setPos(npos);
      points[i]->FlattenY();
    }

    getObjEditor()->getUndoSystem()->accept("Flatten spline(s)");

    markModifChangedWhenUsed();
  }
  else if (pid == PID_FLATTEN_AVERAGE)
  {
    getObjEditor()->getUndoSystem()->begin();
    putObjTransformUndo();
    for (int i = 0; i < points.size(); i++)
      points[i]->putMoveUndo();

    flattenBySpline = NULL;

    Point3 cpos = Point3(0, 0, 0);
    for (int i = 0; i < points.size(); i++)
      cpos += points[i]->getPt();

    cpos /= points.size();

    Point3 cnorm = Point3(0, 0, 0);

    for (int i = 0; i < points.size(); i++)
    {
      Point3 p1 = points[i]->getPt();
      Point3 p2 = (i == points.size() - 1 ? points[0]->getPt() : points[i + 1]->getPt());

      Point3 v1 = p1 - cpos;
      Point3 v2 = p2 - cpos;

      Point3 tnorm = v1 % v2;
      tnorm.normalize();

      cnorm += tnorm;
    }

    cnorm.normalize();

    real tD = -cnorm.x * cpos.x - cnorm.y * cpos.y - cnorm.z * cpos.z;

    BBox3 oldBox, newBox;
    getWorldBox(oldBox);
    for (int i = 0; i < points.size(); i++)
    {
      real NY = -tD - cnorm.x * points[i]->getPt().x - cnorm.z * points[i]->getPt().z;
      NY /= cnorm.y;

      Point3 npos = points[i]->getPt();
      npos.y = NY;
      points[i]->setPos(npos);
    }
    getWorldBox(newBox);
    float mul = newBox.width().y / oldBox.width().y;
    for (int i = 0; i < points.size(); i++)
      points[i]->FlattenAverage(mul);

    getObjEditor()->getUndoSystem()->accept("Flatten spline(s)");

    markModifChangedWhenUsed();
  }
  else if (pid == PID_FLATTEN_SPLINE_APPLY)
  {
    getObjEditor()->getUndoSystem()->begin();
    putObjTransformUndo();
    for (int i = 0; i < points.size(); i++)
      points[i]->putMoveUndo();

    flattenBySpline = NULL;

    RenderableEditableObject *o = getObjEditor()->getObjectByName(panel.getText(PID_FLATTEN_SPLINE).str());
    if (!o)
    {
      destroy_it(csgGen);
      polyGeom.clear();
      return;
    }

    SplineObject *s = RTTI_cast<SplineObject>(o);
    if (!s || !s->points.size())
    {
      destroy_it(csgGen);
      polyGeom.clear();
      return;
    }

    flattenBySpline = s;

    getObjEditor()->getUndoSystem()->accept("Flatten spline(s)");

    markModifChangedWhenUsed();

    triangulatePoly();
  }
  else if (pid == PID_OPACGEN_APPLY)
  {
    getObjEditor()->getUndoSystem()->begin();

    for (int si = 0; si < objects.size(); si++)
      if (SplineObject *o = RTTI_cast<SplineObject>(objects[si]))
      {
        for (int i = 0; i < o->points.size(); i++)
        {
          float new_opac = clamp(opac_gen_base + gsrnd() * opac_gen_var, 0.0f, 1.0f);
          SplinePointObject::Props p = o->points[i]->getProps();
          if (new_opac != p.attr.opacity)
          {
            getObjEditor()->getUndoSystem()->put(o->points[i]->makePropsUndoObj());

            p.attr.opacity = opac_gen_base + gsrnd() * opac_gen_var;
            o->points[i]->setProps(p);
            o->points[i]->markChanged();
          }
        }
        o->getSpline();
      }
    getObjEditor()->getUndoSystem()->accept("Generate opacity in points");
  }
  else if (pid > PID_PER_MATERIAL_CONTROLS_BEGIN)
  {
    int materialPid = (pid - PID_PER_MATERIAL_CONTROLS_BEGIN) % MATERIAL_PID_COUNT;
    int materialIdx = (pid - PID_PER_MATERIAL_CONTROLS_BEGIN) / MATERIAL_PID_COUNT;

    if (materialPid == MATERIAL_PID_SAVEMAT_BTN)
    {
      DagorAsset *materialAsset = getMaterialAsset(materialIdx);
      if (!materialAsset)
        return;
      materialAsset->props.saveToTextFile(materialAsset->getTargetFilePath());
    }
  }
}


//==================================================================================================
bool SplineObject::getWorldBox(BBox3 &box) const
{
  if (points.size())
  {
    for (int i = 0; i < points.size(); ++i)
      box += points[i]->getPt();

    return true;
  }

  return false;
}

void SplineObject::onPointRemove(int id)
{
  if (!points.size())
    return;

  G_ASSERT(id >= 0 && id < points.size());

  bool closed = isClosed();
  erase_items(points, id, 1);

  if (!points.size())
    return;

  if (closed && id == 0)
    points.resize(points.size() - 1);
  else if (closed && id == points.size())
    erase_items(points, 0, 1);

  for (int i = points.size() - 1; i >= 0; i--)
    points[i]->arrId = i;

  if (points.size() > 1)
    getSpline();

  if (id > 0 && getObjEditor())
    getObjEd().updateCrossRoads(points[id - 1]);
  if (id < points.size() && getObjEditor())
    getObjEd().updateCrossRoads(points[id]);

  prepareSplineClassInPoints();
  pointChanged(id);
  if (id > 0)
    pointChanged(id - 1);
}

void SplineObject::addPoint(SplinePointObject *pt)
{
  G_ASSERTF(pt->arrId >= 0 && pt->arrId <= points.size(), "pt->arrId=%d points.size()=%d", pt->arrId, points.size());

  insert_items(points, pt->arrId, 1, &pt);

  for (int i = points.size() - 1; i >= 0; i--)
    points[i]->arrId = i;

  if (points.size() > 1)
    getSpline();

  if (getObjEditor())
  {
    if (pt->arrId > 0)
      getObjEd().updateCrossRoads(points[pt->arrId - 1]);
    getObjEd().updateCrossRoads(points[pt->arrId]);
  }

  prepareSplineClassInPoints();
  pointChanged(pt->arrId);
  if (pt->arrId > 0)
    pointChanged(pt->arrId - 1);
}

void SplineObject::refine(int seg_id, real loc_t, Point3 &p_pos)
{
  getObjEditor()->getUndoSystem()->begin();

  SplinePointObject *pt = new SplinePointObject;

  SplinePointObject *p1 = points[seg_id];
  SplinePointObject *p2 = points[(seg_id + 1) % points.size()];

  SplinePointObject::Props startProp1 = p1->getProps();
  SplinePointObject::Props startProp2 = p2->getProps();

  Point3 targetPos;

  if (poly)
    targetPos = p_pos;
  else
  {
    Point3 sp[4], cp[2];

    sp[0] = p1->getPt();
    sp[1] = p1->getBezierOut();
    sp[2] = p2->getBezierIn();
    sp[3] = p2->getPt();
    sp[0] = sp[0] * (1 - loc_t) + sp[1] * loc_t;
    sp[1] = sp[1] * (1 - loc_t) + sp[2] * loc_t;
    sp[2] = sp[2] * (1 - loc_t) + sp[3] * loc_t;
    sp[0] = sp[0] * (1 - loc_t) + sp[1] * loc_t;
    sp[1] = sp[1] * (1 - loc_t) + sp[2] * loc_t;
    targetPos = sp[0] * (1 - loc_t) + sp[1] * loc_t;

    p1->setRelBezierOut((p1->getBezierOut() - p1->getPt()) * loc_t);
    p2->setRelBezierIn((p2->getBezierIn() - p2->getPt()) * (1 - loc_t));

    pt->setRelBezierIn(sp[0] - targetPos);
    pt->setRelBezierOut(sp[1] - targetPos);
  }

  SplinePointObject::Props endProp1 = p1->getProps();
  SplinePointObject::Props endProp2 = p2->getProps();

  pt->setPos(targetPos);
  pt->spline = this;
  pt->arrId = seg_id + 1;
  getObjEditor()->addObject(pt);

  UndoRefineSpline *undoRefine = new UndoRefineSpline(this, pt, seg_id, startProp1, startProp2, endProp1, endProp2);

  pointChanged(-1);
  prepareSplineClassInPoints();
  getSpline();

  getObjEditor()->getUndoSystem()->put(undoRefine);
  putObjTransformUndo();
  getObjEditor()->getUndoSystem()->accept("Refine spline");
}


void SplineObject::split(int pt_id)
{
  G_ASSERT(pt_id >= 0 && pt_id < points.size());
  G_ASSERT(!isClosed());

  if (pt_id == 0)
  {
    DAEDITOR3.conWarning("cannot split spline at start point");
    return;
  }
  else if (pt_id == points.size() - 1)
  {
    DAEDITOR3.conWarning("cannot split spline at end point");
    return;
  }

  SplineObject *newSpline = new SplineObject(false);
  newSpline->setEditLayerIdx(EditLayerProps::activeLayerIdx[newSpline->lpIndex()]);
  newSpline->setCornerType(props.cornerType);

  getObjEditor()->getUndoSystem()->begin();
  UndoSplitSpline *undoSplit = new UndoSplitSpline(this, newSpline, pt_id, (HmapLandObjectEditor *)getObjEditor());

  newSpline->onCreated();

  getObjEditor()->getUndoSystem()->put(undoSplit);
  putObjTransformUndo();
  getObjEditor()->getUndoSystem()->accept("Split spline");
}


void SplineObject::splitOnTwoPolys(int pt1, int pt2)
{
  if (pt1 < 0 || pt1 + 1 > points.size() || pt2 < 0 || pt2 + 1 > points.size() || pt1 == pt2)
    return;

  int segid1, segid2;
  if (pt1 < pt2)
  {
    segid1 = pt1;
    segid2 = pt2;
  }
  else
  {
    segid1 = pt2;
    segid2 = pt1;
  }

  SplineObject *newPoly = new SplineObject(true);
  newPoly->setEditLayerIdx(EditLayerProps::activeLayerIdx[newPoly->lpIndex()]);
  newPoly->setBlkGenName(props.blkGenName);

  getObjEditor()->getUndoSystem()->begin();

  UndoSplitPoly *undoSplit = new UndoSplitPoly(this, newPoly, segid1, segid2, (HmapLandObjectEditor *)getObjEditor());

  newPoly->onCreated();

  getObjEditor()->getUndoSystem()->put(undoSplit);
  getObjEditor()->getUndoSystem()->accept("Split polygons");
}


void SplineObject::save(DataBlock &blk)
{
  if (!points.size())
    return;

  DataBlock *sblk = blk.addNewBlock(poly ? "polygon" : "spline");
  sblk->setStr("name", name);
  sblk->setStr("notes", props.notes);
  sblk->setInt("layerOrder", props.layerOrder);
  if (props.scaleTcAlong != 1.0f)
    sblk->setReal("scaleTcAlong", props.scaleTcAlong);

  sblk->setStr("blkGenName", props.blkGenName);
  sblk->setInt("modifType", props.modifType);

  if (poly)
  {
    sblk->setBool("polyHmapAlign", props.poly.hmapAlign);
    sblk->setPoint2("polyObjOffs", props.poly.objOffs);
    sblk->setReal("polyObjRot", props.poly.objRot);
    sblk->setBool("polySmooth", props.poly.smooth);
    sblk->setReal("polyCurv", props.poly.curvStrength);
    sblk->setReal("polyMinStep", props.poly.minStep);
    sblk->setReal("polyMaxStep", props.poly.maxStep);
  }
  else
    sblk->setBool("maySelfCross", props.maySelfCross);

  sblk->setBool("exportable", props.exportable);
  if (props.useForNavMesh != DEF_USE_FOR_NAVMESH)
    sblk->setBool("useForNavMesh", props.useForNavMesh);
  if (props.navMeshStripeWidth != DEF_NAVMESH_STRIPE_WIDTH)
    sblk->setReal("navMeshStripeWidth", props.navMeshStripeWidth);
  if (props.navmeshIdx >= 0)
    sblk->setInt("navmeshIdx", props.navmeshIdx);
  if (polyGeom.altGeom)
  {
    sblk->setBool("altGeom", polyGeom.altGeom);
    if (polyGeom.bboxAlignStep != 1.0f)
      sblk->setReal("bboxAlignStep", polyGeom.bboxAlignStep);
  }

  if (props.cornerType != 0)
    sblk->setInt("cornerType", props.cornerType);

  sblk->setInt("rseed", props.rndSeed);
  if (props.perInstSeed)
    sblk->setInt("perInstSeed", props.perInstSeed);

  DataBlock *mblk = sblk->addNewBlock("modifParams");
  mblk->setReal("width", props.modifParams.width);
  mblk->setReal("smooth", props.modifParams.smooth);
  mblk->setReal("offsetPow", props.modifParams.offsetPow);
  mblk->setPoint2("offset", props.modifParams.offset);
  mblk->setBool("additive", props.modifParams.additive);

  int pt_num = points.size();
  if (isClosed())
  {
    sblk->setBool("closed", true);
    pt_num--;
  }

  for (int i = 0; i < pt_num; i++)
  {
    DataBlock *cb = sblk->addNewBlock("point");
    points[i]->save(*cb);
  }
}


void SplineObject::load(const DataBlock &blk, bool use_undo)
{
  const char *n = blk.getStr("name", NULL);
  if (n)
    getObjEditor()->setUniqName(this, n);

  props.notes = blk.getStr("notes", "");
  props.layerOrder = blk.getInt("layerOrder", 0);
  props.blkGenName = blk.getStr("blkGenName", "");
  props.scaleTcAlong = blk.getReal("scaleTcAlong", 1.0f);

  const DataBlock *mblk = blk.getBlockByName("modifParams");
  if (mblk)
    loadModifParams(*mblk);

  props.maySelfCross = blk.getBool("maySelfCross", false);

  props.modifType = blk.getInt("modifType", MODIF_NONE);
  props.poly.hmapAlign = blk.getBool("polyHmapAlign", false);

  props.poly.objOffs = blk.getPoint2("polyObjOffs", Point2(0, 0));
  props.poly.objRot = blk.getReal("polyObjRot", 0);
  props.poly.smooth = blk.getBool("polySmooth", false);
  props.poly.curvStrength = blk.getReal("polyCurv", 1);
  props.poly.minStep = blk.getReal("polyMinStep", 30);
  props.poly.maxStep = blk.getReal("polyMaxStep", 1000);

  props.exportable = blk.getBool("exportable", false);
  props.cornerType = blk.getInt("cornerType", 0);
  props.useForNavMesh = blk.getBool("useForNavMesh", DEF_USE_FOR_NAVMESH);
  props.navMeshStripeWidth = blk.getReal("navMeshStripeWidth", DEF_NAVMESH_STRIPE_WIDTH);
  props.navmeshIdx = blk.getInt("navmeshIdx", -1);

  polyGeom.altGeom = blk.getBool("altGeom", false);
  polyGeom.bboxAlignStep = blk.getReal("bboxAlignStep", blk.getReal("minGridStep", 1.0f));

  int nid = blk.getNameId("point");

  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == nid)
    {
      const DataBlock &cb = *blk.getBlock(i);

      SplinePointObject *s = new SplinePointObject;
      s->load(cb);

      s->arrId = points.size();
      s->spline = this;

      getObjEditor()->addObject(s, use_undo);
    }
  if (blk.getBool("closed", false))
  {
    points.push_back(points[0]);
    getSpline();
  }

  props.rndSeed = blk.getInt("rseed", points[1]->getPt().lengthSq() * 1000);
  props.perInstSeed = blk.getInt("perInstSeed", 0);

  prepareSplineClassInPoints();
  markModifChangedWhenUsed();
}

void SplineObject::loadModifParams(const DataBlock &blk)
{
  props.modifParams.width = blk.getReal("width", 60);
  props.modifParams.smooth = blk.getReal("smooth", 10);
  props.modifParams.offsetPow = blk.getReal("offsetPow", 2);
  props.modifParams.offset = blk.getPoint2("offset", Point2(-0.05, 0));
  props.modifParams.additive = blk.getBool("additive", false);
}

bool SplineObject::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
{
  if (EditLayerProps::layerProps[getEditLayerIdx()].hide)
    return false;
  if (points.size() < 2)
    return false;

  Matrix44 gtm;
  d3d::getglobtm(gtm);
  Frustum f(gtm);
  if (!f.testSphereB(splSph.c, splSph.r))
    return false;

  Point2 last, cur;
  BBox2 box(Point2(rect.l, rect.t), Point2(rect.r, rect.b));

  if (poly && !props.poly.smooth)
  {
    if (vp->worldToClient(points[0]->getPt(), last) && (box & last))
      return true;

    for (int i = 1; i < points.size(); i++)
    {
      if (vp->worldToClient(points[i]->getPt(), cur) && (box & cur))
        return true;

      if (::isect_line_segment_box(last, cur, box))
        return true;

      last = cur;
    }

    if (vp->worldToClient(points[0]->getPt(), cur) && (box & cur))
      return true;
    if (::isect_line_segment_box(last, cur, box))
      return true;
  }
  else
  {
    for (int i = 0; i < segSph.size(); i++)
      if (::is_sphere_hit_by_rect(segSph[i].c, segSph[i].r, vp, rect))
      {
        if (vp->worldToClient(bezierSpline.segs[i].point(0.0), last) && (box & last))
          return true;

        for (float t = splStep; t < 1.0; t += splStep)
        {
          if (vp->worldToClient(bezierSpline.segs[i].point(t), cur) && (box & cur))
            return true;
          if (::isect_line_segment_box(last, cur, box))
            return true;
          last = cur;
        }
        if (vp->worldToClient(bezierSpline.segs[i].point(1.0), cur) && (box & cur))
          return true;
        if (::isect_line_segment_box(last, cur, box))
          return true;
      }
  }

  return false;
}

bool SplineObject::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
{
  if (EditLayerProps::layerProps[getEditLayerIdx()].hide)
    return false;
  for (int i = 0; i < points.size(); i++)
    if (points[i]->isSelectedByPointClick(vp, x, y))
      return ((HmapLandObjectEditor *)objEditor)->getSelectMode() == CM_SELECT_SPLINES;
  return getPosOnSpline(vp, x, y, 4);
}

bool SplineObject::getPosOnSpline(IGenViewportWnd *vp, int x, int y, float max_dist, int *out_segid, float *out_local_t,
  Point3 *out_p) const
{
  if (out_segid)
    *out_segid = -1;

  if (points.size() < 2)
    return false;

  Matrix44 gtm;
  d3d::getglobtm(gtm);
  Frustum f(gtm);
  if (!f.testSphereB(splSph.c, splSph.r))
    return false;

  Point3 last_p3, cur_p3;
  Point2 last_p2, cur_p2;
  float last_t, cur_t;
  Point2 p(x, y);
  int segId = -1;

  for (int i = 0; i < segSph.size(); i++)
    if (::is_sphere_hit_by_point(segSph[i].c, segSph[i].r, vp, x, y))
    {
      segId = i;

      last_t = 0.0;
      last_p3 = bezierSpline.segs[i].point(last_t);
      vp->worldToClient(last_p3, last_p2);

      for (float t = splStep; t < 1.0; t += splStep)
      {
        cur_t = t;
        cur_p3 = bezierSpline.segs[i].point(t);
        vp->worldToClient(cur_p3, cur_p2);
        if (::distance_point_to_line_segment(p, last_p2, cur_p2) < max_dist)
          goto success;
        last_t = cur_t;
        last_p2 = cur_p2;
        last_p3 = cur_p3;
      }

      cur_t = 1.0;
      cur_p3 = bezierSpline.segs[i].point(cur_t);
      vp->worldToClient(cur_p3, cur_p2);
      if (::distance_point_to_line_segment(p, last_p2, cur_p2) < max_dist)
        goto success;
    }

  if (poly && !props.poly.smooth)
  {
    segId = points.size() - 1;
    last_p3 = points.back()->getPt();
    vp->worldToClient(last_p3, last_p2);

    for (float tt = splStep; tt < 1.0; tt += splStep)
    {
      cur_p3 = points.back()->getPt() * (1 - tt) + points[0]->getPt() * tt;

      vp->worldToClient(cur_p3, cur_p2);

      if (::distance_point_to_line_segment(p, last_p2, cur_p2) < max_dist)
        goto success;

      last_p2 = cur_p2;
      last_p3 = cur_p3;
    }
  }

  if (out_segid)
    *out_segid = -1;

  return false;

success:

  if (out_segid)
    *out_segid = segId;

  if (out_local_t || out_p)
  {
    Point2 dp = p - last_p2;
    Point2 dir = cur_p2 - last_p2;
    real len2 = lengthSq(dir);

    float t = (dp * dir) / len2;

    if (t <= 0)
    {
      if (out_local_t)
        *out_local_t = last_t;
      if (out_p)
        *out_p = last_p3;
    }
    else if (t >= 1)
    {
      if (out_local_t)
        *out_local_t = cur_t;
      if (out_p)
        *out_p = cur_p3;
    }
    else
    {
      if (out_local_t)
        *out_local_t = last_t * (1 - t) + cur_t * t;
      if (out_p)
        *out_p = last_p3 * (1 - t) + cur_p3 * t;
    }
  }
  return true;
}


static void build_corner_spline(BezierSplinePrec3d &loftSpl, BezierSplinePrec3d &baseSpl, dag::ConstSpan<splineclass::SegData> seg,
  int ss, int es)
{
  clear_and_shrink(loftSpl.segs);
  if (!seg.size())
  {
    loftSpl.calculateLength();
    return;
  }

  Tab<Point3> pts(tmpmem);
  pts.reserve(seg.size() * 3);

  // debug("loftSegs=%d (%d..%d)", seg.size(), ss, es);
  for (int i = 0; i < seg.size(); ++i)
  {
    if (seg[i].segN + ss < ss)
      continue;
    else if (seg[i].segN + ss > es)
      break;
    int base = append_items(pts, 3);
    pts[base + 0] = pts[base + 1] = pts[base + 2] = Point3::xVz(baseSpl.segs[seg[i].segN + ss].point(seg[i].offset), seg[i].y);
    // debug(" %d: seg=%d ofs=%.4f y=%.4f  %~p3", i, seg[i].segN+ss, seg[i].offset, seg[i].y, pts[base+0]);
  }
  loftSpl.calculate(pts.data(), pts.size(), false);
}
static void build_ground_spline(BezierSpline3d &gndSpl, const PtrTab<SplinePointObject> &points, bool poly, bool poly_smooth,
  bool is_closed)
{
  Tab<Point3> pt_gnd;
  pt_gnd.resize(points.size() * 3);

  // drop points to collision and recompute Y for helpers using CatmullRom
  {
    for (int pi = 0; pi < points.size(); pi++)
    {
      pt_gnd[pi * 3 + 0] = points[pi]->getPt();
      HmapLandPlugin::self->getHeightmapPointHt(pt_gnd[pi * 3 + 0], NULL);

      pt_gnd[pi * 3 + 1] = points[pi]->getPtEffRelBezierIn();
      pt_gnd[pi * 3 + 2] = points[pi]->getPtEffRelBezierOut();
    }

    Point3 *catmul[4];
    BezierSplineInt<Point3> sp;
    Point3 v[4];
    int pn = points.size();

    for (int i = -1; i < pn - 2; i++)
    {
      for (int j = 0; j < 4; j++)
        catmul[j] = pt_gnd.data() + ((poly || is_closed) ? (i + j + pn) % pn : clamp(i + j, 0, pn - 1)) * 3;

      for (int j = 0; j < 4; j++)
        v[j] = *catmul[j];
      sp.calculateCatmullRom(v);
      sp.calculateBack(v);

      float pi0_y = v[0].y - v[1].y, pi1_y = v[2].y - v[3].y;
      catmul[1][1].y = pi0_y;
      catmul[1][2].y = -pi0_y;
      catmul[2][1].y = pi1_y;
      catmul[2][2].y = -pi1_y;
    }
  }

  // build spline from ground points
  SmallTab<Point3, TmpmemAlloc> pts;
  int pts_num = points.size() + (poly ? 1 : 0);
  clear_and_resize(pts, pts_num * 3);

  for (int pi = 0; pi < pts_num; ++pi)
  {
    int wpi = pi % points.size();
    pts[pi * 3 + 1] = pt_gnd[wpi * 3 + 0];
    if ((poly && !poly_smooth) || (points[wpi]->isCross && points[wpi]->isRealCross))
      pts[pi * 3 + 0] = pts[pi * 3 + 2] = pts[pi * 3 + 1];
    else
    {
      pts[pi * 3 + 0] = pt_gnd[wpi * 3 + 0] + pt_gnd[wpi * 3 + 1];
      pts[pi * 3 + 2] = pt_gnd[wpi * 3 + 0] + pt_gnd[wpi * 3 + 2];
    }
  }

  gndSpl.calculate(pts.data(), pts.size(), false);
}

void SplineObject::regenerateObjects()
{
  if (splineInactive)
    return;
  if (!points.size())
    return;

  if (poly)
    placeObjectsInsidePolygon();

  if (points.size() > 1)
  {
    BezierSpline3d onGndSpline;
    BezierSpline3d &effSpline = (props.modifType == MODIF_SPLINE) ? onGndSpline : bezierSpline;

    if (props.modifType == MODIF_SPLINE)
    {
      for (SplinePointObject *p : points)
        if (ISplineGenObj *gen = p->getSplineGen())
          for (auto &p : gen->entPools)
            p.resetUsedEntities();
      build_ground_spline(onGndSpline, points, poly, props.poly.smooth, isClosed());
    }

    int start_seg = 0;
    SplinePointObject *p0 = points[start_seg];

    for (int i = 0; i < cablesPool.size(); ++i)
      HmapLandPlugin::cableService->removeCable(cablesPool[i]);
    cablesPool.clear();

    for (int i = 1; i < (int)points.size() - 1; i++)
    {
      if (!points[i]->getSplineGen())
        continue;

      if (ISplineGenObj *gen = p0->getSplineGen())
        gen->generateObjects(effSpline, start_seg, i, splineSubTypeId, editLayerIdx, props.rndSeed, props.perInstSeed, &cablesPool);

      start_seg = i;
      p0 = points[start_seg];
    }

    if (ISplineGenObj *gen = p0->getSplineGen())
      gen->generateObjects(effSpline, start_seg, effSpline.segs.size(), splineSubTypeId, editLayerIdx, props.rndSeed,
        props.perInstSeed, &cablesPool);
  }
  DAGORED2->restoreEditorColliders();
}


bool SplineObject::onAssetChanged(landclass::AssetData *data)
{
  if (landClass && landClass->data == data)
  {
    pointChanged(-1);
    return !splineInactive;
  }
  return false;
}

bool SplineObject::onAssetChanged(splineclass::AssetData *data)
{
  if (poly)
    return false;

  bool ret = false;
  for (int i = 0; i < points.size(); i++)
    if (points[i]->getSplineClass() == data)
    {
      points[i]->resetSplineClass();
      markAssetChanged(i);
      ret = true;
    }
  return !splineInactive;
}

void SplineObject::putObjTransformUndo()
{
  class ObjTransformUndo : public UndoRedoObject
  {
    Ptr<SplineObject> obj;

  public:
    ObjTransformUndo(SplineObject *o) : obj(o) {}

    virtual void restore(bool save_redo)
    {
      obj->pointChanged(-1);
      if (obj->isAffectingHmap())
        obj->markModifChanged();
    }
    virtual void redo()
    {
      obj->pointChanged(-1);
      if (obj->isAffectingHmap())
        obj->markModifChanged();
    }

    virtual size_t size() { return sizeof(*this); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "ObjTransformUndo"; }
  };

  getObjEditor()->getUndoSystem()->put(new ObjTransformUndo(this));
}

void SplineObject::createMaterialControls(PropPanel::ContainerPropertyControl &op)
{
  if (points.size() == 0)
    return;

  ISplineGenObj *gen = points[0]->getSplineGen();
  if (!gen || !gen->splineClass || !gen->splineClass->genGeom)
    return;

  int matGroupIdx = 0;
  for (int iLoft = 0; iLoft < gen->splineClass->genGeom->loft.size(); iLoft++)
  {
    auto &loft = gen->splineClass->genGeom->loft[iLoft];
    for (int iMat = 0; iMat < loft.matNames.size(); iMat++, matGroupIdx++)
    {
      const SimpleString &matName = loft.matNames[iMat];
      DagorAsset *materialAsset = DAEDITOR3.getAssetByName(matName.c_str(), DAEDITOR3.getAssetTypeId("mat"));
      if (!materialAsset)
        continue;

      int pidBase = PID_PER_MATERIAL_CONTROLS_BEGIN + matGroupIdx * MATERIAL_PID_COUNT;
      PropPanel::ContainerPropertyControl &materialGroup =
        *op.createGroup(pidBase + MATERIAL_PID_GROUP, String(128, "Loft #%d / Mat (%s)", iLoft, matName.c_str()));

      E3DCOLOR color(255, 255, 255, 0);
      if (materialAsset->props.paramExists("script"))
      {
        const char *matScript = materialAsset->props.getStr("script");

        CfgReader c;
        c.getdiv_text(String(128, "[q]\r\n%s\r\n", matScript), "q");
        color = c.gete3dcolor("color_mul_add", color);
      }

      materialGroup.createColorBox(pidBase + MATERIAL_PID_COLOR, "Material color mul:", color);
      materialGroup.createButton(pidBase + MATERIAL_PID_SAVEMAT_BTN, "Save mat.blk");
    }
  }
}

void SplineObject::placeObjectsInsidePolygon()
{
  // lines.clear();
  if (!landClass)
    return;

  Point3 hmofs = HmapLandPlugin::self->getHeightmapOffset();
  landClass->beginGenerate();
  if (landClass->data && landClass->data->tiled)
    objgenerator::generateTiledEntitiesInsidePoly(*landClass->data->tiled, tiledByPolygonSubTypeId, editLayerIdx, HmapLandPlugin::self,
      make_span(landClass->poolTiled), make_span_const((SplinePointObject **)points.data(), points.size()),
      DEG_TO_RAD * props.poly.objRot, props.poly.objOffs + Point2(hmofs.x, hmofs.z));
  if (landClass->data && landClass->data->planted)
  {
    typedef HierBitMap2d<ConstSizeBitMap2d<5>> HierBitmap32;
    HierBitmap32 bmp;
    float ofs_x, ofs_z, cell;

    cell = rasterize_poly(bmp, ofs_x, ofs_z, make_span_const((SplinePointObject **)points.data(), points.size()));
    float mid_y = 0;
    for (int i = 0; i < points.size(); i++)
      mid_y += points[i]->getPt().y / points.size();


    objgenerator::generatePlantedEntitiesInMaskedRect(*landClass->data->planted, tiledByPolygonSubTypeId, editLayerIdx, NULL,
      make_span(landClass->poolPlanted), bmp, 1.0 / cell, 0, 0, bmp.getW() * cell, bmp.getH() * cell, ofs_x, ofs_z, mid_y);
  }
  landClass->endGenerate();

  DAGORED2->restoreEditorColliders();
}

void SplineObject::gatherStaticGeometry(StaticGeometryContainer &cont, int flags, bool collision, int layer, int stage)
{
  if (splineInactive)
    return;
  if (!created)
    return;

  if (collision)
    flags |= StaticGeometryNode::FLG_COLLIDABLE;

  int st_mask =
    DAEDITOR3.getEntitySubTypeMask(collision ? IObjEntityFilter::STMASK_TYPE_COLLISION : IObjEntityFilter::STMASK_TYPE_EXPORT);

  const char *num_suffix = strrchr(name, '_');
  int id = num_suffix ? atoi(num_suffix + 1) : 0;

  if (st_mask & roadsSubtypeMask)
    for (int i = 0; i < (int)points.size() - 1; i++)
    {
      GeomObject *road = points[i]->getRoadGeom();
      if (!road)
        continue;

      const StaticGeometryContainer *geom = road->getGeometryContainer();
      if (geom)
        gatherStaticGeom(cont, *geom, flags, id, i, stage);
    }

  if (poly && (st_mask & polygonSubtypeMask) && !polyGeom.altGeom)
  {
    if (polyGeom.mainMesh)
      gatherStaticGeom(cont, *polyGeom.mainMesh->getGeometryContainer(), flags, id, 0, stage);
    if (polyGeom.borderMesh)
      gatherStaticGeom(cont, *polyGeom.borderMesh->getGeometryContainer(), flags, id, 1, stage);
  }
}

void SplineObject::gatherStaticGeom(StaticGeometryContainer &cont, const StaticGeometryContainer &geom, int flags, int id,
  int name_idx1, int stage)
{
  for (int ni = 0; ni < geom.nodes.size(); ++ni)
  {
    StaticGeometryNode *node = geom.nodes[ni];
    if (node && node->script.getInt("stage", 0) != stage)
      continue;

    if (node && (!flags || (node->flags & flags)))
    {
      StaticGeometryNode *n = dagGeom->newStaticGeometryNode(*node, tmpmem);
      n->name = (const char *)String(1024, "%d_%s#%d", id, (const char *)n->name, name_idx1);
      cont.addNode(n);
    }
  }
}

void SplineObject::gatherStaticGeomLayered(StaticGeometryContainer &cont, const StaticGeometryContainer &geom, int flags, int id,
  int name_idx1, int layer, int stage)
{
  for (int ni = 0; ni < geom.nodes.size(); ++ni)
  {
    StaticGeometryNode *node = geom.nodes[ni];
    if (node && node->script.getInt("stage", 0) != stage)
      continue;

    if (node && node->script.getInt("layer", 0) == layer && (!flags || (node->flags & flags)))
    {
      StaticGeometryNode *n = dagGeom->newStaticGeometryNode(*node, tmpmem);
      n->name = (const char *)String(1024, "%d_%s#%d", id, (const char *)n->name, name_idx1);
      cont.addNode(n);
    }
  }
}

bool SplineObject::setName(const char *nm)
{
  const bool result = RenderableEditableObject::setName(nm);

  if (getObjEditor())
    getObjEd(getObjEditor()).onRegisteredObjectNameChanged(*this);

  return result;
}

void SplineObject::moveObject(const Point3 &delta, IEditorCoreEngine::BasisType basis)
{
  if (!points.size())
    return;

  RenderableEditableObject::putMoveUndo();
  putObjTransformUndo();
  RenderableEditableObject::moveObject(delta, basis);

  int pnum = isClosed() ? points.size() - 1 : points.size();
  for (int i = 0; i < pnum; i++)
  {
    points[i]->putMoveUndo();
    points[i]->setPos(points[i]->getPos() + delta);
  }

  objectWasMoved = true;
}

void SplineObject::rotateObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis)
{
  if (!points.size())
    return;

  if (basis == IEditorCoreEngine::BASIS_Local)
    return;

  RenderableEditableObject::putRotateUndo();
  putObjTransformUndo();
  RenderableEditableObject::rotateObject(delta, origin, basis);


  Matrix3 dtm = rotxM3(delta.x) * rotyM3(delta.y) * rotzM3(delta.z);
  TMatrix tdtm = rotxTM(delta.x) * rotyTM(delta.y) * rotzTM(delta.z);

  int pnum = isClosed() ? points.size() - 1 : points.size();
  for (int i = 0; i < pnum; i++)
  {
    points[i]->putRotateUndo();

    Point3 pIn = points[i]->getProps().relIn, pOut = points[i]->getProps().relOut;

    pIn = tdtm * pIn;
    pOut = tdtm * pOut;

    float wtm_det = points[i]->getWtm().det();
    if (fabsf(wtm_det) < 1e-12)
      continue;
    Point3 lorg = inverse(points[i]->getWtm(), wtm_det) * origin;
    points[i]->setMatrix(dtm * points[i]->getMatrix());
    points[i]->setPos(points[i]->getPos() + origin - points[i]->getWtm() * lorg);

    points[i]->setRelBezierIn(pIn);
    points[i]->setRelBezierOut(pOut);
  }

  objectWasRotated = true;
}

void SplineObject::scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis)
{
  if (!points.size())
    return;

  RenderableEditableObject::putScaleUndo();
  putObjTransformUndo();
  RenderableEditableObject::scaleObject(delta, origin, basis);

  Matrix3 dtm;
  dtm.zero();
  dtm[0][0] = delta.x;
  dtm[1][1] = delta.y;
  dtm[2][2] = delta.z;

  TMatrix tdtm;
  tdtm.zero();
  tdtm[0][0] = delta.x;
  tdtm[1][1] = delta.y;
  tdtm[2][2] = delta.z;

  int pnum = isClosed() ? points.size() - 1 : points.size();
  for (int i = 0; i < pnum; i++)
  {
    points[i]->putScaleUndo();

    Point3 pIn = points[i]->getProps().relIn, pOut = points[i]->getProps().relOut;

    pIn = tdtm * pIn;
    pOut = tdtm * pOut;

    const Point3 pos = points[i]->getPos();
    const Point3 dir = pos - origin;
    points[i]->setPos(pos - dir + dtm * dir);

    points[i]->setRelBezierIn(pIn);
    points[i]->setRelBezierOut(pOut);
  }

  objectWasScaled = true;
}

struct TmpPointRec
{
  Ptr<SplinePointObject> p;
  String blk;
  int aIdx;
};
void SplineObject::reverse()
{
  G_ASSERT(points.size());

  Tab<TmpPointRec> old(tmpmem);
  bool closed = isClosed();

  old.resize(points.size());
  int a_idx = 0;
  for (int i = 0; i < points.size(); i++)
  {
    old[i].p = points[i];
    if (poly)
      old[i].blk = points[i]->getProps().blkGenName.str();
    else
      old[i].blk = i == 0 ? props.blkGenName.str() : points[i]->getProps().blkGenName.str();
    if (points[i]->hasSplineClass())
      a_idx = i;
    old[i].aIdx = a_idx;

    points[i]->resetSplineClass();
  }

  points[0]->setBlkGenName(NULL);
  for (int i = 0; i < (int)points.size() - 1; i++)
  {
    points[i] = old[points.size() - 1 - i].p;
    points[i]->arrId = i;
    points[i]->swapHelpers();
  }

  if (closed)
    points.back() = points[0];
  else
  {
    points.back() = old[0].p;
    points.back()->arrId = points.size() - 1;
    points.back()->swapHelpers();
    points.back()->setBlkGenName(NULL);
  }

  a_idx = old.back().aIdx;
  if (poly)
    points[0]->setBlkGenName(old[a_idx].blk);
  else
    props.blkGenName = old[a_idx].blk;

  for (int i = 1; i < (int)points.size() - 1; i++)
  {
    int inv_id = (points.size() - 1) - i - 1;

    if (a_idx == old[inv_id].aIdx)
      points[i]->setBlkGenName(NULL);
    else
    {
      a_idx = old[inv_id].aIdx;
      points[i]->setBlkGenName(old[a_idx].blk);
    }
  }

  for (int i = 0; i < points.size(); i++)
    markAssetChanged(i);
}

void SplineObject::onCreated(bool gen)
{
  created = true;
  pointChanged(-1);
}

bool SplineObject::isSelfCross()
{
  // TODO: self crossing
  return false;
}


void SplineObject::updateFullLoft() { generateLoftSegments(0, points.size() - (poly ? 0 : 1)); }


bool SplineObject::pointInsidePoly(const Point2 &p)
{
  if (!poly)
    return false;

  int pj, pk = 0;
  double wrkx, yu, yl;

  for (pj = 0; pj < points.size(); pj++)
  {
    Point3 ppj = points[pj]->getPt();
    Point3 ppj1 = points[(pj + 1) % points.size()]->getPt();

    yu = ppj.z > ppj1.z ? ppj.z : ppj1.z;
    yl = ppj.z < ppj1.z ? ppj.z : ppj1.z;

    if (ppj1.z - ppj.z)
      wrkx = ppj.x + (ppj1.x - ppj.x) * (p.y - ppj.z) / (ppj1.z - ppj.z);
    else
      wrkx = ppj.x;

    if (yu >= p.y)
      if (yl < p.y)
      {
        if (p.x > wrkx)
          pk++;
        if (fabs(p.x - wrkx) < 0.001)
          return true;
      }

    if ((fabs(p.y - yl) < 0.001) && (fabs(yu - yl) < 0.001) &&
        (fabs(fabs(wrkx - ppj.x) + fabs(wrkx - ppj1.x) - fabs(ppj.x - ppj1.x)) < 0.001))
      return true;
  }

  if (pk % 2)
    return true;
  else
    return false;

  return false;
}

void SplineObject::updateSpline(int stage)
{
  switch (stage)
  {
    case STAGE_START:
      getSpline();
      updateChangedSegmentsRoadGeom();
      return;

    case STAGE_GEN_POLY:
      if (poly)
        triangulatePoly();
      return;

    case STAGE_GEN_LOFT: updateChangedSegmentsLoftGeom(); return;

    case STAGE_FINISH:
      regenerateObjects();
      regenerateVBuf();
      for (int i = 0; i < points.size(); i++)
        points[i]->segChanged = false;
      splineChanged = false;
      return;

    default: DAG_FATAL("bad stage: %d");
  }
}

void SplineObject::onAdd(ObjectEditor *objEditor)
{
  if (name.empty())
  {
    String objname((poly ? "Polygon_001" : "Spline_001"));
    objEditor->setUniqName(this, objname);
  }

  prepareSplineClassInPoints();
  pointChanged(-1);
  markModifChangedWhenUsed();
}

void SplineObject::onRemove(ObjectEditor *objEditor)
{
  for (int i = 0; i < cablesPool.size(); ++i)
    HmapLandPlugin::cableService->removeCable(cablesPool[i]);

  for (int i = 0; i < points.size(); i++)
    points[i]->resetSplineClass();

  del_it(landClass);
  pointChanged(-1);
  markModifChangedWhenUsed();
  HmapLandPlugin::hmlService->invalidateClipmap(false);
}

Point3 SplineObject::splineCenter() const
{
  if (!points.size())
    return Point3(0, 0, 0);

  BBox3 box;
  for (int i = 0; i < points.size(); i++)
    box += points[i]->getPt();

  BSphere3 sph;
  sph = box;

  return sph.c;
}

Point3 SplineObject::getProjPoint(const Point3 &p, real *proj_t)
{
  real splineLen = bezierSpline.getLength();

  Tab<Point3> splinePoly(tmpmem);
  for (real t = 0.0; t < splineLen; t += splineLen / RENDER_SPLINE_POINTS)
    splinePoly.push_back(bezierSpline.get_pt(t));

  if (!splinePoly.size())
    return Point3(0, 0, 0);

  Point3 diff = splinePoly[0] - p;

  Point3 ddest = splinePoly[0];
  real ddist = diff.length();
  real dt = 0;

  for (int j = 1; j < splinePoly.size(); j++)
  {
    Point3 tdiff = splinePoly[j] - p;
    if (tdiff.length() < ddist)
    {
      ddist = tdiff.length();
      ddest = splinePoly[j];
      dt = splineLen / RENDER_SPLINE_POINTS * j;
    }
  }

  if (proj_t)
    *proj_t = dt;

  return ddest;
}

Point3 SplineObject::getPolyClosingProjPoint(Point3 &p, real *proj_t)
{
  Point3 p1 = points.back()->getPt();
  Point3 p2 = points[0]->getPt();

  Point3 dest;
  real out_t = 0, dist = MAX_REAL;

  real splineLen = bezierSpline.getLength();

  Point3 ptsDiff = p2 - p1;
  real ptsL = ptsDiff.length();

  for (real t = 0.0; t < ptsL; t += splineLen / RENDER_SPLINE_POINTS)
  {
    real dt = t / ptsL;
    Point3 cur = p1 * (1 - dt) + p2 * dt;

    Point3 pd = cur - p;
    if (pd.length() < dist)
    {
      dist = pd.length();
      dest = cur;
      out_t = splineLen + t;
    }
  }

  if (proj_t)
    *proj_t = out_t;

  return dest;
}

SplinePointObject *SplineObject::getNearestPoint(Point3 &p)
{
  if (!points.size())
    return NULL;

  Point3 diff = points[0]->getPt() - p;
  real dist = diff.length();
  int id = 0;

  for (int i = 1; i < points.size(); i++)
  {
    diff = points[i]->getPt() - p;
    real k = diff.length();
    if (k < dist)
    {
      dist = k;
      id = i;
    }
  }

  return points[id];
}

bool SplineObject::lineIntersIgnoreY(Point3 &p1, Point3 &p2)
{
  for (int i = 0; i < (int)points.size() - 1; i++)
  {
    Point3 pp1 = points[i]->getPt();
    Point3 pp2 = points[i + 1]->getPt();

    if (lines_inters_ignore_Y(p1, p2, pp1, pp2))
      return true;
  }

  return false;
}

real SplineObject::getTotalLength()
{
  real total = 0;

  if (poly)
  {
    for (int i = 0; i < points.size(); i++)
    {
      Point3 p1 = points[i]->getPt(), p2;
      if (i == points.size() - 1)
        p2 = points[0]->getPt();
      else
        p2 = points[i + 1]->getPt();

      Point3 diff = p2 - p1;
      total += diff.length();
    }
  }
  else
  {
    getSpline();
    total = bezierSpline.getLength();
  }

  return total;
}

real SplineObject::getSplineTAtPoint(int id)
{
  real tt = 0;
  for (int i = 0; i < id; i++)
    tt += bezierSpline.segs[i].len;

  return tt;
}

bool SplineObject::isDirReversed(Point3 &p1, Point3 &p2)
{
  real t1 = 0, t2 = 1;
  getProjPoint(p1, &t1);
  getProjPoint(p2, &t2);

  if (t1 > t2)
    return true;

  return false;
}

bool SplineObject::intersects(const BBox2 &r, bool mark_segments)
{
  if (!(BBox2(Point2(splBox[0].x, splBox[0].z), Point2(splBox[1].x, splBox[1].z)) & r))
    return false;
  if (poly)
    return true;


  if (mark_segments)
  {
    bool isect = false;
    for (int i = 0; i < segSph.size(); i++)
      if (segSph[i] & BBox3(Point3(r[0].x, segSph[i].c.y - segSph[i].r, r[0].y), Point3(r[1].x, segSph[i].c.y + segSph[i].r, r[1].y)))
      {
        isect = true;
        pointChanged(i);
      }
    return isect;
  }

  for (int i = 0; i < segSph.size(); i++)
    if (segSph[i] & BBox3(Point3(r[0].x, segSph[i].c.y - segSph[i].r, r[0].y), Point3(r[1].x, segSph[i].c.y + segSph[i].r, r[1].y)))
      return true;
  return false;
}

void SplineObject::reApplyModifiers(bool apply_now, bool force)
{
  if (splineInactive)
    return;
  if (!force && !isAffectingHmap())
  {
    modifChanged = false;
    return;
  }

  BBox3 b = splBox;
  float addr = props.modifParams.width + props.modifParams.smooth;
  Point3 add(addr, 0, addr);

  b[0] -= add;
  b[1] += add;
  splBoxPrev += b;

  HmapLandPlugin::self->invalidateFinalBox(splBoxPrev);

  splBoxPrev = splBox;
  splBoxPrev[0] -= add;
  splBoxPrev[1] += add;

  modifChanged = false;
  if (apply_now)
    HmapLandPlugin::self->applyHmModifiers();
}

void SplineObject::pointChanged(int pt_idx)
{
  if (pt_idx >= 0 && pt_idx + 1 <= points.size())
  {
    points[pt_idx]->segChanged = true;
    if (points[pt_idx]->isCross)
      ((HmapLandObjectEditor *)getObjEditor())->markCrossRoadChanged(points[pt_idx]);
  }
  else if (pt_idx == -1)
    for (int i = 0; i < points.size(); i++)
      points[i]->segChanged = true;
  splineChanged = true;
  if (getObjEditor())
    ((HmapLandObjectEditor *)getObjEditor())->splinesChanged = true;
}
void SplineObject::markModifChanged()
{
  modifChanged = true;
  if (getObjEditor())
    ((HmapLandObjectEditor *)getObjEditor())->splinesChanged = true;
  DAGORED2->invalidateViewportCache();
}
void SplineObject::markAssetChanged(int i)
{
  // debug("markAssetChanged: %d", i);
  prepareSplineClassInPoints();

  pointChanged(i);
  if (!points[i]->isCross && i > 0)
    pointChanged(i - 1);
  i++;

  for (; i < points.size(); i++)
  {
    pointChanged(i);

    if (points[i]->hasSplineClass() && points[i]->getSplineClass())
      break;
  }
}

void SplineObject::updateChangedSegmentsRoadGeom()
{
  splineclass::RoadData *asset_prev = NULL, *asset = NULL;
  if (isClosed())
    asset_prev = getRoad(points[points.size() - 2]);

  int start_seg = 0;
  asset = getRoad(points[0]);

  // update all segments
  for (int i = 1; i < (int)points.size() - 1; i++)
  {
    splineclass::RoadData *asset_next = getRoad(points[i]);
    if (asset_next != asset)
    {
      generateRoadSegments(start_seg, i, asset_prev, asset, asset_next);
      start_seg = i;
      asset_prev = asset;
      asset = asset_next;
    }
  }

  generateRoadSegments(start_seg, points.size() - 1, asset_prev, asset, isClosed() ? getRoad(points[0]) : NULL);

  if (!isClosed())
    points.back()->removeRoadGeom();
}

void SplineObject::updateChangedSegmentsLoftGeom()
{
  int start_seg = 0;
  for (int i = 1; i < (int)points.size() - 1; i++)
  {
    if (points[i]->getSplineClass() != points[start_seg]->getSplineClass())
    {
      for (int j = start_seg; j < i; j++)
        if (points[j]->segChanged)
        {
          generateLoftSegments(start_seg, i);
          break;
        }
      start_seg = i;
    }
  }

  for (int j = start_seg; j < points.size(); j++)
    if (points[j]->segChanged)
    {
      generateLoftSegments(start_seg, points.size() - (poly ? 0 : 1));
      break;
    }

  if (!isClosed() && !poly)
    points.back()->removeLoftGeom();
}

UndoRedoObject *SplineObject::makePointListUndo()
{
  class UndoPointsListChange : public UndoRedoObject
  {
  public:
    PtrTab<SplinePointObject> pt;
    Ptr<SplineObject> s;

    UndoPointsListChange(SplineObject *_s) : s(_s), pt(_s->points) {}

    virtual void restore(bool save_redo)
    {
      PtrTab<SplinePointObject> pt1(s->points);
      s->points = pt;
      for (int i = pt.size() - 1; i >= 0; i--)
        pt[i]->arrId = i;
      pt = pt1;
      s->pointChanged(-1);
    }

    virtual void redo()
    {
      PtrTab<SplinePointObject> pt1(s->points);
      s->points = pt;
      for (int i = pt.size() - 1; i >= 0; i--)
        pt[i]->arrId = i;
      pt = pt1;
      s->pointChanged(-1);
    }

    virtual size_t size() { return sizeof(*this) + pt.size() * 4; }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoSplinePtListChange"; }
  };

  return new (midmem) UndoPointsListChange(this);
}


SplineObject *SplineObject::clone()
{
  Ptr<SplineObject> obj = new SplineObject(poly);
  obj->setEditLayerIdx(EditLayerProps::activeLayerIdx[obj->lpIndex()]);

  getObjEditor()->setUniqName(obj, getName());
  getObjEditor()->addObject(obj);

  for (int i = 0; i < points.size(); i++)
  {
    Ptr<SplinePointObject> p = new SplinePointObject;
    p->arrId = i;
    p->spline = obj;
    getObjEditor()->addObject(p);
    p->setWtm(points[i]->getWtm());
    p->setProps(points[i]->getProps());
  }
  obj->props.blkGenName = props.blkGenName;
  obj->markAssetChanged(0);
  return obj;
}

void SplineObject::putMoveUndo()
{
  HmapLandObjectEditor *ed = (HmapLandObjectEditor *)getObjEditor();
  if (!ed->isCloneMode())
    RenderableEditableObject::putMoveUndo();
}

void SplineObject::attachTo(SplineObject *s, int to_idx)
{
  UndoAttachSpline *undoAttach = new UndoAttachSpline(s, this, (HmapLandObjectEditor *)getObjEditor());

  getObjEditor()->getUndoSystem()->put(points[0]->makePropsUndoObj());
  getObjEditor()->getUndoSystem()->put(points.back()->makePropsUndoObj());

  getObjEditor()->getUndoSystem()->put(undoAttach);

  if (to_idx == -1)
    append_items(s->points, points.size(), points.data());
  else
    insert_items(s->points, to_idx, points.size(), points.data());

  for (int i = 0; i < s->points.size(); i++)
  {
    s->points[i]->spline = s;
    s->points[i]->arrId = i;
  }

  if (to_idx == 0)
    s->props.blkGenName = props.blkGenName;

  clear_and_shrink(points);
  getObjEditor()->removeObject(this);

  s->prepareSplineClassInPoints();
  s->pointChanged(-1);
  s->getSpline();
}

DagorAsset *SplineObject::getMaterialAsset(int idx) const
{
  if (points.size() == 0)
    return nullptr;

  ISplineGenObj *gen = points[0]->getSplineGen();
  if (!gen || !gen->splineClass || !gen->splineClass->genGeom)
    return nullptr;

  for (const splineclass::LoftGeomGenData::Loft &loft : gen->splineClass->genGeom->loft)
  {
    if (idx < loft.matNames.size())
    {
      const SimpleString &matName = loft.matNames[idx];
      DagorAsset *a = DAEDITOR3.getAssetByName(matName.c_str(), DAEDITOR3.getAssetTypeId("mat"));
      return a;
    }
    idx -= loft.matNames.size();
  }

  return nullptr;
}

bool SplineObject::isUsingMaterial(const char *mat_name_to_find) const
{
  if (points.size() == 0)
    return false;

  ISplineGenObj *gen = points[0]->getSplineGen();
  if (!gen || !gen->splineClass || !gen->splineClass->genGeom)
    return false;

  for (const auto &loft : gen->splineClass->genGeom->loft)
  {
    for (const auto &matName : loft.matNames)
    {
      if (strcmp(matName.c_str(), mat_name_to_find) == 0)
        return true;
    }
  }

  return false;
}

void SplineObject::makeMonoUp()
{
  float prevPosY = points[0]->getPt().y;
  for (int i = 1; i < points.size(); i++)
  {
    Point3 curPos = points[i]->getPt();
    if (curPos.y < prevPosY)
    {
      curPos.y = prevPosY;
      points[i]->setPos(curPos);
    }
    else
      prevPosY = curPos.y;
  }
}
void SplineObject::makeMonoDown()
{
  float prevPosY = points[0]->getPt().y;
  for (int i = 1; i < points.size(); i++)
  {
    Point3 curPos = points[i]->getPt();
    if (curPos.y > prevPosY)
    {
      curPos.y = prevPosY;
      points[i]->setPos(curPos);
    }
    else
      prevPosY = curPos.y;
  }
}
void SplineObject::makeLinearHt()
{
  BezierSpline2d s;
  getSplineXZ(s);
  if (s.leng < 1e-3)
    return;

  float h0 = points[0]->getPt().y;
  float h1 = points.back()->getPt().y;

  for (int i = 1; i < s.segs.size(); ++i)
    points[i]->setPos(Point3::xVz(points[i]->getPt(), lerp(h0, h1, s.segs[i - 1].tlen / s.leng)));
}
void SplineObject::applyCatmull(bool xz, bool y)
{
  if (!xz && !y)
    return;

  SplinePointObject *catmul[4];
  BezierSplineInt<Point3> sp;
  Point3 v[4];
  int pn = points.size();

  for (int i = -1; i < pn - 2; i++)
  {
    for (int j = 0; j < 4; j++)
      catmul[j] = points[(poly || isClosed()) ? (i + j + pn) % pn : clamp(i + j, 0, pn - 1)];

    for (int j = 0; j < 4; j++)
      v[j] = catmul[j]->getPt();
    sp.calculateCatmullRom(v);
    sp.calculateBack(v);

    Point3 pi0 = v[0] - v[1], pi1 = v[2] - v[3];
    if (xz && y)
    {
      catmul[1]->setRelBezierIn(pi0);
      catmul[1]->setRelBezierOut(-pi0);
      catmul[2]->setRelBezierIn(pi1);
      catmul[2]->setRelBezierOut(-pi1);
    }
    else if (xz)
    {
      catmul[1]->setRelBezierIn(Point3::xVz(pi0, catmul[1]->getProps().relIn.y));
      catmul[1]->setRelBezierOut(Point3::xVz(-pi0, catmul[1]->getProps().relOut.y));
      catmul[2]->setRelBezierIn(Point3::xVz(pi1, catmul[2]->getProps().relIn.y));
      catmul[2]->setRelBezierOut(Point3::xVz(-pi1, catmul[2]->getProps().relOut.y));
    }
    else if (y)
    {
      catmul[1]->setRelBezierIn(Point3::xVz(catmul[1]->getProps().relIn, pi0.y));
      catmul[1]->setRelBezierOut(Point3::xVz(catmul[1]->getProps().relOut, -pi0.y));
      catmul[2]->setRelBezierIn(Point3::xVz(catmul[2]->getProps().relIn, pi1.y));
      catmul[2]->setRelBezierOut(Point3::xVz(catmul[2]->getProps().relOut, -pi1.y));
    }
  }
  for (int i = 0; i < points.size(); i++)
  {
    points[i]->markChanged();
    if (getObjEditor())
      getObjEd().updateCrossRoads(points[i]);
  }
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

void SplineObject::getSmoothPoly(Tab<Point3> &pts)
{
  if (!poly)
    return;
  if (!props.poly.smooth)
  {
    pts.reserve(points.size());
    for (int i = 0; i < points.size(); i++)
      pts.push_back(points[i]->getPt());
    return;
  }

  getSpline();
  const BezierSplinePrec3d &path = ::toPrecSpline(bezierSpline);

  for (int i = 0; i < path.segs.size(); i++)
    buildSegment(pts, path.segs[i], path.segs[i].len, false, props.poly.minStep, props.poly.maxStep, props.poly.curvStrength);
}

struct SplineCrossPtDesc
{
  int splIdx, segIdx;
  float locT;
  static int cmp(const SplineCrossPtDesc *a, const SplineCrossPtDesc *b)
  {
    if (a->splIdx != b->splIdx)
      return a->splIdx - b->splIdx;
    if (a->segIdx != b->segIdx)
      return b->segIdx - a->segIdx;
    if (a->locT < b->locT)
      return 1;
    return a->locT > b->locT ? -1 : 0;
  }
};
static void add_when_differs(Tab<SplineCrossPtDesc> &cpt, const SplineCrossPtDesc &d)
{
  if (d.locT < 1e-6 || d.locT > 1.f - 1e-5)
    return;
  for (int i = 0; i < cpt.size(); i++)
    if (cpt[i].splIdx == d.splIdx && cpt[i].segIdx == d.segIdx && fabsf(cpt[i].locT - d.locT) < 1e-4)
      return;
  cpt.push_back(d);
}
static int find_crosses(const Point2 &lp0, const Point2 &cp0, const BezierSplinePrecInt3d &seg1, float step,
  Tab<SplineCrossPtDesc> &cpt, int _si, int _i, float _lti0, float _lti1, int _sj, int _j)
{
  Point2 resp;
  int cnum = 0;
  Point2 lp1 = Point2::xz(seg1.point(0.f)), cp1 = lp1;
  float lt1 = 0;
  for (float s = step, seg_len = seg1.len - step; s <= seg_len; s += step)
  {
    const float curT1 = seg1.getTFromS(s);
    cp1 = Point2::xz(seg1.point(curT1));
    if (get_lines_intersection(lp0, cp0, lp1, cp1, &resp))
    {
      SplineCrossPtDesc d0 = {_si, _i, lerp(_lti0, _lti1, ((resp - lp0) * (cp0 - lp0)) / (cp0 - lp0).lengthSq())};
      SplineCrossPtDesc d1 = {_sj, _j, lerp(lt1, curT1, ((resp - lp1) * (cp1 - lp1)) / (cp1 - lp1).lengthSq())};
      cnum++;
      add_when_differs(cpt, d0);
      add_when_differs(cpt, d1);
    }
    lp1 = cp1;
    lt1 = curT1;
  }
  cp1 = Point2::xz(seg1.point(1.f));
  if (get_lines_intersection(lp0, cp0, lp1, cp1, &resp))
  {
    SplineCrossPtDesc d0 = {_si, _i, lerp(_lti0, _lti1, ((resp - lp0) * (cp0 - lp0)) / (cp0 - lp0).lengthSq())};
    SplineCrossPtDesc d1 = {_sj, _j, lerp(lt1, 1.f, ((resp - lp1) * (cp1 - lp1)) / (cp1 - lp1).lengthSq())};
    cnum++;
    add_when_differs(cpt, d0);
    add_when_differs(cpt, d1);
  }
  return cnum;
}

int SplineObject::makeSplinesCrosses(dag::ConstSpan<SplineObject *> spls)
{
  Tab<SplineCrossPtDesc> cpt;
  const float step = 4;
  int cnum = 0;
  for (int si = 0; si < spls.size(); si++)
    spls[si]->getSpline();

  for (int si = 0; si < spls.size(); si++)
    for (int sj = si + 1; sj < spls.size(); sj++)
    {
      if (!(spls[si]->getSplineBox() & spls[sj]->getSplineBox()))
        continue;
      for (int i = 0; i < spls[si]->segSph.size(); i++)
        for (int j = 0; j < spls[sj]->segSph.size(); j++)
        {
          if (!(spls[si]->segSph[i] & spls[sj]->segSph[j]))
            continue;

          // finally test intersection of spline[si].seg[i] and spline[sj].seg[j]
          BezierSplinePrecInt3d seg0 = ::toPrecSpline(spls[si]->bezierSpline).segs[i];
          BezierSplinePrecInt3d seg1 = ::toPrecSpline(spls[sj]->bezierSpline).segs[j];

          Point3 lp0 = seg0.point(0.f);
          float lt0 = 0;
          for (float s = step, seg_len = seg0.len - step; s <= seg_len; s += step)
          {
            const float curT0 = seg0.getTFromS(s);
            Point3 cp0 = seg0.point(curT0);
            cnum += find_crosses(Point2::xz(lp0), Point2::xz(cp0), seg1, step, cpt, si, i, lt0, curT0, sj, j);
            lp0 = cp0;
            lt0 = curT0;
          }
          cnum += find_crosses(Point2::xz(lp0), Point2::xz(seg0.point(1.f)), seg1, step, cpt, si, i, lt0, 1.f, sj, j);
        }
    }
  if (!cpt.size())
    return 0;

  sort(cpt, &SplineCrossPtDesc::cmp);
  spls[0]->getObjEditor()->getUndoSystem()->begin();
  Point3 p_pos;
  for (int i = 0; i < cpt.size(); i++)
  {
    float locT = cpt[i].locT;
    if (i > 0 && cpt[i - 1].splIdx == cpt[i].splIdx && cpt[i - 1].segIdx == cpt[i].segIdx)
      locT /= cpt[i - 1].locT;
    spls[cpt[i].splIdx]->refine(cpt[i].segIdx, locT, p_pos);
  }
  spls[0]->getObjEditor()->getUndoSystem()->accept(String(0, "Make %d crosspoints for %d splines", cnum, spls.size()));

  return cnum;
}
