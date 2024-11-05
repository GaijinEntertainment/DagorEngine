// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "plugIn.h"
#include "ivyObject.h"

#include "ivyObjectsEditor.h"

#include <EditorCore/ec_IEditorCore.h>
#include <libTools/staticGeom/geomObject.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <winGuiWrapper/wgw_input.h>
#include <shaders/dag_overrideStates.h>

using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;

#define OBJECT_IVY     "Ivy"
#define MAX_TRACE_DIST 1000.0

enum
{
  CM_ = CM_PLUGIN_BASE + 1,
  CM_CREATE_IVY,
  CM_REBUILD,
  CM_RECREATE_GEOM,
  CM_CLEAR_GEOM
};


IvyObjectEditor::IvyObjectEditor() : inGizmo(false), cloneMode(false)
{
  IvyObject::initStatics();
  ptDynBuf = dagRender->newDynRenderBuffer("editor_rhv_tex", midmem);
  nextPtUid = 0;
  secTime = 0;
  curPt = new IvyObject(0);
  curPt->visible = false;

  shaders::OverrideState zFuncLessState;
  zFuncLessState.set(shaders::OverrideState::Z_FUNC);
  zFuncLessState.zFunc = CMPF_LESS;
  zFuncLessStateId = dagGeom->create(zFuncLessState);
}


IvyObjectEditor::~IvyObjectEditor()
{
  dagGeom->destroy(zFuncLessStateId);
  dagRender->deleteDynRenderBuffer(ptDynBuf);
}


void IvyObjectEditor::fillToolBar(PropPanel::ContainerPropertyControl *toolbar)
{
  ObjectEditor::fillToolBar(toolbar);

  PropPanel::ContainerPropertyControl *tb = toolbar->createToolbarPanel(0, "");

  tb->createSeparator();
  addButton(tb, CM_CREATE_IVY, "create_complex", "Create ivy", true);
  addButton(tb, CM_REBUILD, "compile", "Re-grow selected Ivy objects");
  tb->createSeparator();
  addButton(tb, CM_RECREATE_GEOM, "compile", "Generate selected Ivys geometry");
  tb->createSeparator();
  addButton(tb, CM_CLEAR_GEOM, "clear", "Clear selected Ivys geometry");
}


void IvyObjectEditor::updateToolbarButtons()
{
  ObjectEditor::updateToolbarButtons();
  setRadioButton(CM_CREATE_IVY, getEditMode());
}


void IvyObjectEditor::beforeRender()
{
  ObjectEditor::beforeRender();

  for (int i = 0; i < objects.size(); i++)
  {
    IvyObject *p = RTTI_cast<IvyObject>(objects[i]);
    if (p)
      p->beforeRender();
  }
}


void IvyObjectEditor::render()
{
  bool curPlugin = DAGORED2->curPlugin() == IvyGenPlugin::self;

  for (int i = 0; i < objects.size(); i++)
  {
    IvyObject *p = RTTI_cast<IvyObject>(objects[i]);
    if (p)
      p->render();
  }

  if (curPlugin)
  {
    TMatrix4 gtm;
    Point2 scale;
    int w, h;

    d3d::get_target_size(w, h);
    d3d::getglobtm(gtm);
    scale.x = 2.0 / w;
    scale.y = 2.0 / h;

    dagRender->dynRenderBufferClearBuf(*ptDynBuf);

    int cnt = 0;

    for (int i = 0; i < objects.size(); i++)
    {
      IvyObject *p = RTTI_cast<IvyObject>(objects[i]);
      if (p)
      {
        p->renderPts(*ptDynBuf, gtm, scale);

        cnt++;
        if (cnt >= 512)
        {
          dagGeom->set(zFuncLessStateId);
          dagGeom->shaderGlobalSetInt(IvyObject::ptRenderPassId, 0);
          dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, IvyObject::texPt);
          dagGeom->reset_override();

          dagGeom->shaderGlobalSetInt(IvyObject::ptRenderPassId, 1);
          dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, IvyObject::texPt);
          dagRender->dynRenderBufferClearBuf(*ptDynBuf);
          cnt = 0;
        }
      }
    }

    if (curPt && curPt->visible)
      IvyObject::renderPoint(*ptDynBuf, toPoint4(curPt->getPt(), 1) * gtm, scale * IvyObject::ptScreenRad, E3DCOLOR(200, 200, 10));

    dagGeom->set(zFuncLessStateId);
    dagGeom->shaderGlobalSetInt(IvyObject::ptRenderPassId, 0);
    dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, IvyObject::texPt);
    dagGeom->reset_override();

    dagGeom->shaderGlobalSetInt(IvyObject::ptRenderPassId, 1);
    dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, IvyObject::texPt);
    dagRender->dynRenderBufferFlush(*ptDynBuf);
  }
}


