// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ivyObject.h"

#include <EditorCore/ec_IEditorCore.h>
#include <drv/3d/dag_driver.h>

#include "plugin.h"
#include <sceneRay/dag_sceneRay.h>
#include <math/dag_traceRayTriangle.h>
#include <scene/dag_frtdump.h>

#include <oldEditor/pluginService/de_IDagorPhys.h>

#include <de3_heightmap.h>
#include <de3_interface.h>
#include <assets/asset.h>
#include <heightMapLand/dag_hmlGetFaces.h>

#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_zlibIo.h>
#include <coolConsole/coolConsole.h>

#include <libTools/staticGeom/staticGeometryContainer.h>

#include <debug/dag_debug.h>

using namespace objgenerator; // prng
using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;
using editorcore_extapi::make_full_start_path;

extern IDagorPhys *dagPhys;

bool IvyObject::objectWasMoved = false;

E3DCOLOR IvyObject::norm_col(200, 10, 10), IvyObject::norm_col_start_point(10, 10, 200), IvyObject::sel_col(255, 255, 255),
  IvyObject::sel2_col(190, 190, 190), IvyObject::hlp_col(10, 200, 10);
TEXTUREID IvyObject::texPt = BAD_TEXTUREID;
float IvyObject::ptScreenRad = 9.0;
float IvyObject::ptScreenRadSm = 4.0;
int IvyObject::ptRenderPassId = -1;

enum
{
  GEN_TYPE_MAX_COUNT = 10,

  PID_PARAMS_GRP = 1,
  PID_GENBLKNAME,
  // PID_GENBLKNAME_RESET,
  PID_RESET_TO_ASSET,
  PID_IVYSIZE,
  PID_PRIMARYWEIGHT,
  PID_RANDOMWEIGHT,
  PID_GRAVITYWEIGHT,
  PID_ADHESIONWEIGHT,
  PID_BRANCHINGPROBABILITY,
  PID_MAXFLOATINGLENGTH,
  PID_MAXADHESIONDISTANCE,
  PID_IVYBRANCHSIZE,
  PID_IVYLEAFSIZE,
  PID_LEAFDENSITY,

  PID_GROWTH_GRP,
  PID_RANDOMSEED,
  PID_GROWITERATIONS,
  PID_AUTOGROW,
  // PID_DECREASE,
  PID_RESET,

  PID_GEOM_GRP,
  PID_GEN_TYPE,
  PID_NODES_ANGLE_DELTA = PID_GEN_TYPE + GEN_TYPE_MAX_COUNT,
  PID_MAX_TRIANGLES,
  PID_GENERATE_GEOM,
  PID_TRIS_INFO,

  PID_RENDER_GRP,
  PID_RENDER_DEBUG,
  PID_RENDER_GEOM,
};


static class MyCollisionCallback : public IvyObject::CollisionCallback
{
public:
  Tab<int> faces;
  MyCollisionCallback() : rt(NULL), hmap(NULL), faces(tmpmem) {}

  /** compute the adhesion of scene objects at a point pos*/
  Point3 computeAdhesion(const Point3 &pos, real maxDist, Point3 &norm) override
  {
    float minDist = maxDist;
    Point3 adhesionVector = Point3(0, 0, 0);

    if (rt)
      computeAdhesion(adhesionVector, pos, minDist, maxDist, rt->getRt(), norm);

    if (hmap)
    {
      Tab<HeightmapTriangle> hmapFaces(tmpmem);
      BBox3 ivyBB;
      ivyBB.lim[0] = pos - Point3(1, 1, 1) * maxDist;
      ivyBB.lim[1] = pos + Point3(1, 1, 1) * maxDist;
      get_faces_from_midpoint_heightmap(*hmap, hmapFaces, ivyBB);

      for (int i = 0; i < hmapFaces.size(); i++)
      {
        Point3 v0 = hmapFaces[i].v0;
        Point3 e1 = hmapFaces[i].e1;
        Point3 e2 = hmapFaces[i].e2;

        Point3 n = e2 % e1;
        n.normalize();

        real u, v;
        if (!traceRayToTriangleNoCull(pos, n, minDist, v0, e1, e2, u, v))
          continue;

        norm = -n;
        adhesionVector = n;
        adhesionVector *= 1.0f - minDist / maxDist;
      }
    }

    return adhesionVector;
  }

  void computeAdhesion(Point3 &adhesionVector, const Point3 &pos, real &minDist, real maxDist, StaticSceneRayTracer &rt, Point3 &norm)
  {
    faces.clear();
    rt.getFaces(faces, pos, maxDist);
    for (int i = 0; i < faces.size(); ++i)
    {
      int fi = faces[i];
      const StaticSceneRayTracer::RTface &face = rt.faces(fi);
      const StaticSceneRayTracer::RTfaceBoundFlags &bound = rt.facebounds(fi);
      Point3 v0 = rt.verts(face.v[0]);

      Point3 n = -bound.n;

      real u, v;
      if (!traceRayToTriangleNoCull(pos, n, minDist, v0, rt.verts(face.v[1]) - v0, rt.verts(face.v[2]) - v0, u, v))
        continue;

      norm = -n;
      adhesionVector = n;
      adhesionVector *= 1.0f - minDist / maxDist;
    }
  }

