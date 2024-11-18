// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlSplinePoint.h"
#include "hmlSplineObject.h"
#include "hmlPlugin.h"
#include "hmlSplineUndoRedo.h"

#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <drv/3d/dag_driver.h>

#include <libTools/util/strUtil.h>
#include <sepGui/wndGlobal.h>

#include "hmlCm.h"
#include "common.h"
#include <de3_genObjAlongSpline.h>

#include <propPanel/control/container.h>

#include <winGuiWrapper/wgw_input.h>

using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;
using editorcore_extapi::make_full_start_path;

class HmapLandObjectEditor;


E3DCOLOR SplinePointObject::norm_col(200, 10, 10), SplinePointObject::norm_col_start_point(10, 10, 200),
  SplinePointObject::sel_col(255, 255, 255), SplinePointObject::sel2_col(190, 190, 190), SplinePointObject::hlp_col(10, 200, 10);
TEXTUREID SplinePointObject::texPt = BAD_TEXTUREID;
float SplinePointObject::ptScreenRad = 7.0;
int SplinePointObject::ptRenderPassId = -1;
SplinePointObject::Props *SplinePointObject::defaultProps = NULL;

enum
{
  PID_USEDEFSET = 1,
  PID_SPLINE_LAYERS,
  PID_GENBLKNAME,
  PID_EFFECTIVE_ASSET,
  PID_FUSE_POINTS,
  PID_LEVEL_TANGENTS,
  PID_SCALE_H,
  PID_SCALE_W,
  PID_OPACITY,
  PID_TC3U,
  PID_TC3V,
  PID_FOLLOW,
  PID_ROADBHV,
  PID_CORNER_TYPE,
  PID_LEVEL_POINTS,
};

void SplinePointObject::Props::defaults()
{
  pt = Point3(0, 0, 0);
  relIn = Point3(0, 0, 0);
  relOut = Point3(0, 0, 0);
  useDefSet = true;
  attr.scale_h = attr.scale_w = attr.opacity = 1.0f;
  attr.tc3u = attr.tc3v = 1.0f;
  attr.followOverride = -1;
  attr.roadBhvOverride = -1;
  cornerType = -2;
}

void SplinePointObject::initStatics()
{
  if (texPt == BAD_TEXTUREID)
  {
    texPt = dagRender->addManagedTexture(::make_full_start_path("../commonData/tex/point.tga"));
    dagRender->acquireManagedTex(texPt);
  }
  if (ptRenderPassId == -1)
    ptRenderPassId = dagGeom->getShaderVariableId("editor_rhv_tex_pass");
  if (!defaultProps)
  {
    defaultProps = new Props;
    defaultProps->defaults();
  }
  SplinePointObject::ptScreenRad = float(hdpi::_pxS(7));
}


SplinePointObject::SplinePointObject() : selObj(SELOBJ_POINT), targetSelObj(SELOBJ_POINT)
{
  objFlags |= FLG_WANTRESELECT;
  props = *defaultProps;
  props.pt = getPos();

  arrId = -1;
  spline = NULL;
  visible = true;
  isCross = false;
  isRealCross = false;
  segChanged = false;
  roadGeom = NULL;
  roadGeomBox.setempty();
  tmpUpDir = Point3(0, 1, 0);
  ppanel_ptr = NULL;
}

SplinePointObject::~SplinePointObject() { resetSplineClass(); }

void SplinePointObject::renderPts(DynRenderBuffer &dynBuf, const TMatrix4 &gtm, const Point2 &s, bool start)
{
  if (!visible)
    return;

  E3DCOLOR rendCol = isSelected() ? (selObj == SELOBJ_POINT ? sel_col : sel2_col) : (start ? norm_col_start_point : norm_col);

  renderPoint(dynBuf, toPoint4(props.pt, 1) * gtm, s * ptScreenRad, rendCol);

  if (isSelected() && !spline->isPoly() && spline->getProps().cornerType > -1)
  {
    renderPoint(dynBuf, toPoint4(props.pt + props.relIn, 1) * gtm, s * ptScreenRad * 0.6, selObj == SELOBJ_IN ? sel_col : hlp_col);
    renderPoint(dynBuf, toPoint4(props.pt + props.relOut, 1) * gtm, s * ptScreenRad * 0.6, selObj == SELOBJ_OUT ? sel_col : hlp_col);
  }
}
void SplinePointObject::renderPoint(DynRenderBuffer &db, const Point4 &p, const Point2 &s, E3DCOLOR c)
{
  if (p.w > 0.0)
  {
    Point3 dx(s.x, 0, 0);
    Point3 dy(0, s.y, 0);
    Point3 p3(p.x / p.w, p.y / p.w, p.z / p.w);
    if (p3.z > 0 && p3.z < 1)
      dagRender->dynRenderBufferDrawQuad(db, p3 - dx - dy, p3 - dx + dy, p3 + dx + dy, p3 + dx - dy, c, 1, 1);
  }
}