void IvyObjectEditor::renderTrans()
{
  bool curPlugin = DAGORED2->curPlugin() == IvyGenPlugin::self;
  if (!curPlugin)
    return;

  for (int i = 0; i < objects.size(); i++)
  {
    IvyObject *p = RTTI_cast<IvyObject>(objects[i]);
    if (p)
      p->renderTrans();
  }
}


void IvyObjectEditor::gizmoStarted()
{
  ObjectEditor::gizmoStarted();
  inGizmo = false;
  getAxes(locAx[0], locAx[1], locAx[2]);
  inGizmo = true;

  for (int i = 0; i < selection.size(); ++i)
    if (!RTTI_cast<IvyObject>(selection[i]))
      return;

  if (wingw::is_key_pressed(wingw::V_SHIFT) && getEditMode() == CM_OBJED_MODE_MOVE)
  {
    if (!cloneMode)
      cloneMode = true;
  }

  if (cloneMode)
  {
    PtrTab<RenderableEditableObject> oldSel(selection);

    unselectAll();

    for (int i = 0; i < oldSel.size(); ++i)
    {
      IvyObject *p = RTTI_cast<IvyObject>(oldSel[i]);
      if (!p)
        continue;

      IvyObject *np = p->clone();
      addObject(np);

      np->selectObject();
    }
  }
}


void IvyObjectEditor::gizmoEnded(bool apply)
{
  if (cloneMode && selection.size())
  {
    getUndoSystem()->accept("Clone object(s)");

    cloneMode = false;

    gizmoRotO = gizmoRot;
    gizmoSclO = gizmoScl;
    isGizmoStarted = false;
  }
  else
    ObjectEditor::gizmoEnded(apply);
  inGizmo = false;
}


void IvyObjectEditor::save(DataBlock &blk)
{
  for (int i = 0; i < objects.size(); i++)
  {
    IvyObject *p = RTTI_cast<IvyObject>(objects[i]);
    if (p)
      p->save(blk);
  }
}


void IvyObjectEditor::load(const DataBlock &blk)
{
  int nid = blk.getNameId("ivy");

  for (int i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock &cb = *blk.getBlock(i);
    if (cb.getBlockNameId() != nid)
      continue;

    int id = cb.getInt("uid", -1);
    Ptr<IvyObject> s = new IvyObject(id);
    addObject(s, false);
    s->load(cb);
    nextPtUid++;
    if (nextPtUid <= id)
      nextPtUid = id + 1;
  }
}


void IvyObjectEditor::getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types)
{
  names.clear();

  if (types.empty())
    return;

  bool showIvys = false;

  for (int ti = 0; ti < types.size(); ++ti)
  {
    if (types[ti] == 0)
      showIvys = true;
  }

  for (int i = 0; i < objects.size(); i++)
  {
    IvyObject *p = RTTI_cast<IvyObject>(objects[i]);
    if (p && showIvys)
    {
      names.push_back() = p->getName();

      if (p->isSelected())
        sel_names.push_back() = p->getName();
    }
  }
}


void IvyObjectEditor::getTypeNames(Tab<String> &names)
{
  names.resize(1);
  names[0] = OBJECT_IVY;
}


void IvyObjectEditor::onSelectedNames(const Tab<String> &names)
{
  getUndoSystem()->begin();
  unselectAll();

  for (int ni = 0; ni < names.size(); ++ni)
  {
    RenderableEditableObject *o = getObjectByName(names[ni]);
    o->selectObject();
  }

  getUndoSystem()->accept("Select");
  updateGizmo();
}


void IvyObjectEditor::update(real dt)
{
  __int64 tm = ref_time_ticks();
  while (1)
  {
    bool needRepaint = false, needBreak = false;

    for (int i = 0; i < objects.size(); i++)
    {
      IvyObject *p = RTTI_cast<IvyObject>(objects[i]);
      if (p)
      {
        if (!p->canGrow)
          continue;

        if (p->growed < p->needToGrow)
        {
          needRepaint = true;
          p->grow();
          if (p->growed >= p->needToGrow)
          {
            p->growed = p->needToGrow;
            continue;
          }
        }
      }

      if (get_time_usec(tm) > 5000)
      {
        needBreak = true;
        break;
      }
    }

    if (needRepaint)
      DAGORED2->repaint();
    else
      break;

    if (needBreak)
      break;
  }

  ObjectEditor::update(dt);
}


void IvyObjectEditor::setEditMode(int cm)
{
  ObjectEditor::setEditMode(cm);
  DAGORED2->repaint();
}