  bool traceRay(const Point3 &p, const Point3 &dir, float &dist, Point3 &normal) override
  {
    return DAGORED2->traceRay(p, dir, dist, &normal, false);
  }

  void init()
  {
    hmap = DAGORED2->getInterfaceEx<IHeightmap>();
    rt = dagPhys->getFastRtDump();
  }

  FastRtDump *rt;
  IHeightmap *hmap;

} ivyCb;


void IvyObject::initStatics()
{
  if (texPt == BAD_TEXTUREID)
  {
    texPt = dagRender->addManagedTexture(::make_full_start_path("../commonData/tex/point.tga"));
    dagRender->acquireManagedTex(texPt);
  }
  if (ptRenderPassId == -1)
    ptRenderPassId = dagGeom->getShaderVariableId("editor_rhv_tex_pass");
  ptScreenRad = float(hdpi::_pxS(9));
  ptScreenRadSm = float(hdpi::_pxS(4));
}


void IvyObject::initCollisionCallback() { ivyCb.init(); }


IvyObject::IvyObject(int _uid) : uid(_uid), roots(midmem)
{
  ivyGeom = dagGeom->newGeomObject(midmem);
  pt = getPos();
  setName(String(64, "IvyObject_%02d", uid));

  visible = true;
  growing = 0;
  growed = 0;
  clearIvy();

  nodesAngleDelta = 0;
  maxTriangles = 100000;
  trisCount = 0;

  needRenderDebug = true;
  needRenderGeom = true;
  needDecreaseGrow = false;

  canGrow = false;
  firstAct = true;

  genGeomType = GEOM_TYPE_PRISM;

  defaults();
}

void IvyObject::render()
{
  if (ivyGeom && needRenderGeom)
    dagGeom->geomObjectRender(*ivyGeom);
}

void IvyObject::renderTrans()
{
  if (needRenderDebug)
    debugRender();

  if (ivyGeom && needRenderGeom)
    dagGeom->geomObjectRenderTrans(*ivyGeom);
}

void IvyObject::renderPts(DynRenderBuffer &dynBuf, const TMatrix4 &gtm, const Point2 &s, bool start)
{
  if (!visible)
    return;

  bool curPlug = DAGORED2->curPlugin() == IvyGenPlugin::self;

  E3DCOLOR rendCol = (isSelected() && curPlug) ? sel_col : (start ? norm_col_start_point : norm_col);

  renderPoint(dynBuf, toPoint4(pt, 1) * gtm, s * (curPlug ? ptScreenRad : ptScreenRadSm), rendCol);
}

void IvyObject::renderPoint(DynRenderBuffer &db, const Point4 &p, const Point2 &s, E3DCOLOR c)
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

bool IvyObject::isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
{
  if (pt_intersects(pt, ptScreenRad, rect, vp))
    return true;

  return false;
}
bool IvyObject::isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
{
  if (pt_intersects(pt, ptScreenRad, x, y, vp))
    return true;

  return false;
}

bool IvyObject::getWorldBox(BBox3 &box) const
{
  box.setempty();
  box += pt - Point3(0.3, 0.3, 0.3);
  box += pt + Point3(0.3, 0.3, 0.3);
  return true;
}