bool SplinePointObject::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
{
  if (spline && EditLayerProps::layerProps[spline->getEditLayerIdx()].hide)
    return false;
  if (pt_intersects(props.pt, ptScreenRad, rect, vp))
  {
    (int &)targetSelObj = SELOBJ_POINT;
    return true;
  }

  if (isSelected() && spline->getProps().cornerType > -1)
  {
    if (pt_intersects(props.pt + props.relIn, ptScreenRad * 0.6, rect, vp))
    {
      (int &)targetSelObj = SELOBJ_IN;
      return true;
    }
    if (pt_intersects(props.pt + props.relOut, ptScreenRad * 0.6, rect, vp))
    {
      (int &)targetSelObj = SELOBJ_OUT;
      return true;
    }
  }
  return false;
}
bool SplinePointObject::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
{
  if (spline && EditLayerProps::layerProps[spline->getEditLayerIdx()].hide)
    return false;
  if (pt_intersects(props.pt, ptScreenRad, x, y, vp))
  {
    (int &)targetSelObj = SELOBJ_POINT;
    return true;
  }

  if (isSelected() && spline->getProps().cornerType > -1)
  {
    if (pt_intersects(props.pt + props.relIn, ptScreenRad * 0.6, x, y, vp))
    {
      (int &)targetSelObj = SELOBJ_IN;
      return true;
    }
    if (pt_intersects(props.pt + props.relOut, ptScreenRad * 0.6, x, y, vp))
    {
      (int &)targetSelObj = SELOBJ_OUT;
      return true;
    }
  }
  return false;
}
bool SplinePointObject::getWorldBox(BBox3 &box) const
{
  box.setempty();
  box += props.pt - Point3(0.3, 0.3, 0.3);
  box += props.pt + Point3(0.3, 0.3, 0.3);
  return true;
}

