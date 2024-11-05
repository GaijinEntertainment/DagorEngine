// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "objEd_occ.h"
#include "plugin_occ.h"
#include "occluder.h"

#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_ObjectCreator.h>
#include <coolConsole/coolConsole.h>
#include <oldEditor/de_cm.h>
#include <de3_editorEvents.h>

#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>

#include <winGuiWrapper/wgw_input.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <propPanel/commonWindow/dialogWindow.h>

#include <debug/dag_debug.h>

using editorcore_extapi::dagConsole;
using editorcore_extapi::dagGeom;
using editorcore_extapi::dagRender;

enum
{
  CM_ = CM_PLUGIN_BASE + 1,
  CM_CREATE_OCCLUDER_BOX,
  CM_CREATE_OCCLUDER_QUAD,
  CM_SHOW_LOCAL_OCCLUDERS,
  CM_SHOW_ALL_OCCLUDERS,
  CM_GATHER_OCCLUDERS,
};


occplugin::ObjEd::ObjEd() : cloneMode(false), objCreator(NULL)
{
  Occluder::initStatics();
  ptDynBuf = dagRender->newDynRenderBuffer("editor_helper_no_tex_blend", midmem);
  secTime = 0;
  showLocalOccluders = true;
  showGlobalOccluders = false;
}


occplugin::ObjEd::~ObjEd() { dagRender->deleteDynRenderBuffer(ptDynBuf); }


void occplugin::ObjEd::fillToolBar(PropPanel::ContainerPropertyControl *toolbar)
{
  PropPanel::ContainerPropertyControl *tb1 = toolbar->createToolbarPanel(0, "");

  addButton(tb1, CM_CREATE_OCCLUDER_BOX, "create_box", "Create box Occluder (1)", true);
  addButton(tb1, CM_CREATE_OCCLUDER_QUAD, "create_plane", "Create quad Occluder (2)", true);

  ObjectEditor::fillToolBar(toolbar);

  PropPanel::ContainerPropertyControl *tb2 = toolbar->createToolbarPanel(0, "");

  addButton(tb2, CM_SHOW_LOCAL_OCCLUDERS, "show_loc_occ", "Show local occluders (Ctrl-1)", true);
  addButton(tb2, CM_SHOW_ALL_OCCLUDERS, "show_occ", "Show all occluders (Ctrl-2)", true);
  addButton(tb2, CM_GATHER_OCCLUDERS, "compile", "Gather occluders to show (F1)");

  // toolBar->checkButton(CM_OBJED_OBJPROP_PANEL, false);
  // toolBar->setBool(CM_OBJED_OBJPROP_PANEL, false);
}


void occplugin::ObjEd::addButton(PropPanel::ContainerPropertyControl *tb, int id, const char *bmp_name, const char *hint, bool check)
{
  if (id == CM_OBJED_DROP || id == CM_OBJED_MODE_SURF_MOVE)
    return;
  ObjectEditor::addButton(tb, id, bmp_name, hint, check);
}


void occplugin::ObjEd::updateToolbarButtons()
{
  ObjectEditor::updateToolbarButtons();
  setRadioButton(CM_CREATE_OCCLUDER_BOX, getEditMode());
  setRadioButton(CM_CREATE_OCCLUDER_QUAD, getEditMode());
  setButton(CM_SHOW_LOCAL_OCCLUDERS, showLocalOccluders);
  setButton(CM_SHOW_ALL_OCCLUDERS, showGlobalOccluders);
}

void occplugin::ObjEd::reset()
{
  setCreateBySampleMode(NULL);
  del_it(objCreator);
  showLocalOccluders = true;
  showGlobalOccluders = false;
}

void occplugin::ObjEd::beforeRender()
{
  ObjectEditor::beforeRender();

  for (int i = 0; i < objects.size(); i++)
  {
    Occluder *p = RTTI_cast<Occluder>(objects[i]);
    if (p)
      p->beforeRender();
  }
}