void IvyObject::fillProps(PropPanel::ContainerPropertyControl &panel, DClassID for_class_id,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
  bool one_type = true;

  for (int i = 0; i < objects.size(); ++i)
  {
    IvyObject *o = RTTI_cast<IvyObject>(objects[i]);
    if (!o)
    {
      one_type = false;
      continue;
    }
  }

  if (one_type)
  {
    PropPanel::ContainerPropertyControl *grp = panel.createGroup(PID_PARAMS_GRP, "Ivy parameters");

    panel.createIndent();

    grp->createStatic(0, "Ivy settings asset file:");
    grp->createButton(PID_GENBLKNAME, assetName);
    grp->createIndent();
    grp->createButton(PID_RESET_TO_ASSET, "Reset params to asset");
    grp->createIndent();

    grp->createEditFloat(PID_IVYSIZE, "ivy size", size);
    grp->setMinMaxStep(PID_IVYSIZE, 0, 100, 0.01f);
    grp->createEditFloat(PID_PRIMARYWEIGHT, "primary weight", primaryWeight);
    grp->setMinMaxStep(PID_PRIMARYWEIGHT, 0, 1, 0.01f);
    grp->createEditFloat(PID_RANDOMWEIGHT, "random weight", randomWeight);
    grp->setMinMaxStep(PID_RANDOMWEIGHT, 0, 1, 0.01f);
    grp->createEditFloat(PID_GRAVITYWEIGHT, "gravity weight", gravityWeight);
    grp->setMinMaxStep(PID_GRAVITYWEIGHT, 0, 2, 0.01f);
    grp->createEditFloat(PID_ADHESIONWEIGHT, "adhesion weight", adhesionWeight);
    grp->setMinMaxStep(PID_ADHESIONWEIGHT, 0, 1, 0.01f);
    grp->createEditFloat(PID_BRANCHINGPROBABILITY, "branching probability", branchingProbability);
    grp->setMinMaxStep(PID_BRANCHINGPROBABILITY, 0, 1, 0.01f);
    grp->createEditFloat(PID_MAXFLOATINGLENGTH, "max floating length", maxFloatingLength);
    grp->setMinMaxStep(PID_MAXFLOATINGLENGTH, 0, 2, 0.01f);
    grp->createEditFloat(PID_MAXADHESIONDISTANCE, "max adhesion dist.", maxAdhesionDistance);
    grp->setMinMaxStep(PID_MAXADHESIONDISTANCE, 0, 1, 0.01f);
    grp->createIndent();
    grp->createEditFloat(PID_IVYBRANCHSIZE, "ivy branch size", ivyBranchSize);
    grp->setMinMaxStep(PID_IVYBRANCHSIZE, 0, 0.5, 0.01f);
    grp->createEditFloat(PID_IVYLEAFSIZE, "ivy leaf size", ivyLeafSize);
    grp->setMinMaxStep(PID_IVYLEAFSIZE, 0, 2, 0.01f);
    grp->createEditFloat(PID_LEAFDENSITY, "leaf density", leafDensity);
    grp->setMinMaxStep(PID_LEAFDENSITY, 0, 1, 0.01f);

    grp = panel.createGroup(PID_GROWTH_GRP, "Growth");

    grp->createEditInt(PID_RANDOMSEED, "Random seed:", startSeed);
    grp->createEditInt(PID_GROWITERATIONS, "Grow iterations:", needToGrow);
    grp->setMinMaxStep(PID_GROWITERATIONS, 0, 64000, 1);
    grp->createButton(PID_RESET, "Reset");
    grp->createIndent();
    grp->createButton(PID_AUTOGROW, growing ? "Stop" : "Start grow");

    grp = panel.createGroup(PID_GEOM_GRP, "Geometry");

    PropPanel::ContainerPropertyControl *gtRadio = grp->createRadioGroup(PID_GEN_TYPE, "Geometry type:");
    gtRadio->createRadio(PID_GEN_TYPE + GEOM_TYPE_PRISM, "prism");
    gtRadio->createRadio(PID_GEN_TYPE + GEOM_TYPE_BILLBOARDS, "billboards");
    grp->setInt(PID_GEN_TYPE, PID_GEN_TYPE + genGeomType);

    grp->createIndent();
    grp->createEditFloat(PID_NODES_ANGLE_DELTA, "Optim. epsilon (degr.)", nodesAngleDelta);
    grp->createEditFloat(PID_MAX_TRIANGLES, "Max triangles", maxTriangles, 1);
    grp->createButton(PID_GENERATE_GEOM, "Generate");
    grp->createIndent();
    grp->createStatic(PID_TRIS_INFO, String(50, "Triangles: %d", trisCount));

    grp = panel.createGroup(PID_GROWTH_GRP, "Render");

    grp->createCheckBox(PID_RENDER_GEOM, "Render geometry", needRenderGeom);
    grp->createCheckBox(PID_RENDER_DEBUG, "Render debug frame", needRenderDebug);
  }

  for (int i = 0; i < objects.size(); ++i)
  {
    IvyObject *o = RTTI_cast<IvyObject>(objects[i]);
    if (!o)
      continue;

    if (strcmp(o->assetName, assetName))
      panel.setCaption(PID_GENBLKNAME, "");
    if (o->startSeed != startSeed)
      panel.resetById(PID_RANDOMSEED);
    if (o->growed != growed)
      panel.resetById(PID_GROWITERATIONS);
    if (o->size != size)
      panel.resetById(PID_IVYSIZE);
    if (o->primaryWeight != primaryWeight)
      panel.resetById(PID_PRIMARYWEIGHT);
    if (o->randomWeight != randomWeight)
      panel.resetById(PID_RANDOMWEIGHT);
    if (o->gravityWeight != gravityWeight)
      panel.resetById(PID_GRAVITYWEIGHT);
    if (o->adhesionWeight != adhesionWeight)
      panel.resetById(PID_ADHESIONWEIGHT);
    if (o->branchingProbability != branchingProbability)
      panel.resetById(PID_BRANCHINGPROBABILITY);
    if (o->maxFloatingLength != maxFloatingLength)
      panel.resetById(PID_MAXFLOATINGLENGTH);
    if (o->maxAdhesionDistance != maxAdhesionDistance)
      panel.resetById(PID_MAXADHESIONDISTANCE);
    if (o->ivyBranchSize != ivyBranchSize)
      panel.resetById(PID_IVYBRANCHSIZE);
    if (o->ivyLeafSize != ivyLeafSize)
      panel.resetById(PID_IVYLEAFSIZE);
    if (o->leafDensity != leafDensity)
      panel.resetById(PID_LEAFDENSITY);
    if (o->genGeomType != genGeomType)
      panel.resetById(PID_GEN_TYPE);
    if (o->needRenderGeom != needRenderGeom)
      panel.resetById(PID_RENDER_GEOM);
    // if (o->needDecreaseGrow != needDecreaseGrow)
    //   panel.resetById(PID_DECREASE);
    if (o->needRenderDebug != needRenderDebug)
      panel.resetById(PID_RENDER_DEBUG);
    if (o->maxTriangles != maxTriangles)
      panel.resetById(PID_MAX_TRIANGLES);
    if (o->nodesAngleDelta != nodesAngleDelta)
      panel.resetById(PID_NODES_ANGLE_DELTA);
  }
}