void SplinePointObject::fillProps(PropPanel::ContainerPropertyControl &op, DClassID for_class_id,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  ppanel_ptr = &op;
  if (!spline)
    return;

  PropPanel::ContainerPropertyControl *commonGrp = op.getContainerById(PID_COMMON_GROUP);

  bool one_type = true;
  int one_type_idx = -1;
  int one_layer = -1;

  bool targetLayerEnabled = true;
  for (int i = 0; i < objects.size(); ++i)
    if (SplinePointObject *o = RTTI_cast<SplinePointObject>(objects[i]))
    {
      if (!o->spline)
      {
        one_layer = -2;
        targetLayerEnabled = false;
      }
      else
      {
        int type_idx = o->spline->isPoly() ? EditLayerProps::PLG : EditLayerProps::SPL;
        if (one_type_idx == type_idx || one_type_idx == -1)
          one_type_idx = type_idx;
        else
        {
          one_type_idx = -2;
          targetLayerEnabled = false;
        }

        if (one_layer == -1)
          one_layer = o->spline->getEditLayerIdx();
        else if (one_layer != o->spline->getEditLayerIdx())
          one_layer = -2;
      }
    }
    else
    {
      one_layer = -2;
      one_type = false;
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

  if (one_type)
  {
    commonGrp->createCheckBox(PID_USEDEFSET, "Use default spline settings", props.useDefSet);

    commonGrp->createEditFloat(PID_SCALE_H, "Scale Height", props.attr.scale_h);
    commonGrp->createEditFloat(PID_SCALE_W, "Scale Width", props.attr.scale_w);
    commonGrp->createTrackFloat(PID_OPACITY, "Opacity", props.attr.opacity, 0.0f, 1.0f, 0.01f);
    commonGrp->createEditFloat(PID_TC3U, "TC3.u", props.attr.tc3u);
    commonGrp->createEditFloat(PID_TC3V, "TC3.v", props.attr.tc3v);

    commonGrp->setMinMaxStep(PID_SCALE_H, 0.0f, 1e6f, 0.01f);
    commonGrp->setMinMaxStep(PID_SCALE_W, 0.0f, 1e6f, 0.01f);
    commonGrp->setMinMaxStep(PID_OPACITY, 0.0f, 1.0f, 0.01f);

    {
      commonGrp->createIndent();

      PropPanel::ContainerPropertyControl &followGrp = *commonGrp->createRadioGroup(PID_FOLLOW, "Follow type override");
      followGrp.createRadio(-1, "Default (as in asset)");
      followGrp.createRadio(0, "No follow");
      followGrp.createRadio(1, "Follow hills");
      followGrp.createRadio(2, "Follow hollows");
      followGrp.createRadio(3, "Follow landscape");
      commonGrp->setInt(PID_FOLLOW, props.attr.followOverride);

      PropPanel::ContainerPropertyControl &roadGrp = *commonGrp->createRadioGroup(PID_ROADBHV, "Road behavior override");
      roadGrp.createRadio(-1, "Default (as in asset)");
      roadGrp.createRadio(0, "Not a road");
      roadGrp.createRadio(1, "Follow as road");
      commonGrp->setInt(PID_ROADBHV, props.attr.roadBhvOverride);

      commonGrp->createIndent();
      commonGrp->createStatic(-1, "Spline class asset");
      commonGrp->createButton(PID_GENBLKNAME, ::dd_get_fname(props.blkGenName), !props.useDefSet);

      if (props.useDefSet)
      {
        commonGrp->createStatic(-1, "Effective spline class asset");
        commonGrp->createButton(PID_EFFECTIVE_ASSET, ::dd_get_fname(getEffectiveAsset()));
      }
      else
      {
        commonGrp->createStatic(-1, "Previous spline class asset");
        commonGrp->createButton(PID_EFFECTIVE_ASSET, ::dd_get_fname(getEffectiveAsset(-1)));
      }
    }

    commonGrp->createSeparator(0);
    PropPanel::ContainerPropertyControl &cornerGrp = *commonGrp->createRadioGroup(PID_CORNER_TYPE, "Corner type");
    cornerGrp.createRadio(-2, "As spline (default)");
    cornerGrp.createRadio(-1, "Corner");
    cornerGrp.createRadio(0, "Smooth tangent");
    cornerGrp.createRadio(1, "Smooth curvature");
    commonGrp->setInt(PID_CORNER_TYPE, props.cornerType);

    commonGrp->createSeparator(0);
    commonGrp->createButton(PID_LEVEL_TANGENTS, String(64, "Level tangents (%d points)", objects.size()));
    if (objects.size() > 1)
    {
      commonGrp->createButton(PID_LEVEL_POINTS, String(64, "Level %d points (set average .y)", objects.size()));
      commonGrp->createButton(PID_FUSE_POINTS, String(64, "Fuse %d points", objects.size()));
    }
  }

  int eff_disp = props.useDefSet ? 0 : -1;
  for (int i = 0; i < objects.size(); ++i)
  {
    SplinePointObject *o = RTTI_cast<SplinePointObject>(objects[i]);
    if (!o)
      continue;

    if (o->props.useDefSet != props.useDefSet)
      commonGrp->resetById(PID_USEDEFSET);
    if (stricmp(o->props.blkGenName, props.blkGenName))
      commonGrp->setCaption(PID_GENBLKNAME, "");

    if (o->props.useDefSet == props.useDefSet)
      if (stricmp(o->getEffectiveAsset(eff_disp), getEffectiveAsset(eff_disp)))
        commonGrp->setCaption(PID_EFFECTIVE_ASSET, "");

    if (o->props.cornerType != props.cornerType)
      commonGrp->resetById(PID_CORNER_TYPE);
  }
}

void SplinePointObject::onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  if (!edit_finished)
    return;

  if (pid == PID_USEDEFSET)
  {
    bool val = panel.getBool(pid);
    for (int i = 0; i < objects.size(); ++i)
    {
      SplinePointObject *o = RTTI_cast<SplinePointObject>(objects[i]);
      if (!o || o->props.useDefSet == val)
        continue;
      getObjEditor()->getUndoSystem()->put(new UndoChangeAsset(o->spline, o->arrId));

      o->props.useDefSet = val;
      o->resetSplineClass();
      if (o->spline)
        o->spline->markAssetChanged(o->arrId);

      if (o->props.useDefSet)
        if (ISplineGenObj *gen = o->getSplineGen())
          gen->clearLoftGeom();
    }
    getObjEditor()->invalidateObjectProps();
  }

  if (pid == PID_SPLINE_LAYERS)
  {
    dag::Vector<RenderableEditableObject *> objectsToMove;
    objectsToMove.set_capacity(objects.size());
    int oneTypeIdx = -1;
    for (int i = 0; i < objects.size(); ++i)
    {
      if (SplinePointObject *s = RTTI_cast<SplinePointObject>(objects[i]))
      {
        if (s->spline)
        {
          if (eastl::find(objectsToMove.begin(), objectsToMove.end(), s->spline) == objectsToMove.end())
            objectsToMove.push_back(s->spline);

          int typeIdx = s->spline->isPoly() ? EditLayerProps::PLG : EditLayerProps::SPL;
          if (oneTypeIdx == typeIdx || oneTypeIdx == -1)
          {
            oneTypeIdx = typeIdx;
            continue;
          }
        }
      }
      oneTypeIdx = -2;
    }

    int selectedLayer = panel.getInt(PID_SPLINE_LAYERS);
    int layerIdx = EditLayerProps::findLayerByTypeIdx(oneTypeIdx, selectedLayer);
    if (layerIdx != -1)
    {
      HmapLandPlugin::self->moveObjectsToLayer(layerIdx, make_span(objectsToMove));

      DAGORED2->invalidateViewportCache();
      getObjEditor()->invalidateObjectProps();
    }
  }

  if ((pid == PID_SCALE_H) || (pid == PID_SCALE_W) || (pid == PID_OPACITY) || (pid == PID_TC3U) || (pid == PID_TC3V))
  {
    float val = panel.getFloat(pid);
    for (int i = 0; i < objects.size(); ++i)
    {
      SplinePointObject *o = RTTI_cast<SplinePointObject>(objects[i]);
      if (!o)
        continue;

      getObjEditor()->getUndoSystem()->put(new UndoPropsChange(o));
      if (pid == PID_SCALE_H)
        o->props.attr.scale_h = val;
      if (pid == PID_SCALE_W)
        o->props.attr.scale_w = val;
      if (pid == PID_OPACITY)
        o->props.attr.opacity = val;
      if (pid == PID_TC3U)
        o->props.attr.tc3u = val;
      if (pid == PID_TC3V)
        o->props.attr.tc3v = val;
      if (o->spline)
        o->markChanged();
    }
  }
  if (pid == PID_FOLLOW)
  {
    int val = panel.getInt(pid);
    for (int i = 0; i < objects.size(); ++i)
    {
      SplinePointObject *o = RTTI_cast<SplinePointObject>(objects[i]);
      if (!o)
        continue;

      getObjEditor()->getUndoSystem()->put(new UndoPropsChange(o));
      o->props.attr.followOverride = val;
      if (o->spline)
        o->markChanged();
    }
  }
  if (pid == PID_ROADBHV)
  {
    int val = panel.getInt(pid);
    for (int i = 0; i < objects.size(); ++i)
    {
      SplinePointObject *o = RTTI_cast<SplinePointObject>(objects[i]);
      if (!o)
        continue;

      getObjEditor()->getUndoSystem()->put(new UndoPropsChange(o));
      o->props.attr.roadBhvOverride = val;
      if (o->spline)
        o->markChanged();
    }
  }
  if (pid == PID_CORNER_TYPE)
  {
    int val = panel.getInt(pid);
    for (int i = 0; i < objects.size(); ++i)
    {
      SplinePointObject *o = RTTI_cast<SplinePointObject>(objects[i]);
      if (!o)
        continue;

      getObjEditor()->getUndoSystem()->put(new UndoPropsChange(o));
      o->props.cornerType = val;
      if (o->spline)
      {
        o->markChanged();
        o->spline->invalidateSplineCurve();
      }
    }
  }
}
void SplinePointObject::onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  if (pid == PID_GENBLKNAME || pid == PID_EFFECTIVE_ASSET)
  {
    if (!spline)
      return;

    int eff_disp = (pid == PID_GENBLKNAME || props.useDefSet) ? 0 : -1;
    const char *asset_name = getEffectiveAsset(eff_disp);

    asset_name = DAEDITOR3.selectAssetX(asset_name, "Select splineclass", "spline");

    for (int i = 0; i < objects.size(); i++)
    {
      SplinePointObject *o = RTTI_cast<SplinePointObject>(objects[i]);
      if (!o || o->props.useDefSet != props.useDefSet)
        continue;
      o->setEffectiveAsset(asset_name, true, eff_disp);
    }
    panel.setText(pid, asset_name);
    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  else if (pid == PID_FUSE_POINTS || pid == PID_LEVEL_POINTS)
  {
    Point3 newPos(0, 0, 0);
    int pointsCount = 0;
    for (int i = 0; i < objects.size(); ++i)
    {
      SplinePointObject *o = RTTI_cast<SplinePointObject>(objects[i]);
      G_ASSERT(o && "Select not only spline points");

      newPos += o->getPt();
      pointsCount++;
    }

    G_ASSERT(pointsCount && "Select 0 points");
    newPos = Point3(newPos.x / pointsCount, newPos.y / pointsCount, newPos.z / pointsCount);
    for (int i = 0; i < objects.size(); ++i)
    {
      SplinePointObject *o = RTTI_cast<SplinePointObject>(objects[i]);

      if (o->spline)
      {
        o->putMoveUndo();
        o->spline->putObjTransformUndo();
      }

      if (pid == PID_FUSE_POINTS)
        o->setPos(newPos);
      else if (pid == PID_LEVEL_POINTS)
        o->setPos(Point3::xVz(o->getPt(), newPos.y));
    }
    IEditorCoreEngine::get()->invalidateViewportCache();
  }
  else if (pid == PID_LEVEL_TANGENTS)
  {
    for (int i = 0; i < objects.size(); ++i)
    {
      SplinePointObject *o = RTTI_cast<SplinePointObject>(objects[i]);

      getObjEditor()->getUndoSystem()->put(new UndoPropsChange(o));
      if (o->spline)
        o->spline->putObjTransformUndo();

      o->props.relIn.y = o->props.relOut.y = 0;

      o->markChanged();
      if (o->spline)
        o->spline->getSpline();
      getObjEd().updateCrossRoads(o);
    }
    IEditorCoreEngine::get()->invalidateViewportCache();
  }
}