void occplugin::ObjEd::objRender(dag::ConstSpan<TMatrix> ob, dag::ConstSpan<IOccluderGeomProvider::Quad> oq)
{
  bool curPlugin = DAGORED2->curPlugin() == Plugin::self;

  dagRender->startLinesRender();
  if (showLocalOccluders)
  {
    for (int i = 0; i < objects.size(); i++)
    {
      Occluder *p = RTTI_cast<Occluder>(objects[i]);
      if (p)
        p->render();
    }
  }

  if (showGlobalOccluders && (ob.size() + oq.size()) > 0)
  {
    E3DCOLOR c(20, 240, 30, 255);
    for (int i = 0; i < ob.size(); i++)
    {
      dagRender->setLinesTm(ob[i]);
      dagRender->renderBox(Occluder::normBox, c);
    }

    c = E3DCOLOR(120, 240, 30, 255);
    dagRender->setLinesTm(TMatrix::IDENT);
    for (int i = 0; i < oq.size(); i++)
    {
      dagRender->renderLine(oq[i].v[0], oq[i].v[1], c);
      dagRender->renderLine(oq[i].v[1], oq[i].v[2], c);
      dagRender->renderLine(oq[i].v[2], oq[i].v[3], c);
      dagRender->renderLine(oq[i].v[3], oq[i].v[0], c);
    }
  }
  dagRender->endLinesRender();

  if (objCreator)
    objCreator->render();
}


void occplugin::ObjEd::objRenderTr(dag::ConstSpan<TMatrix> ob, dag::ConstSpan<IOccluderGeomProvider::Quad> oq)
{
  bool curPlugin = DAGORED2->curPlugin() == Plugin::self;
  if (!curPlugin)
    return;

  TMatrix4 gtm;
  d3d::getglobtm(gtm);

  dagRender->dynRenderBufferClearBuf(*ptDynBuf);

  int cnt = 0;

  if (showLocalOccluders)
    for (int i = 0; i < objects.size(); i++)
    {
      Occluder *p = RTTI_cast<Occluder>(objects[i]);
      if (p)
      {
        p->renderCommonGeom(*ptDynBuf, gtm);

        cnt++;
        if (cnt >= 512)
          dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, BAD_TEXTUREID);
      }
    }

  if (showGlobalOccluders && (ob.size() + oq.size()) > 0)
  {
    E3DCOLOR c(20, 240, 30, 63);
    for (int i = 0; i < ob.size(); i++)
    {
      Occluder::renderBox(*ptDynBuf, ob[i], c);
      cnt++;
      if (cnt >= 512)
        dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, BAD_TEXTUREID);
    }

    c = E3DCOLOR(120, 240, 30, 63);
    for (int i = 0; i < oq.size(); i++)
    {
      Occluder::renderQuad(*ptDynBuf, oq[i].v, c, false);
      cnt++;
      if (cnt >= 512)
        dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, BAD_TEXTUREID);
    }
  }

  if (objCreator)
  {
    if (getEditMode() == CM_CREATE_OCCLUDER_BOX && ((BoxCreator *)objCreator)->getStageNo())
    {
      TMatrix tm = objCreator->matrix;
      tm.setcol(3, tm * Point3(0, 0.5, 0));
      Occluder::renderBox(*ptDynBuf, tm, E3DCOLOR(10, 10, 250, 64));
    }
    else if (getEditMode() == CM_CREATE_OCCLUDER_QUAD && ((PlaneCreator *)objCreator)->getStageNo())
    {
      TMatrix tm;
      tm.setcol(0, objCreator->matrix.getcol(0));
      tm.setcol(1, objCreator->matrix.getcol(2));
      tm.setcol(2, -objCreator->matrix.getcol(1));
      tm.setcol(3, objCreator->matrix.getcol(3));
      Occluder::renderQuad(*ptDynBuf, tm, E3DCOLOR(10, 160, 250, 64));
    }
  }

  dagRender->dynRenderBufferFlushToBuffer(*ptDynBuf, BAD_TEXTUREID);
  dagRender->dynRenderBufferFlush(*ptDynBuf);
}


void occplugin::ObjEd::gizmoStarted()
{
  ObjectEditor::gizmoStarted();

  for (int i = 0; i < selection.size(); ++i)
    if (!RTTI_cast<Occluder>(selection[i]))
      return;

  // const int keyModif = dagGui->ctlInpDevGetModifiers();
  bool shift_pressed = wingw::is_key_pressed(wingw::V_SHIFT);

  // if ((keyModif & SHIFT_PRESSED) && getEditMode() == CM_OBJED_MODE_MOVE)
  if (shift_pressed && getEditMode() == CM_OBJED_MODE_MOVE)
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
      Occluder *p = RTTI_cast<Occluder>(oldSel[i]);
      if (!p)
        continue;

      Occluder *np = new Occluder(*p);
      addObject(np);

      np->selectObject();
    }
  }
}