void IvyObject::onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
  dag::ConstSpan<RenderableEditableObject *> objects)
{
#define CHANGE_VAL(type, pname, getfunc)               \
  {                                                    \
    type val = panel.getfunc(pid);                     \
    for (int i = 0; i < objects.size(); ++i)           \
    {                                                  \
      IvyObject *o = RTTI_cast<IvyObject>(objects[i]); \
      if (!o || o->pname == val)                       \
        continue;                                      \
      o->pname = val;                                  \
      if (!o->canGrow)                                 \
      {                                                \
        o->canGrow = true;                             \
        o->seed();                                     \
      }                                                \
      DAGORED2->repaint();                             \
    }                                                  \
  }

#define CHANGE_VAL_FUNC(type, pname, getfunc, objfunc) \
  {                                                    \
    type val = panel.getfunc(pid);                     \
    for (int i = 0; i < objects.size(); ++i)           \
    {                                                  \
      IvyObject *o = RTTI_cast<IvyObject>(objects[i]); \
      if (!o || o->pname == val)                       \
        continue;                                      \
      o->pname = val;                                  \
      if (!o->canGrow)                                 \
      {                                                \
        o->canGrow = true;                             \
        o->seed();                                     \
      }                                                \
      else                                             \
        o->objfunc();                                  \
      DAGORED2->repaint();                             \
    }                                                  \
  }

  if (pid == PID_IVYSIZE)
    CHANGE_VAL_FUNC(float, size, getFloat, reGrow)
  else if (pid == PID_PRIMARYWEIGHT)
    CHANGE_VAL_FUNC(float, primaryWeight, getFloat, reGrow)
  else if (pid == PID_RANDOMWEIGHT)
    CHANGE_VAL_FUNC(float, randomWeight, getFloat, reGrow)
  else if (pid == PID_GRAVITYWEIGHT)
    CHANGE_VAL_FUNC(float, gravityWeight, getFloat, reGrow)
  else if (pid == PID_ADHESIONWEIGHT)
    CHANGE_VAL_FUNC(float, adhesionWeight, getFloat, reGrow)
  else if (pid == PID_BRANCHINGPROBABILITY)
    CHANGE_VAL_FUNC(float, branchingProbability, getFloat, reGrow)
  else if (pid == PID_MAXFLOATINGLENGTH)
    CHANGE_VAL_FUNC(float, maxFloatingLength, getFloat, reGrow)
  else if (pid == PID_MAXADHESIONDISTANCE)
    CHANGE_VAL_FUNC(float, maxAdhesionDistance, getFloat, reGrow)
  else if (pid == PID_IVYBRANCHSIZE)
    CHANGE_VAL(float, ivyBranchSize, getFloat)
  else if (pid == PID_IVYLEAFSIZE)
    CHANGE_VAL(float, ivyLeafSize, getFloat)
  else if (pid == PID_LEAFDENSITY)
    CHANGE_VAL(float, leafDensity, getFloat)
  else if (pid == PID_NODES_ANGLE_DELTA)
    CHANGE_VAL(float, nodesAngleDelta, getFloat)
  else if (pid == PID_MAX_TRIANGLES)
    CHANGE_VAL(float, maxTriangles, getFloat)
  else if (pid == PID_RENDER_GEOM)
    CHANGE_VAL(bool, needRenderGeom, getBool)
  else if (pid == PID_RENDER_DEBUG)
    CHANGE_VAL(bool, needRenderDebug, getBool)
  // else if (pid == PID_DECREASE)
  // CHANGE_VAL(bool, needDecreaseGrow, getCheck)
  else if (pid == PID_GEN_TYPE && panel.getInt(pid) != PropPanel::RADIO_SELECT_NONE)
  {
    int type = panel.getInt(pid) - PID_GEN_TYPE;
    for (int i = 0; i < objects.size(); i++)
    {
      IvyObject *o = RTTI_cast<IvyObject>(objects[i]);
      if (o && o->genGeomType != type)
      {
        o->genGeomType = type;
        if (!o->canGrow)
        {
          o->canGrow = true;
          o->seed();
        }
        DAGORED2->repaint();
      }
    }
  }
  else if (pid == PID_RANDOMSEED)
  {
    for (int i = 0; i < objects.size(); i++)
    {
      IvyObject *o = RTTI_cast<IvyObject>(objects[i]);
      if (o)
      {
        o->currentSeed = o->startSeed = panel.getInt(pid);
        o->reGrow();
      }
    }
  }
  else if (pid == PID_GROWITERATIONS)
  {
    for (int i = 0; i < objects.size(); i++)
    {
      IvyObject *o = RTTI_cast<IvyObject>(objects[i]);
      if (o)
      {
        if (!o->canGrow)
        {
          o->canGrow = true;
          o->seed();
        }

        int ntg = panel.getInt(pid);
        if (ntg < o->growed)
        {
          o->clearIvy();
          o->seed();
          o->needToGrow = ntg;
        }
        else
          needToGrow = ntg;
      }
    }
  }
#undef CHANGE_VAL
#undef CHANGE_VAL_FUNC
}