void SplinePointObject::onPPClose(PropPanel::ContainerPropertyControl &panel, dag::ConstSpan<RenderableEditableObject *> objects)
{
  ppanel_ptr = NULL;
}

void SplinePointObject::saveProps(const SplinePointObject::Props &props, DataBlock &blk, bool bh)
{
  blk.setBool("useDefSplineGen", props.useDefSet);
  blk.setStr("blkGenName", props.blkGenName);
  blk.setPoint3("pt", props.pt);
  if (fabs(props.attr.scale_h - 1.0f) > 1e-6f)
    blk.setReal("scale_h", props.attr.scale_h);
  if (fabs(props.attr.scale_w - 1.0f) > 1e-6f)
    blk.setReal("scale_w", props.attr.scale_w);
  if (fabs(props.attr.opacity - 1.0f) > 1e-6f)
    blk.setReal("opacity", props.attr.opacity);
  if (fabs(props.attr.tc3u - 1.0f) > 1e-6f)
    blk.setReal("tc3u", props.attr.tc3u);
  if (fabs(props.attr.tc3v - 1.0f) > 1e-6f)
    blk.setReal("tc3v", props.attr.tc3v);
  if (props.attr.followOverride != -1)
    blk.setInt("followOverride", props.attr.followOverride);
  if (props.attr.roadBhvOverride != -1)
    blk.setInt("roadBhvOverride", props.attr.roadBhvOverride);
  if (bh)
  {
    blk.setPoint3("in", props.relIn);
    blk.setPoint3("out", props.relOut);
  }
  if (props.cornerType != -2)
    blk.setInt("cornerType", props.cornerType);
}
void SplinePointObject::loadProps(SplinePointObject::Props &props, const DataBlock &blk)
{
  props.useDefSet = blk.getBool("useDefSplineGen", props.useDefSet);
  props.blkGenName = blk.getStr("blkGenName", props.blkGenName);

  props.pt = blk.getPoint3("pt", props.pt);
  props.relIn = blk.getPoint3("in", Point3(0, 0, 0));
  props.relOut = blk.getPoint3("out", Point3(0, 0, 0));
  props.attr.scale_h = blk.getReal("scale_h", 1.0f);
  props.attr.scale_w = blk.getReal("scale_w", 1.0f);
  props.attr.opacity = blk.getReal("opacity", 1.0f);
  props.attr.tc3u = blk.getReal("tc3u", 1.0f);
  props.attr.tc3v = blk.getReal("tc3v", 1.0f);
  props.attr.followOverride = blk.getInt("followOverride", -1);
  props.attr.roadBhvOverride = blk.getInt("roadBhvOverride", -1);
  props.cornerType = blk.getInt("cornerType", -2);
}
void SplinePointObject::save(DataBlock &blk) { saveProps(props, blk, !spline->isPoly()); }
void SplinePointObject::load(const DataBlock &blk)
{
  loadProps(props, blk);
  matrix.setcol(3, props.pt);
}