void occplugin::ObjEd::gizmoEnded(bool apply)
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
}


void occplugin::ObjEd::save(DataBlock &blk)
{
  for (int i = 0; i < objects.size(); i++)
  {
    Occluder *p = RTTI_cast<Occluder>(objects[i]);
    if (p)
      p->save(*blk.addNewBlock("Occluder"));
  }
}


void occplugin::ObjEd::load(const DataBlock &blk)
{
  int nid = blk.getNameId("Occluder");

  for (int i = 0; i < blk.blockCount(); i++)
  {
    const DataBlock &cb = *blk.getBlock(i);
    if (cb.getBlockNameId() != nid)
      continue;

    Ptr<Occluder> s = new Occluder(true);
    s->load(cb);
    addObject(s, false);
  }
}


void occplugin::ObjEd::getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types)
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
    Occluder *p = RTTI_cast<Occluder>(objects[i]);
    if (p && showIvys)
    {
      names.push_back() = p->getName();

      if (p->isSelected())
        sel_names.push_back() = p->getName();
    }
  }
}


void occplugin::ObjEd::getTypeNames(Tab<String> &names)
{
  names.resize(1);
  names[0] = "Occluders";
}


void occplugin::ObjEd::onSelectedNames(const Tab<String> &names)
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


void occplugin::ObjEd::update(real dt)
{
  updateGizmo();
  if (objCreator && objCreator->isFinished())
  {
    if (objCreator->isOk())
    {
      if (getEditMode() == CM_CREATE_OCCLUDER_BOX)
      {
        // create box occluder
        Occluder *obj = new Occluder(true);
        objCreator->matrix.setcol(3, objCreator->matrix * Point3(0, 0.5, 0));
        obj->setWtm(objCreator->matrix);

        getUndoSystem()->begin();
        addObject(obj);
        getUndoSystem()->accept("Create box occluder");

        onClick(CM_CREATE_OCCLUDER_BOX, NULL);
      }
      else
      {
        // create quad occluder
        Occluder *obj = new Occluder(false);
        TMatrix tm;
        tm.setcol(0, objCreator->matrix.getcol(0));
        tm.setcol(1, objCreator->matrix.getcol(2));
        tm.setcol(2, -objCreator->matrix.getcol(1));
        tm.setcol(3, objCreator->matrix.getcol(3));
        obj->setWtm(tm);

        getUndoSystem()->begin();
        addObject(obj);
        getUndoSystem()->accept("Create quad occluder");

        onClick(CM_CREATE_OCCLUDER_QUAD, NULL);
      }
    }
    else
      onClick(CM_OBJED_MODE_SELECT, NULL);
  }
}

static int last_mx = 0, last_my = 0, last_inside = 0;
static IGenViewportWnd *last_wnd = NULL;

bool occplugin::ObjEd::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
  {
    last_mx = x;
    last_my = y;
    last_inside = inside;
    last_wnd = wnd;
    // return objCreator->handleMouseMove(wnd, x, y, inside, buttons, key_modif^CTRL_PRESSED, true);
    return objCreator->handleMouseMove(wnd, x, y, inside, buttons, !wingw::is_key_pressed(wingw::V_CONTROL), true);
  }
  return ObjectEditor::handleMouseMove(wnd, x, y, inside, buttons, key_modif);
}


bool occplugin::ObjEd::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
    return objCreator->handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
  return ObjectEditor::handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
}


bool occplugin::ObjEd::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
    return objCreator->handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif);
  return ObjectEditor::handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif);
}


bool occplugin::ObjEd::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (objCreator)
    return objCreator->handleMouseRBPress(wnd, x, y, inside, buttons, key_modif);
  return ObjectEditor::handleMouseRBPress(wnd, x, y, inside, buttons, key_modif);
}