void IvyObject::onPPBtnPressed(int pid, PropPanel::ContainerPropertyControl &panel, dag::ConstSpan<RenderableEditableObject *> objects)
{
  switch (pid)
  {
    case PID_RESET_TO_ASSET:
    {
      for (int i = 0; i < objects.size(); i++)
      {
        IvyObject *o = RTTI_cast<IvyObject>(objects[i]);
        if (!o)
          continue;

        DagorAsset *a = DAEDITOR3.getAssetByName(assetName, DAEDITOR3.getAssetTypeId("ivyClass"));
        if (a)
        {
          o->loadIvy(a->props);
          o->reGrow();
        }
      }

      getObjEditor()->invalidateObjectProps();
      DAGORED2->repaint();
    }
    break;

    case PID_AUTOGROW:
    {
      if (!growing)
      {
        panel.setCaption(pid, "Stop");
        for (int i = 0; i < objects.size(); i++)
        {
          IvyObject *o = RTTI_cast<IvyObject>(objects[i]);
          if (o)
          {
            if (!o->canGrow)
            {
              o->canGrow = true;
              o->seed();
            }

            if (o->needDecreaseGrow)
              o->growing = -1;
            else
              o->growing = 1;
          }
        }
      }
      else
      {
        panel.setCaption(pid, "Start grow");
        for (int i = 0; i < objects.size(); i++)
        {
          IvyObject *o = RTTI_cast<IvyObject>(objects[i]);
          if (o)
            o->growing = 0;
        }
        getObjEditor()->invalidateObjectProps();
      }
    }
    break;

    case PID_RESET:
    {
      for (int i = 0; i < objects.size(); i++)
      {
        IvyObject *o = RTTI_cast<IvyObject>(objects[i]);
        if (o)
        {
          o->clearIvy();
          o->seed();
        }
      }
      getObjEditor()->invalidateObjectProps();
      DAGORED2->repaint();
    }
    break;

    case PID_GENERATE_GEOM:
    {
      for (int i = 0; i < objects.size(); i++)
      {
        IvyObject *o = RTTI_cast<IvyObject>(objects[i]);
        if (o)
          o->generateGeom(DAGORED2->getConsole());
      }
      getObjEditor()->invalidateObjectProps();
      DAGORED2->repaint();
    }
    break;

    case PID_GENBLKNAME:
    {
      const char *asset_name = DAEDITOR3.selectAssetX(assetName, "Select ivy asset", "ivyClass");

      for (int i = 0; i < objects.size(); i++)
      {
        IvyObject *o = RTTI_cast<IvyObject>(objects[i]);
        if (!o)
          continue;

        o->assetName = asset_name;
      }

      panel.setCaption(pid, asset_name);
      getObjEditor()->invalidateObjectProps();
      DAGORED2->repaint();
    }
    break;
  }
}


void IvyObject::setWtm(const TMatrix &wtm)
{
  RenderableEditableObject::setWtm(wtm);
  pt = getPos();
}


void IvyObject::moveObject(const Point3 &delta, IEditorCoreEngine::BasisType basis)
{
  RenderableEditableObject::moveObject(delta, basis);
  objectWasMoved = true;
}


void IvyObject::putMoveUndo()
{
  IvyObjectEditor *ed = (IvyObjectEditor *)getObjEditor();
  if (ed->isCloneMode())
    return;

  class ReGrowUndo : public UndoMove
  {
  public:
    ReGrowUndo(IvyObject *o) : UndoMove(o) {}

    void restore(bool save_redo) override
    {
      if (save_redo)
        redoPos = obj->getPos();
      obj->setPos(oldPos);

      IvyObject *v = RTTI_cast<IvyObject>(obj);
      if (v)
        v->reGrow();
    }
    void redo() override
    {
      obj->setPos(redoPos);

      IvyObject *v = RTTI_cast<IvyObject>(obj);
      if (v)
        v->reGrow();
    }
  };

  getObjEditor()->getUndoSystem()->put(new ReGrowUndo(this));
}


void IvyObject::defaults()
{
  size = 0.005;
  primaryWeight = 0.5;
  randomWeight = 0.2;
  gravityWeight = 1;
  adhesionWeight = 0.1;
  branchingProbability = 0.95;
  maxFloatingLength = 0.2;
  maxAdhesionDistance = 0.1;
  ivyBranchSize = 0.15;
  ivyLeafSize = 1.5;
  leafDensity = 0.7;

  currentSeed = startSeed = 1;
}


void IvyObject::clipMax()
{
#define IVY_CLIP(x, y) \
  if (x > y)           \
  x = y
  IVY_CLIP(size, 100);
  IVY_CLIP(primaryWeight, 1);
  IVY_CLIP(randomWeight, 1);
  IVY_CLIP(gravityWeight, 2);
  IVY_CLIP(adhesionWeight, 1);
  IVY_CLIP(branchingProbability, 1);
  IVY_CLIP(maxFloatingLength, 2);
  IVY_CLIP(maxAdhesionDistance, 1);
  IVY_CLIP(ivyBranchSize, 0.5);
  IVY_CLIP(ivyLeafSize, 2);
  IVY_CLIP(leafDensity, 1);
#undef IVY_CLIP
}


void IvyObject::loadIvy(const DataBlock &blk)
{
#define IVY_LOAD(x) x = blk.getReal(#x, x)
  IVY_LOAD(size);
  IVY_LOAD(primaryWeight);
  IVY_LOAD(randomWeight);
  IVY_LOAD(gravityWeight);
  IVY_LOAD(adhesionWeight);
  IVY_LOAD(branchingProbability);
  IVY_LOAD(maxFloatingLength);
  IVY_LOAD(maxAdhesionDistance);
  IVY_LOAD(ivyBranchSize);
  IVY_LOAD(ivyLeafSize);
  IVY_LOAD(leafDensity);

  IVY_LOAD(nodesAngleDelta);
#undef IVY_LOAD
}