Point3 SplinePointObject::getPtEffRelBezierIn() const
{
  int t = spline->getProps().cornerType;
  if (props.cornerType != -2)
    t = props.cornerType;
  if (t >= 0)
    return t == 0 ? props.relIn : (props.relIn - props.relOut) * 0.5f;

  int pn = spline->points.size();
  if (pn < 2)
    return Point3(0, 0, 0);
  if (arrId > 0)
    return normalize(spline->points[arrId - 1]->props.pt - props.pt) * 0.05;
  return normalize(props.pt - spline->points[arrId + 1]->props.pt) * 0.05;
}
Point3 SplinePointObject::getPtEffRelBezierOut() const
{
  int t = spline->getProps().cornerType;
  if (props.cornerType != -2)
    t = props.cornerType;
  if (t >= 0)
    return t == 0 ? props.relOut : (props.relOut - props.relIn) * 0.5f;

  int pn = spline->points.size();
  if (pn < 2)
    return Point3(0, 0, 0);
  if (arrId + 1 < pn)
    return normalize(spline->points[arrId + 1]->props.pt - props.pt) * 0.05;
  return normalize(props.pt - spline->points[arrId - 1]->props.pt) * 0.05;
}

void SplinePointObject::setWtm(const TMatrix &wtm)
{
  RenderableEditableObject::setWtm(wtm);

  if (selObj == SELOBJ_POINT)
    props.pt = getPos();
  else if (selObj == SELOBJ_IN)
  {
    props.relIn = getPos() - props.pt;
    props.relOut = -length(props.relOut) * normalize(props.relIn);
  }
  else if (selObj == SELOBJ_OUT)
  {
    props.relOut = getPos() - props.pt;
    props.relIn = -length(props.relIn) * normalize(props.relOut);
  }
}