void occplugin::ObjEd::handleKeyPress(IGenViewportWnd *wnd, int vk, int modif)
{
  if (getEditMode() == CM_CREATE_OCCLUDER_QUAD && wingw::is_key_pressed(wingw::V_CONTROL))
  {
    if (last_wnd == wnd && last_inside)
      // objCreator->handleMouseMove(wnd, last_mx, last_my, true, 0, modif^CTRL_PRESSED, true);
      objCreator->handleMouseMove(wnd, last_mx, last_my, true, 0, !wingw::is_key_pressed(wingw::V_CONTROL), true);
    return;
  }

  if (!wingw::is_special_pressed())
  {
    if (wingw::is_key_pressed('1'))
      onClick(CM_CREATE_OCCLUDER_BOX, NULL);
    if (wingw::is_key_pressed('2'))
      onClick(CM_CREATE_OCCLUDER_QUAD, NULL);
    if (wingw::is_key_pressed(wingw::V_F1))
      onClick(CM_GATHER_OCCLUDERS, NULL);
    if (wingw::is_key_pressed(wingw::V_ESC) && objCreator)
      onClick(CM_OBJED_MODE_SELECT, NULL);
  }

  if (modif & wingw::M_CTRL)
  {
    if (wingw::is_key_pressed('1'))
      onClick(CM_SHOW_LOCAL_OCCLUDERS, NULL);
    if (wingw::is_key_pressed('2'))
      onClick(CM_SHOW_ALL_OCCLUDERS, NULL);
  }

  ObjectEditor::handleKeyPress(wnd, vk, modif);
}

void occplugin::ObjEd::handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif)
{
  // if (getEditMode() == CM_CREATE_OCCLUDER_QUAD && !(modif & CTRL_PRESSED))
  if (getEditMode() == CM_CREATE_OCCLUDER_QUAD && !wingw::is_key_pressed(wingw::V_CONTROL))
  {
    if (last_wnd == wnd && last_inside)
      objCreator->handleMouseMove(wnd, last_mx, last_my, true, 0, !wingw::is_key_pressed(wingw::V_CONTROL), true);
    return;
  }
  ObjectEditor::handleKeyRelease(wnd, vk, modif);
}


void occplugin::ObjEd::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case CM_OBJED_MODE_SELECT:
    case CM_OBJED_MODE_MOVE:
    case CM_OBJED_MODE_ROTATE:
    case CM_OBJED_MODE_SCALE:
      del_it(objCreator);
      setEditMode(pcb_id);
      break;

    case CM_CREATE_OCCLUDER_BOX:
      showLocalOccluders = true;
      del_it(objCreator);
      setEditMode(pcb_id);
      objCreator = dagGeom->newBoxCreator();
      last_wnd = NULL;
      break;

    case CM_CREATE_OCCLUDER_QUAD:
      showLocalOccluders = true;
      del_it(objCreator);
      setEditMode(pcb_id);
      objCreator = dagGeom->newPlaneCreator();
      last_wnd = NULL;
      break;

    case CM_SHOW_LOCAL_OCCLUDERS:
      showLocalOccluders = panel->getBool(pcb_id);
      IEditorCoreEngine::get()->invalidateViewportCache();
      break;

    case CM_SHOW_ALL_OCCLUDERS:
      showGlobalOccluders = panel->getBool(pcb_id);
      IEditorCoreEngine::get()->invalidateViewportCache();
      break;

    case CM_GATHER_OCCLUDERS:
    {
      PropPanel::DialogWindow *myDlg = DAGORED2->createDialog(hdpi::_pxScaled(240), hdpi::_pxScaled(480), "Gather occluders");
      PropPanel::ContainerPropertyControl *myPanel = myDlg->getPanel();
      Plugin::self->fillExportPanel(*myPanel);

      if (myDlg->showDialog() != PropPanel::DIALOG_ID_OK)
        dagConsole->showConsole(DAGORED2->getConsole(), true);
      else if (!Plugin::self->validateBuild(_MAKE4C('PC'), DAGORED2->getConsole(), myPanel))
        wingw::message_box(wingw::MBS_EXCL, "Gather failed", "Errors during occluders gathering\nSee console for details.");

      DAGORED2->spawnEvent(HUID_UseVisibilityOpt, NULL);

      DAGORED2->deleteDialog(myDlg);
    }
    break;
  }

  updateGizmo();
  updateToolbarButtons();

  ObjectEditor::onClick(pcb_id, panel);
}