bool IvyObjectEditor::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (inside && wnd->isActive())
  {
    if (curPt->visible)
    {
      Point3 pos;
      findTargetPos(wnd, x, y, pos);
      curPt->setPos(pos);
    }

    updateGizmo();
    wnd->invalidateCache();
  }
  return ObjectEditor::handleMouseMove(wnd, x, y, inside, buttons, key_modif);
}


bool IvyObjectEditor::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (inside && wnd->isActive())
  {
    if (curPt->visible)
    {
      getUndoSystem()->begin();

      Point3 pos;
      findTargetPos(wnd, x, y, pos);

      Ptr<IvyObject> iv = new IvyObject(nextPtUid++);

      addObject(iv);
      iv->setPos(pos);
      iv->seed();

      curPt->setPos(Point3(0, 0, 0));
      curPt->visible = false;

      getUndoSystem()->accept("Create Ivy object");

      setEditMode(CM_OBJED_MODE_SELECT);
    }

    updateGizmo();
    wnd->invalidateCache();
  }
  return ObjectEditor::handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
}


bool IvyObjectEditor::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (inside && wnd->isActive())
  {
    if (IvyObject::objectWasMoved)
    {
      for (int i = 0; i < selection.size(); i++)
      {
        IvyObject *v = RTTI_cast<IvyObject>(selection[i]);
        if (v)
          v->reGrow();
      }

      IvyObject::objectWasMoved = false;
      DAGORED2->repaint();
    }

    updateGizmo();
    wnd->invalidateCache();
  }
  return ObjectEditor::handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif);
}


bool IvyObjectEditor::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (inside && wnd->isActive())
  {
    if (curPt->visible)
    {
      setEditMode(CM_OBJED_MODE_SELECT);
      curPt->visible = false;
      return true;
    }

    updateGizmo();
    wnd->invalidateCache();
  }
  return ObjectEditor::handleMouseRBPress(wnd, x, y, inside, buttons, key_modif);
}


void IvyObjectEditor::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case CM_OBJED_MODE_SELECT:
    case CM_OBJED_MODE_MOVE:
    case CM_OBJED_MODE_SURF_MOVE:
    case CM_OBJED_MODE_ROTATE:
    case CM_OBJED_MODE_SCALE:
      setEditMode(pcb_id);
      updateGizmo();
      break;

    case CM_CREATE_IVY:
      setEditMode(pcb_id);
      curPt->visible = true;
      break;

    case CM_REBUILD:
      for (int i = 0; i < selection.size(); i++)
      {
        IvyObject *p = RTTI_cast<IvyObject>(selection[i]);
        if (p)
          p->reGrow();
      }
      break;

    case CM_RECREATE_GEOM:
      for (int i = 0; i < selection.size(); i++)
      {
        IvyObject *p = RTTI_cast<IvyObject>(selection[i]);
        if (p)
          p->generateGeom(DAGORED2->getConsole());
      }
      break;

    case CM_CLEAR_GEOM:
      for (int i = 0; i < selection.size(); i++)
      {
        IvyObject *p = RTTI_cast<IvyObject>(selection[i]);
        if (p)
          dagGeom->geomObjectClear(*p->ivyGeom);
      }
      break;

    case CM_OBJED_DROP:
      setEditMode(pcb_id);

      getUndoSystem()->begin();

      const Point3 dir = Point3(0, -1, 0);

      for (int i = 0; i < selection.size(); i++)
      {
        IvyObject *p = RTTI_cast<IvyObject>(selection[i]);
        if (p)
        {
          const Point3 pos = p->getPos() + Point3(0, 0.2, 0);

          real dist = MAX_TRACE_DIST;
          if (IEditorCoreEngine::get()->traceRay(pos, dir, dist))
          {
            p->putMoveUndo();
            p->setPos(pos + dir * dist);
          }

          p->reGrow();
        }
      }

      getUndoSystem()->accept("Drop object(s)");

      updateGizmo();
      DAGORED2->repaint();
      updateToolbarButtons();

      return;
  }

  updateGizmo();
  updateToolbarButtons();

  ObjectEditor::onClick(pcb_id, panel);
}


bool IvyObjectEditor::findTargetPos(IGenViewportWnd *wnd, int x, int y, Point3 &out)
{
  Point3 dir, world;
  real dist = DAGORED2->getMaxTraceDistance();

  wnd->clientToWorld(Point2(x, y), world, dir);

  bool hasTarget = IEditorCoreEngine::get()->traceRay(world, dir, dist, NULL);

  if (hasTarget)
  {
    out = world + dir * dist;
    return true;
  }

  return false;
}


void IvyObjectEditor::gatherStaticGeometry(StaticGeometryContainer &cont, int flags)
{
  for (int i = 0; i < objects.size(); i++)
  {
    IvyObject *p = RTTI_cast<IvyObject>(objects[i]);
    if (p)
      p->gatherStaticGeometry(cont, flags);
  }
}