void SplinePointObject::onAdd(ObjectEditor *objEditor)
{
  if (spline)
    spline->addPoint(this);

  getObjEd(objEditor).updateCrossRoadsOnPointAdd(this);
  if (getObjEditor())
    getObjEd(objEditor).updateCrossRoads(this);
}

void SplinePointObject::onRemove(ObjectEditor *objEditor)
{
  resetSplineClass();
  getObjEd(objEditor).updateCrossRoadsOnPointRemove(this);

  if (spline)
    spline->onPointRemove(arrId);
}


void SplinePointObject::selectObject(bool select)
{
  RenderableEditableObject::selectObject(select);

  if (select && selObj != targetSelObj)
  {
    selObj = targetSelObj;
    if (selObj == SELOBJ_POINT)
      matrix.setcol(3, props.pt);
    else if (selObj == SELOBJ_IN)
      matrix.setcol(3, props.relIn + props.pt);
    else if (selObj == SELOBJ_OUT)
      matrix.setcol(3, props.relOut + props.pt);
    if (objEditor)
    {
      objEditor->onObjectGeomChange(this);
      objEditor->updateGizmo();
    }
    targetSelObj = SELOBJ_POINT;
  }

  if (!select && selObj != SELOBJ_POINT)
  {
    targetSelObj = selObj = SELOBJ_POINT;
    matrix.setcol(3, props.pt);
  }
}

Point3 SplinePointObject::getUpDir() const { return Point3(0, 1, 0); }

int SplinePointObject::linkCount()
{
  if (arrId < 0 || arrId > spline->points.size() - 1)
    return -1;
  else if (arrId == 0 || arrId == spline->points.size() - 1)
    return 1;
  else
    return 2;
}

SplinePointObject *SplinePointObject::getLinkedPt(int id)
{
  if (arrId < 0 || arrId >= spline->points.size())
    return NULL;

  if (id == 0)
  {
    if (arrId == 0)
      return NULL;
    else
      return spline->points[arrId - 1];
  }
  else if (id == 1)
  {
    if (arrId == spline->points.size() - 1)
      return NULL;
    else
      return spline->points[arrId + 1];
  }
  return NULL;
}