void IvyObject::saveIvy(DataBlock &blk)
{
#define IVY_SAVE(x) blk.setReal(#x, x)
  IVY_SAVE(size);
  IVY_SAVE(primaryWeight);
  IVY_SAVE(randomWeight);
  IVY_SAVE(gravityWeight);
  IVY_SAVE(adhesionWeight);
  IVY_SAVE(branchingProbability);
  IVY_SAVE(maxFloatingLength);
  IVY_SAVE(maxAdhesionDistance);
  IVY_SAVE(ivyBranchSize);
  IVY_SAVE(ivyLeafSize);
  IVY_SAVE(leafDensity);

  IVY_SAVE(nodesAngleDelta);
#undef IVY_SAVE
}


void IvyObject::load(const DataBlock &blk)
{
  clearIvy();

  assetName = blk.getStr("genBlkName", NULL);
  DagorAsset *a = DAEDITOR3.getAssetByName(assetName, DAEDITOR3.getAssetTypeId("ivyClass"));
  if (a)
    loadIvy(a->props);

  pt = blk.getPoint3("pos", pt);
  startSeed = blk.getInt("startSeed", startSeed);
  needToGrow = blk.getInt("growed", growed);
  genGeomType = blk.getInt("genGeomType", genGeomType);
  maxTriangles = blk.getInt("maxTriangles", maxTriangles);

  loadIvy(blk);

  setPos(pt);
}


void IvyObject::save(DataBlock &blk)
{
  DataBlock &cb = *blk.addNewBlock("ivy");

  cb.setInt("uid", uid);
  cb.setStr("genBlkName", assetName);
  cb.setPoint3("pos", pt);

  cb.setInt("startSeed", startSeed);
  cb.setInt("growed", needToGrow);
  cb.setInt("genGeomType", genGeomType);
  cb.setReal("maxTriangles", maxTriangles);

  saveIvy(cb);

  String basePath;
  DAGORED2->getPluginProjectPath(IvyGenPlugin::self, basePath);
  dagGeom->geomObjectSaveToDag(*ivyGeom, String(260, "%s/%s.ivy.dag", (char *)basePath, getName()));
}

void IvyObject::update(float dt)
{
  if (firstAct)
  {
    firstAct = false;

    String basePath;
    DAGORED2->getPluginProjectPath(IvyGenPlugin::self, basePath);
    String fname(260, "%s/%s.ivy.dag", (char *)basePath, getName());

    if (!::dd_file_exist(fname))
      return;

    CoolConsole &con = DAGORED2->getConsole();
    con.startProgress();
    con.setActionDesc(String(512, "recompile %s nodes...", getName()));
    con.setTotal(3);


    dagGeom->geomObjectLoadFromDag(*ivyGeom, fname, &con);
    con.incDone();
    recalcLighting();
    con.incDone();
    ivyGeom->notChangeVertexColors(true);
    dagGeom->geomObjectRecompile(*ivyGeom);
    ivyGeom->notChangeVertexColors(false);
    con.incDone();
    con.endProgress();
  }

  if (growing)
    needToGrow++;
}

void IvyObject::decreaseGrow()
{
  if (growed <= 1)
  {
    growing = 1;
    getObjEditor()->invalidateObjectProps();
    return;
  }

  for (int i = 0; i < roots.size(); i++)
  {
    if (roots[i]->iteration == growed)
      erase_items(roots, i, 1);
    else
    {
      if (roots[i]->aliveIteration == growed)
        roots[i]->alive = true;

      for (int j = 0; j < roots[i]->nodes.size(); j++)
        if (roots[i]->nodes[j]->iteration == growed)
          erase_items(roots[i]->nodes, j, 1);
    }
  }

  growed--;
}

void IvyObject::startGrow()
{
  currentSeed = startSeed;

  primaryWeight_ = primaryWeight;
  randomWeight_ = randomWeight;
  adhesionWeight_ = adhesionWeight;

  growed = 0;
}


void IvyObject::clearIvy()
{
  for (int i = 0; i < roots.size(); i++)
    clear_all_ptr_items(roots[i]->nodes);

  clear_all_ptr_items(roots);
  growed = 0;
  needToGrow = 0;
}


void IvyObject::reGrow()
{
  if (!canGrow)
  {
    canGrow = true;
    seed();
  }
  else
  {
    int oldGrowed = growed;

    clearIvy();
    seed();

    needToGrow = oldGrowed;
  }
}


void IvyObject::seed()
{
  IvyNode *tmpNode = new IvyNode;
  tmpNode->pos = pt;
  tmpNode->primaryDir = Point3(0.0f, 1.0f, 0.0f);
  tmpNode->adhesionVector = Point3(0.0f, 0.0f, 0.0f);
  tmpNode->length = 0.0f;
  tmpNode->floatingLength = 0.0f;
  tmpNode->climb = true;

  IvyRoot *tmpRoot = new IvyRoot;

  tmpRoot->nodes.push_back(tmpNode);
  tmpRoot->alive = true;
  tmpRoot->parents = 0;

  roots.push_back(tmpRoot);
  startGrow();
}