void SplinePointObject::moveObject(const Point3 &delta, IEditorCoreEngine::BasisType basis)
{
  RenderableEditableObject::putMoveUndo();
  if (spline)
    spline->putObjTransformUndo();
  RenderableEditableObject::moveObject(delta, basis);

  objectWasMoved = true;

  if (wingw::is_key_pressed(wingw::V_CONTROL))
  {
    IGenViewportWnd *wnd = DAGORED2->getCurrentViewport();
    HmapLandObjectEditor *ed = (HmapLandObjectEditor *)getObjEditor();

    bool is = false;
    for (int i = 0; i < ed->splinesCount(); i++)
    {
      for (int j = 0; j < ed->getSpline(i)->points.size(); j++)
      {
        if (this == ed->getSpline(i)->points[j])
          continue;

        real ddist = ed->screenDistBetweenPoints(wnd, this, ed->getSpline(i)->points[j]);
        if (ddist <= 16 && ddist > 0)
        {
          setPos(ed->getSpline(i)->points[j]->getPos());
          is = true;
          break;
        }
      }
      if (is)
        break;
    }
  }

  if (spline)
    markChanged();
}

void SplinePointObject::scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis)
{
  static float old_h = props.attr.scale_h;
  static float old_w = props.attr.scale_w;

  props.attr.scale_h += delta.y - 1.0f;

  if (fabs(delta.x - 1.0f) > 0.01f)
    props.attr.scale_w += delta.x - 1.0f;
  else
    props.attr.scale_w += delta.z - 1.0f;

  if (((old_h != props.attr.scale_h) || (old_w != props.attr.scale_w)) && (ppanel_ptr))
  {
    ppanel_ptr->setFloat(PID_SCALE_H, props.attr.scale_h);
    ppanel_ptr->setFloat(PID_SCALE_W, props.attr.scale_w);
  }

  old_h = props.attr.scale_h;
  old_w = props.attr.scale_w;
}

bool SplinePointObject::setPos(const Point3 &p)
{
  bool ret = RenderableEditableObject::setPos(p);

  if (spline)
    spline->getSpline();

  markChanged();
  if (getObjEditor())
    getObjEd().updateCrossRoads(this);

  return ret;
}


void SplinePointObject::putMoveUndo()
{
  RenderableEditableObject::putMoveUndo();
  getObjEditor()->getUndoSystem()->put(makePropsUndoObj());
  if (spline)
    spline->putObjTransformUndo();
}


void SplinePointObject::putRotateUndo()
{
  RenderableEditableObject::putRotateUndo();
  getObjEditor()->getUndoSystem()->put(makePropsUndoObj());
}


void SplinePointObject::putScaleUndo()
{
  RenderableEditableObject::putScaleUndo();
  getObjEditor()->getUndoSystem()->put(makePropsUndoObj());
}


void SplinePointObject::swapHelpers()
{
  Point3 p = props.relIn;
  setRelBezierIn(props.relOut);
  setRelBezierOut(p);
}

BBox3 SplinePointObject::getGeomBox(bool render_loft, bool render_road)
{
  BBox3 worldBox;
  worldBox.setempty();
  if (render_loft)
    if (ISplineGenObj *gen = getSplineGen())
      worldBox += gen->loftGeomBox;
  if (render_road)
    worldBox += roadGeomBox;
  return worldBox;
}

void SplinePointObject::renderRoadGeom(bool opaque)
{
  if (roadGeom)
    opaque ? dagGeom->geomObjectRender(*roadGeom) : dagGeom->geomObjectRenderTrans(*roadGeom);
}

void SplinePointObject::resetSplineClass()
{
  clearSegment();
  destroy_it(splineGenObj);
  effSplineClass = nullptr;
}
splineclass::AssetData *SplinePointObject::prepareSplineClass(const char *def_blk_name, splineclass::AssetData *prev)
{
  if (hasSplineClass())
    def_blk_name = props.blkGenName;

  if (def_blk_name && def_blk_name[0])
    importGenerationParams(def_blk_name);
  else if (hasSplineClass())
    resetSplineClass();
  else
  {
    resetSplineClass();
    effSplineClass = prev;
  }

  return effSplineClass;
}
void SplinePointObject::importGenerationParams(const char *blkname)
{
  effSplineClass = nullptr;
  if (!blkname || !*blkname)
  {
    resetSplineClass();
    return;
  }

  static int spline_atype = DAEDITOR3.getAssetTypeId("spline");
  if (!splineGenObj)
    if (DagorAsset *a = DAEDITOR3.getAssetByName(blkname, spline_atype))
      splineGenObj = DAEDITOR3.createEntity(*a);
  ISplineGenObj *gen = getSplineGen();
  if (!gen)
    return;

  IAssetService *srv = DAGORED2->queryEditorInterface<IAssetService>();
  splineclass::AssetData *a = srv ? srv->getSplineClassData(blkname) : NULL;

  gen->splineLayer = spline->getLayer();
  setEditLayerIdx(spline->getEditLayerIdx());
  if (gen->splineClass != a)
  {
    if (srv)
      srv->releaseSplineClassData(gen->splineClass);
    gen->splineClass = a;
    clearSegment();
  }
  else if (srv && a)
    srv->releaseSplineClassData(a);

  effSplineClass = gen->splineClass;
}
void SplinePointObject::clearSegment()
{
  if (ISplineGenObj *gen = getSplineGen())
    gen->clear();
  removeRoadGeom();
}

void SplinePointObject::removeRoadGeom()
{
  dagGeom->deleteGeomObject(roadGeom);
  updateRoadBox();
}

void SplinePointObject::updateRoadBox()
{
  if (roadGeom)
    roadGeomBox = dagGeom->geomObjectGetBoundBox(*roadGeom);
  else
    roadGeomBox.setempty();
  if (spline)
    spline->updateRoadBox();
}

GeomObject &SplinePointObject::getClearedRoadGeom()
{
  removeRoadGeom();
  if (!roadGeom)
    return *(roadGeom = dagGeom->newGeomObject(midmem));
  dagGeom->geomObjectClear(*roadGeom);
  return *roadGeom;
}

void SplinePointObject::removeLoftGeom()
{
  if (ISplineGenObj *gen = getSplineGen())
  {
    gen->clearLoftGeom();
    if (spline)
      spline->updateLoftBox();
  }
}

void SplinePointObject::markChanged()
{
  segChanged = true;
  if (spline)
  {
    if (spline->points.size()) // spline->points.size() may be 0 when spline is being removed
    {
      if (arrId > 0)
        spline->points[arrId - 1]->segChanged = true;
      if (arrId + 1 < spline->points.size())
        spline->points[arrId + 1]->segChanged = true;
    }
    spline->splineChanged = true;
  }
  if (getObjEditor())
    getObjEd().splinesChanged = true;
}

void SplinePointObject::pointChanged()
{
  if (spline)
    spline->pointChanged(-1);
}

const char *SplinePointObject::getEffectiveAsset(int ofs) const
{
  if (!spline)
    return "";

  for (int i = arrId + ofs; i >= 0; i--)
    if (spline->points[i]->hasSplineClass())
      return spline->points[i]->props.blkGenName;
  return !spline->isPoly() ? spline->getProps().blkGenName.str() : "";
}

void SplinePointObject::setEffectiveAsset(const char *asset_name, bool _undo, int ofs)
{
  if (!spline)
    return;

  for (int i = arrId + ofs; i >= 0; i--)
    if (spline->points[i]->hasSplineClass())
    {
      SplinePointObject *o = spline->points[i];
      if (_undo)
        getObjEditor()->getUndoSystem()->put(new UndoChangeAsset(o->spline, o->arrId));

      o->props.blkGenName = asset_name;
      if (!asset_name)
        o->props.useDefSet = false;
      o->resetSplineClass();
      o->spline->markAssetChanged(o->arrId);
      return;
    }

  if (!spline->isPoly())
    spline->changeAsset(asset_name, _undo);
  else
  {
    SplinePointObject *o = spline->points[0];
    if (_undo)
      getObjEditor()->getUndoSystem()->put(new UndoChangeAsset(o->spline, o->arrId));

    o->props.blkGenName = asset_name;
    if (!asset_name)
      o->props.useDefSet = false;
    o->resetSplineClass();
    o->spline->markAssetChanged(o->arrId);
  }
}

void SplinePointObject::FlattenY()
{
  props.relIn.y = 0;
  props.relOut.y = 0;
}

void SplinePointObject::FlattenAverage(float mul)
{
  props.relIn.y *= mul;
  props.relOut.y *= mul;
}

void SplinePointObject::UndoPropsChange::restore(bool save_redo)
{
  if (save_redo)
    redoProps = obj->props;
  obj->props = oldProps;
  obj->pointChanged();
  if (obj->spline)
    obj->spline->invalidateSplineCurve();
  obj->getObjEd().invalidateObjectProps();
}

void SplinePointObject::UndoPropsChange::redo()
{
  obj->props = redoProps;
  obj->pointChanged();
  if (obj->spline)
    obj->spline->invalidateSplineCurve();
  obj->getObjEd().invalidateObjectProps();
}