void IvyObject::grow()
{
  if (!roots.size())
    return;

  growed++;
  if (growed >= 64000)
    return;

  // parameters that depend on the scene object bounding sphere
  // float local_ivySize = Common::mesh.boundingSphereRadius * ivySize;
  // float local_maxFloatLength = Common::mesh.boundingSphereRadius * maxFloatLength;
  float local_ivySize = size;

  float local_maxFloatLength = size * maxFloatingLength * 100.0;


  // normalize weights of influence
  float sum = primaryWeight_ + randomWeight_ + adhesionWeight_;

  primaryWeight_ /= sum;
  randomWeight_ /= sum;
  adhesionWeight_ /= sum;


  // lets grow
  for (int i = 0; i < roots.size(); i++)
  {
    // process only roots that are alive
    if (!roots[i]->alive)
      continue;

    // let the ivy die, if the maximum float length is reached
    if (roots[i]->nodes[roots[i]->nodes.size() - 1]->floatingLength > local_maxFloatLength)
      roots[i]->alive = false;


    // grow vectors: primary direction, random influence, and adhesion of scene objectss

    // primary vector = weighted sum of previous grow vectors
    Point3 primaryVector = roots[i]->nodes[roots[i]->nodes.size() - 1]->primaryDir;

    // random influence plus a little upright vector
    Point3 randomVector = normalize(getRandomized() + Point3(0.0f, 0.2f, 0.0f));

    // adhesion influence to the nearest triangle = weighted sum of previous adhesion vectors
    Point3 faceNorm;
    Point3 adhesionVector = computeAdhesion(roots[i]->nodes[roots[i]->nodes.size() - 1]->pos, ivyCb, faceNorm);

    // compute grow vector
    Point3 growVector =
      local_ivySize * (primaryVector * primaryWeight + randomVector * randomWeight + adhesionVector * adhesionWeight);


    // gravity influence

    // compute gravity vector
    Point3 gravityVector = local_ivySize * Point3(0.0f, -1.0f, 0.0f) * gravityWeight;

    // gravity depends on the floating length
    gravityVector *= powf(roots[i]->nodes[roots[i]->nodes.size() - 1]->floatingLength / local_maxFloatLength, 0.7f);


    // next possible ivy node

    // climbing state of that ivy node, will be set during collision detection
    bool climbing;

    // compute position of next ivy node
    Point3 newPos = roots[i]->nodes[roots[i]->nodes.size() - 1]->pos + growVector + gravityVector;

    if (!computeCollision(roots[i]->nodes[roots[i]->nodes.size() - 1]->pos, newPos, climbing, ivyCb))
    {
      roots[i]->alive = false;
      roots[i]->aliveIteration = growed;
    }

    // update grow vector due to a changed newPos
    growVector = newPos - roots[i]->nodes[roots[i]->nodes.size() - 1]->pos - gravityVector;


    // create next ivy node
    IvyNode *tmpNode = new IvyNode;

    tmpNode->faceNormal = faceNorm;

    tmpNode->iteration = growed;

    tmpNode->pos = newPos;

    tmpNode->primaryDir = normalize(0.5f * roots[i]->nodes[roots[i]->nodes.size() - 1]->primaryDir + 0.5f * normalize(growVector));

    tmpNode->adhesionVector = adhesionVector;

    tmpNode->length =
      roots[i]->nodes[roots[i]->nodes.size() - 1]->length + length(newPos - roots[i]->nodes[roots[i]->nodes.size() - 1]->pos);

    tmpNode->floatingLength = climbing ? 0.0f
                                       : roots[i]->nodes[roots[i]->nodes.size() - 1]->floatingLength +
                                           length(newPos - roots[i]->nodes[roots[i]->nodes.size() - 1]->pos);

    tmpNode->climb = climbing;

    roots[i]->nodes.push_back(tmpNode);
  }


  // lets produce child ivys
  for (int i = 0; i < roots.size(); i++)
  {
    // process only roots that are alive
    if (!roots[i]->alive)
      continue;

    // process only roots up to hierarchy level 3, results in a maximum hierarchy level of 4
    if (roots[i]->parents > 3)
      continue;


    // add child ivys on existing ivy nodes
    for (int j = 0; j < roots[i]->nodes.size(); j++)
    {
      // weight depending on ratio of node length to total length
      float weight =
        1.0f - (cos(roots[i]->nodes[j]->length / roots[i]->nodes[roots[i]->nodes.size() - 1]->length * 2.0f * PI) * 0.5f + 0.5f);

      // random influence
      float probability = frnd(currentSeed);

      if (probability * weight > branchingProbability)
      {
        // new ivy node
        IvyNode *tmpNode = new IvyNode;

        tmpNode->iteration = growed;

        tmpNode->pos = roots[i]->nodes[j]->pos;

        tmpNode->primaryDir = Point3(0.0f, 1.0f, 0.0f);

        tmpNode->adhesionVector = Point3(0.0f, 0.0f, 0.0f);

        tmpNode->length = 0.0f;

        tmpNode->floatingLength = roots[i]->nodes[j]->floatingLength;

        tmpNode->climb = true;


        // new ivy root
        IvyRoot *tmpRoot = new IvyRoot;

        tmpRoot->nodes.push_back(tmpNode);

        tmpRoot->alive = true;
        tmpRoot->iteration = growed;

        tmpRoot->parents = roots[i]->parents + 1;

        roots.push_back(tmpRoot);

        return;
      }
    }
  }
}

bool IvyObject::computeCollision(const Point3 &oldPos, Point3 &newPos, bool &climbing, CollisionCallback &cb)
{
  climbing = false;

  bool intersection;

  int deadlockCounter = 0;

  do
  {
    intersection = false;
    float t0 = 1;
    Point3 normal;
    if (cb.traceRay(oldPos, newPos - oldPos, t0, normal))
    {
      Point3 p0 = oldPos + (newPos - oldPos) * t0;
      newPos -= normal * (1.1 * normal * (newPos - p0));
      intersection = true;

      climbing = true;
    }

    if (deadlockCounter++ > 5)
    {
      return false;
    }
  } while (intersection);

  return true;
}

Point3 IvyObject::computeAdhesion(const Point3 &pos, CollisionCallback &cb, Point3 &norm)
{
  float local_maxAdhesionDistance = maxAdhesionDistance;
  return cb.computeAdhesion(pos, local_maxAdhesionDistance, norm);
}


void IvyObject::debugRender()
{
  Tab<Point3> branch(tmpmem);
  dagRender->startLinesRender();
  dagRender->setLinesTm(TMatrix::IDENT);
  for (int i = 0; i < roots.size(); i++)
  {
    branch.clear();
    branch.reserve(roots[i]->nodes.size());
    for (int j = 0; j < roots[i]->nodes.size(); j++)
      branch.push_back(roots[i]->nodes[j]->pos);
    dagRender->renderLine(branch.data(), branch.size(), E3DCOLOR(150, 220, 50));
  }

  dagRender->endLinesRender();
}

void IvyObject::gatherStaticGeometry(StaticGeometryContainer &cont, int flags)
{
  const StaticGeometryContainer &geom = *ivyGeom->getGeometryContainer();
  for (int ni = 0; ni < geom.nodes.size(); ++ni)
  {
    StaticGeometryNode *node = geom.nodes[ni];

    if (node && (!flags || (node->flags & flags)))
    {
      StaticGeometryNode *n = dagGeom->newStaticGeometryNode(*node, tmpmem);
      n->name = (const char *)String(512, "%s_ivyNode_%s#%d", (char *)getName(), (char *)n->name, ni);
      cont.addNode(n);
    }
  }
}

IvyObject *IvyObject::clone()
{
  IvyObjectEditor *ed = (IvyObjectEditor *)getObjEditor();
  if (!ed)
    return NULL;

  IvyObject *ret = new IvyObject(ed->getNextObjId());
  ret->clearIvy();
  ret->assetName = assetName;

  DagorAsset *a = DAEDITOR3.getAssetByName(assetName, DAEDITOR3.getAssetTypeId("ivyClass"));
  if (a)
    ret->loadIvy(a->props);

  ret->pt = pt;
  ret->setPos(pt);
  ret->startSeed = startSeed;
  ret->needToGrow = needToGrow;
  ret->growed = growed;
  ret->genGeomType = genGeomType;
  ret->needRenderDebug = needRenderDebug;
  ret->needRenderGeom = needRenderGeom;
  ret->canGrow = canGrow;

  ret->size = size;
  ret->primaryWeight = primaryWeight;
  ret->randomWeight = randomWeight;
  ret->gravityWeight = gravityWeight;
  ret->adhesionWeight = adhesionWeight;
  ret->branchingProbability = branchingProbability;
  ret->maxFloatingLength = maxFloatingLength;
  ret->maxAdhesionDistance = maxAdhesionDistance;
  ret->ivyBranchSize = ivyBranchSize;
  ret->ivyLeafSize = ivyLeafSize;
  ret->leafDensity = leafDensity;

  ret->roots.resize(roots.size());
  for (int i = 0; i < roots.size(); i++)
  {
    ret->roots[i] = new IvyRoot;
    ret->roots[i]->alive = roots[i]->alive;
    ret->roots[i]->parents = roots[i]->parents;
    ret->roots[i]->iteration = roots[i]->iteration;
    ret->roots[i]->aliveIteration = roots[i]->aliveIteration;

    ret->roots[i]->nodes.resize(roots[i]->nodes.size());
    for (int j = 0; j < roots[i]->nodes.size(); j++)
    {
      ret->roots[i]->nodes[j] = new IvyNode;
      ret->roots[i]->nodes[j]->iteration = roots[i]->nodes[j]->iteration;
      ret->roots[i]->nodes[j]->climb = roots[i]->nodes[j]->climb;
      ret->roots[i]->nodes[j]->pos = roots[i]->nodes[j]->pos;
      ret->roots[i]->nodes[j]->primaryDir = roots[i]->nodes[j]->primaryDir;
      ret->roots[i]->nodes[j]->adhesionVector = roots[i]->nodes[j]->adhesionVector;
      ret->roots[i]->nodes[j]->smoothAdhesionVector = roots[i]->nodes[j]->smoothAdhesionVector;
      ret->roots[i]->nodes[j]->length = roots[i]->nodes[j]->length;
      ret->roots[i]->nodes[j]->floatingLength = roots[i]->nodes[j]->floatingLength;
    }
  }

  return ret;
}
