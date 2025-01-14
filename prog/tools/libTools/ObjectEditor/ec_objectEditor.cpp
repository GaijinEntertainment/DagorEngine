// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_ObjectEditor.h>
#include <EditorCore/ec_rendEdObject.h>
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_IObjectCreator.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/captureCursor.h>
#include <EditorCore/ec_rect.h>

#include <winGuiWrapper/wgw_dialogs.h>
#include <winGuiWrapper/wgw_input.h>
#include <libTools/util/strUtil.h>
#include <generic/dag_sort.h>

#include <math/dag_mathAng.h>

#include <debug/dag_debug.h>
#include <de3_interface.h>
#include <propPanel/control/panelWindow.h>

#define MAX_TRACE_DIST 1000

#define ROTATE_AND_SCALE_DIVIZER 100.0
#define MIN_SCALE                0.05
#define SAFE_TRASHOLD            15

enum
{
  PROPBAR_EDITOR_WTYPE = 130,
};

enum
{
  PROPBAR_WIDTH = 280,
};


//==================================================================================================
// ObjectEditor
//==================================================================================================
ObjectEditor::ObjectEditor() :
  objects(midmem_ptr()),
  selection(midmem_ptr()),
  editMode(0),
  isGizmoValid(false),
  gizmoPt(0, 0, 0),
  gizmoOrigin(0, 0, 0),
  isGizmoStarted(false),
  objectPropBar(NULL),
  areObjectPropsValid(false),
  sample(NULL),
  creator(NULL),
  objListOwnerName(""),
  gizmoScl(0, 0, 0),
  gizmoRot(0, 0, 0),
  gizmoSclO(0, 0, 0),
  gizmoRotO(0, 0, 0),
  suffixDigitsCount(2),
  toolBarId(-1),
  filterString("")
{}


ObjectEditor::~ObjectEditor()
{
  sample = NULL;
  del_it(creator);

  closeUi();

  if (IEditorCoreEngine::get())
    IEditorCoreEngine::get()->setGizmo(NULL, IEditorCoreEngine::MODE_None);
}

RenderableEditableObject *ObjectEditor::getObjectByName(const char *name) const
{
  if (!name)
    return NULL;

  for (int i = 0; i < objects.size(); ++i)
  {
    if (stricmp(objects[i]->getName(), name) == 0)
      return objects[i];
  }
  return NULL;
}


bool ObjectEditor::getSelectionBox(BBox3 &box) const
{
  if (!selection.size())
    return false;

  bool gotBox = false;

  for (int i = 0; i < selection.size(); ++i)
  {
    BBox3 b;
    if (selection[i]->getWorldBox(b))
    {
      box += b;
      gotBox = true;
    }
  }

  return gotBox;
}


void ObjectEditor::selectionChanged()
{
  invalidateObjectProps();
  isGizmoValid = false;

  IEditorCoreEngine::get()->updateViewports();
  IEditorCoreEngine::get()->invalidateViewportCache();
}


void ObjectEditor::selectAll()
{
  getUndoSystem()->begin();
  unselectAll();
  for (int i = 0; i < objects.size(); ++i)
  {
    RenderableEditableObject *o = objects[i];
    if (!canSelectObj(o))
      continue;
    o->selectObject();
  }
  updateGizmo();
  getUndoSystem()->accept("Select");
}


void ObjectEditor::unselectAll()
{
  for (int i = 0; i < objects.size(); ++i)
    objects[i]->selectObject(false);
}


void ObjectEditor::update(real dt)
{
  updateGizmo();
  updateObjectProps();

  for (int i = 0; i < objects.size(); ++i)
    objects[i]->update(dt);

  if (creator && creator->isFinished())
  {
    if (creator->isOk())
    {
      createObject(creator);
    }
    else
    {
      del_it(creator);
      setEditMode(CM_OBJED_MODE_SELECT);
    }
  }
}


void ObjectEditor::beforeRender()
{
  for (int i = 0; i < objects.size(); ++i)
    objects[i]->beforeRender();
}


void ObjectEditor::render()
{
  for (int i = 0; i < objects.size(); ++i)
    if (!objects[i]->isHidden())
      objects[i]->render();
  if (creator)
    creator->render();
}


void ObjectEditor::renderTrans()
{
  for (int i = 0; i < objects.size(); ++i)
    if (!objects[i]->isHidden())
      objects[i]->renderTrans();
}


void ObjectEditor::updateImgui()
{
  if (objectPropBar)
  {
    PropPanel::PanelWindowPropertyControl *panelWindow = objectPropBar->getPanel();

    bool open = true;
    DAEDITOR3.imguiBegin(*panelWindow, &open);
    panelWindow->updateImgui();
    DAEDITOR3.imguiEnd();

    if (!open && objectPropBar)
    {
      showPanel();
      EDITORCORE->managePropPanels();
    }
  }
}


void ObjectEditor::updateSelection()
{
  selection.clear();

  for (int i = 0; i < objects.size(); ++i)
  {
    RenderableEditableObject *o = objects[i];
    if (o->isSelected())
      selection.push_back(o);
  }

  selectionChanged();
}


void ObjectEditor::onObjectFlagsChange(RenderableEditableObject *obj, int changed_flags)
{
  if (changed_flags & RenderableEditableObject::FLG_SELECTED)
  {
    if (obj->isSelected())
    {
      selection.push_back(obj);
      selectionChanged();
    }
    else
    {
      for (int i = 0; i < selection.size(); ++i)
        if (selection[i] == obj)
        {
          erase_items(selection, i, 1);
          selectionChanged();
          break;
        }
    }
  }
}


void ObjectEditor::onObjectGeomChange(RenderableEditableObject *obj)
{
  IEditorCoreEngine::get()->updateViewports();
  IEditorCoreEngine::get()->invalidateViewportCache();

  if (!isGizmoStarted)
    isGizmoValid = false;
}


void ObjectEditor::onAddObject(RenderableEditableObject &obj) { obj.onAdd(this); }


void ObjectEditor::_addObjects(RenderableEditableObject **obj, int num, bool use_undo)
{
  if (num <= 0)
    return;

  UndoAddObjects *undo = NULL;
  if (use_undo)
    undo = new (midmem) UndoAddObjects(this, num);

  objects.reserve(num);

  for (int i = 0; i < num; ++i)
  {
    RenderableEditableObject *o = obj[i];
    if (!o)
      continue;

    int j;
    for (j = 0; j < objects.size(); ++j)
      if (objects[j] == o)
        break;

    if (j < objects.size())
      continue;

    // G_ASSERT(!o->objEditor);
    onAddObject(*o);
    o->objEditor = this;

    objects.push_back(o);

    if (undo)
      undo->objects.push_back(o);

    if (o->isSelected())
    {
      selection.push_back(o);
      selectionChanged();
    }
  }

  if (undo)
  {
    undo->objects.shrink_to_fit();
    getUndoSystem()->put(undo);
  }

  IEditorCoreEngine::get()->updateViewports();
  IEditorCoreEngine::get()->invalidateViewportCache();
}


void ObjectEditor::onRemoveObject(RenderableEditableObject &obj) { obj.onRemove(this); }


void ObjectEditor::_removeObjects(RenderableEditableObject **obj, int num, bool use_undo)
{
  if (num <= 0)
    return;

  UndoRemoveObjects *undo = NULL;
  if (use_undo)
    undo = new (midmem) UndoRemoveObjects(this, num);

  for (int i = 0; i < num; ++i)
  {
    Ptr<RenderableEditableObject> o = obj[i];
    if (!o)
      continue;

    bool found = false;

    int j;
    for (j = 0; j < objects.size(); ++j)
      if (objects[j] == o)
      {
        G_ASSERT(o->objEditor == this);
        o->objEditor = NULL;
        erase_items(objects, j, 1);
        found = true;
        break;
      }

    if (!found)
      continue;

    if (o == sample)
      sample = NULL;

    onRemoveObject(*o);

    if (undo)
      undo->objects.push_back(o);

    for (j = 0; j < selection.size(); ++j)
      if (selection[j] == o)
      {
        erase_items(selection, j, 1);
        selectionChanged();
        break;
      }
  }

  if (undo)
  {
    undo->objects.shrink_to_fit();
    getUndoSystem()->put(undo);
  }

  IEditorCoreEngine::get()->updateViewports();
  IEditorCoreEngine::get()->invalidateViewportCache();
}


void ObjectEditor::removeAllObjects(bool use_undo)
{
  if (!objects.size())
    return;
  PtrTab<RenderableEditableObject> objs(tmpmem);
  objs = objects;

  removeObjects((RenderableEditableObject **)&objs[0], objs.size(), use_undo);
}


void ObjectEditor::deleteSelectedObjects(bool use_undo)
{
  Tab<RenderableEditableObject *> list(tmpmem);
  list.reserve(selection.size());

  getUndoSystem()->begin();

  for (int i = 0; i < selection.size(); ++i)
    if (selection[i]->mayDelete())
      list.push_back(selection[i]);

  if (!list.size())
  {
    getUndoSystem()->cancel();
    return;
  }

  removeObjects(&list[0], list.size(), use_undo);
  getUndoSystem()->accept("Delete");
}


UndoSystem *ObjectEditor::getUndoSystem() { return IEditorCoreEngine::get()->getUndoSystem(); }


bool ObjectEditor::checkObjSelFilter(RenderableEditableObject &obj)
{
  if (filterStrings.size())
  {
    String objName(obj.getName());
    objName.toLower();
    for (int i = 0; i < filterStrings.size(); i++)
      if (strstr(objName, filterStrings[i]))
        return !invFilter;
    return invFilter;
  }

  return true;
}


bool ObjectEditor::canSelectObj(RenderableEditableObject *o) { return o && !o->isHidden() && !o->isFrozen() && checkObjSelFilter(*o); }


RenderableEditableObject *ObjectEditor::pickObject(IGenViewportWnd *wnd, int x, int y)
{
  Tab<RenderableEditableObject *> objs;
  if (pickObjects(wnd, x, y, objs))
    return objs[0];
  return NULL;
}


static Point2 cmp_obj_z_pt;
static IGenViewportWnd *cmp_obj_z_wnd = NULL;
static int cmp_obj_z(RenderableEditableObject *const *a, RenderableEditableObject *const *b)
{
  Point3 pt_a = a[0]->getPos();
  Point3 pt_b = b[0]->getPos();

  Point3 tmp;
  Point3 dir;
  cmp_obj_z_wnd->clientToWorld(cmp_obj_z_pt, tmp, dir);

  BBox3 box;
  if (a[0]->getWorldBox(box))
    pt_a += dir * length(box.width()) / 2;
  if (b[0]->getWorldBox(box))
    pt_b += dir * length(box.width()) / 2;

  Point2 scr_a, scr_b;
  float a_z = 0, b_z = 0;
  if (!cmp_obj_z_wnd->worldToClient(pt_a, scr_a, &a_z))
    a_z = 100000;
  else if (a_z < 0)
    a_z = 100000 - a_z;
  if (!cmp_obj_z_wnd->worldToClient(pt_b, scr_b, &b_z))
    b_z = 100000;
  else if (b_z < 0)
    b_z = 100000 - b_z;

  return (a_z < b_z) ? 1 : (a_z > b_z ? -1 : 0);
}

bool ObjectEditor::pickObjects(IGenViewportWnd *wnd, int x, int y, Tab<RenderableEditableObject *> &objs)
{
  bool picked = false;

  for (int i = 0; i < objects.size(); ++i)
  {
    RenderableEditableObject *o = objects[i];
    if (!canSelectObj(o))
      continue;

    if (!o->isSelectedByPointClick(wnd, x, y))
      continue;

    objs.push_back(o);
    picked = true;
  }
  if (objs.size() > 1)
  {
    cmp_obj_z_pt.set((real)x, (real)y);
    cmp_obj_z_wnd = wnd;
    sort(objs, &cmp_obj_z);
    cmp_obj_z_wnd = NULL;
  }
  return picked;
}


Point3 ObjectEditor::getPt() { return gizmoPt; }


bool ObjectEditor::getRot(Point3 &p)
{
  if (!selection.size())
    return false;
  ::matrix_to_euler(selection[0]->getWtm(), gizmoRot.y, gizmoRot.z, gizmoRot.x);
  p = gizmoRot;
  return true;
}


bool ObjectEditor::getScl(Point3 &p)
{
  if (!selection.size())
    return false;
  gizmoScl = Point3(0, 0, 0);
  for (int i = 0; i < selection.size(); ++i)
  {
    Matrix3 m = selection[i]->getMatrix();
    gizmoScl.x += length(m.getcol(0));
    gizmoScl.y += length(m.getcol(1));
    gizmoScl.z += length(m.getcol(2));
  }
  gizmoScl = gizmoScl / real(selection.size());
  p = gizmoScl;
  return true;
}


bool ObjectEditor::getAxes(Point3 &ax, Point3 &ay, Point3 &az)
{
  if (IEditorCoreEngine::get()->getGizmoBasisType() == IEditorCoreEngine::BASIS_Local && selection.size() > 0)
  {
    const TMatrix tm = selection[0]->getWtm();

    ax = normalize(tm.getcol(0));
    ay = normalize(tm.getcol(1));
    az = normalize(tm.getcol(2));
  }
  else
  {
    ax = Point3(1, 0, 0);
    ay = Point3(0, 1, 0);
    az = Point3(0, 0, 1);
  }

  return true;
}


void ObjectEditor::moveObjects(PtrTab<RenderableEditableObject> &obj, const Point3 &delta, IEditorCoreEngine::BasisType basis)
{
  for (int i = 0; i < obj.size(); ++i)
    obj[i]->moveObject(delta, basis);
}

void ObjectEditor::saveNormalOnSelection(const Point3 &n)
{
  for (int i = 0; i < selection.size(); i++)
    selection[i]->setSavedPlacementNormal(n);
}

void ObjectEditor::setCollisionIgnoredOnSelection()
{
  for (int i = 0; i < selection.size(); i++)
    selection[i]->setCollisionIgnored();
}

void ObjectEditor::resetCollisionIgnoredOnSelection()
{
  for (int i = 0; i < selection.size(); i++)
    selection[i]->resetCollisionIgnored();
}

void ObjectEditor::changed(const Point3 &delta)
{
  if (editMode != CM_OBJED_MODE_MOVE && editMode != CM_OBJED_MODE_SURF_MOVE)
  {
    getUndoSystem()->cancel();
    getUndoSystem()->begin();
  }

  IEditorCoreEngine::ModeType gt = editModeToModeType(editMode);
  IEditorCoreEngine::BasisType basis = IEditorCoreEngine::get()->getGizmoBasisTypeForMode(gt);

  Point3 origin = gizmoOrigin;

  switch (editMode)
  {
    case CM_OBJED_MODE_MOVE:
      moveObjects(selection, delta, basis);
      updateGizmo(basis);
      break;

    case CM_OBJED_MODE_SURF_MOVE:
      for (int i = 0; i < selection.size(); ++i)
        selection[i]->moveSurfObject(delta, basis);
      updateGizmo(basis);
      break;

    case CM_OBJED_MODE_ROTATE:
      for (int i = 0; i < selection.size(); ++i)
      {
        selection[i]->putRotateUndo();
        selection[i]->rotateObject(-delta, origin, basis);
      }
      gizmoRot = gizmoRotO + delta;
      break;

    case CM_OBJED_MODE_SCALE:
      for (int i = 0; i < selection.size(); ++i)
      {
        selection[i]->putScaleUndo();
        selection[i]->scaleObject(delta, origin, basis);
      }
      gizmoScl = gizmoSclO + delta;
      break;
  }
}


void ObjectEditor::gizmoStarted()
{
  gizmoOrigin = gizmoPt;
  isGizmoStarted = true;

  getUndoSystem()->begin();

  switch (editMode)
  {
    case CM_OBJED_MODE_MOVE:
      for (int i = 0; i < selection.size(); ++i)
        selection[i]->putMoveUndo();
      break;

    case CM_OBJED_MODE_SURF_MOVE:
      for (int i = 0; i < selection.size(); ++i)
      {
        selection[i]->putMoveUndo();
        selection[i]->rememberSurfaceDist();
      }
      break;

    case CM_OBJED_MODE_ROTATE:
      for (int i = 0; i < selection.size(); ++i)
        selection[i]->putRotateUndo();
      break;

    case CM_OBJED_MODE_SCALE:
      for (int i = 0; i < selection.size(); ++i)
        selection[i]->putScaleUndo();
      break;
  }
}


void ObjectEditor::gizmoEnded(bool apply)
{
  if (apply)
  {
    const char *name = "Gizmo Operation";

    switch (editMode)
    {
      case CM_OBJED_MODE_MOVE: name = "Move"; break;
      case CM_OBJED_MODE_SURF_MOVE: name = "Move over surface"; break;
      case CM_OBJED_MODE_ROTATE: name = "Rotate"; break;
      case CM_OBJED_MODE_SCALE: name = "Scale"; break;
    }

    getUndoSystem()->accept(name);
    gizmoRotO = gizmoRot;
    gizmoSclO = gizmoScl;
  }
  else
    getUndoSystem()->cancel();

  isGizmoStarted = false;
  gizmoRotPoint = Point3(0, 0, 0);
  updateGizmo();
}


bool ObjectEditor::canStartChangeAt(IGenViewportWnd *wnd, int x, int y, int gizmo_sel)
{
  Tab<RenderableEditableObject *> objs(tmpmem);
  if (!pickObjects(wnd, x, y, objs))
    return false;

  RenderableEditableObject *obj;
  if (objs.size() > 1 || !objs[0]->isSelected())
  {
    int i;
    for (i = 0; i < objs.size(); ++i)
      if (objs[i]->isSelected())
        break;

    ++i;
    if (i >= objs.size())
      i = 0;

    obj = objs[i];
  }
  else
    obj = objs[0];

  if (!obj->isSelected())
    return false;

  if (obj->objFlags & RenderableEditableObject::FLG_WANTRESELECT)
    obj->selectObject();
  return true;
}


int ObjectEditor::getAvailableTypes()
{
  int types = 0;

  switch (editMode)
  {
    case CM_OBJED_MODE_ROTATE:
      //      types |= CENTER_SelectionNotRotObj;
    case CM_OBJED_MODE_MOVE:
    case CM_OBJED_MODE_SURF_MOVE: types |= IEditorCoreEngine::BASIS_World;
    case CM_OBJED_MODE_SCALE: types |= IEditorCoreEngine::BASIS_Local; break;
  }

  if (types)
    types |= IEditorCoreEngine::CENTER_Pivot | IEditorCoreEngine::CENTER_Selection;

  return types;
}


IEditorCoreEngine::ModeType ObjectEditor::editModeToModeType(int editMode)
{
  switch (editMode)
  {
    case CM_OBJED_MODE_MOVE: return IEditorCoreEngine::MODE_Move;
    case CM_OBJED_MODE_SURF_MOVE: return IEditorCoreEngine::MODE_MoveSurface;
    case CM_OBJED_MODE_ROTATE: return IEditorCoreEngine::MODE_Rotate;
    case CM_OBJED_MODE_SCALE: return IEditorCoreEngine::MODE_Scale;
    default: return IEditorCoreEngine::MODE_None;
  }
}


void ObjectEditor::updateGizmo(int basis)
{
  if (isGizmoValid)
    return;

  if (basis == IEditorCoreEngine::BASIS_None)
    basis = IEditorCoreEngine::get()->getGizmoBasisType();

  IEditorCoreEngine::ModeType gt = editModeToModeType(editMode);

  // calc gizmoPt
  if (selection.size())
  {
    switch (IEditorCoreEngine::get()->getGizmoCenterType())
    {
      case IEditorCoreEngine::CENTER_Pivot:
        if (editMode != CM_OBJED_MODE_SURF_MOVE)
          gizmoPt = selection[0]->getPos();
        else
          gizmoPt = selection[0]->getPos() - Point3(0, selection[0]->getSurfaceDist(), 0);
        break;

      case IEditorCoreEngine::CENTER_Coordinates:
        if (basis == IEditorCoreEngine::BASIS_Local)
          gizmoPt = selection[0]->getPos();
        else
          gizmoPt = Point3(0, 0, 0);
        break;

      case IEditorCoreEngine::CENTER_Selection:
      {
        BBox3 box;
        getSelectionBox(box);

        if (editMode == CM_OBJED_MODE_ROTATE && basis == IEditorCoreEngine::BASIS_World)
        {
          if (gizmoRotPoint.length() < 1e-3)
            gizmoRotPoint = box.center();

          gizmoPt = gizmoRotPoint;
        }
        else if (editMode != CM_OBJED_MODE_SURF_MOVE)
          gizmoPt = box.center();
        else
          gizmoPt = getSurfMoveGizmoPos(box.center());
        break;
      }
    }
  }
  else
    gt = IEditorCoreEngine::MODE_None;

  if (updateViewportGizmo)
  {
    if (IEditorCoreEngine::get()->getGizmoModeType() != (gt & IEditorCoreEngine::GIZMO_MASK_Mode))
    {
      IEditorCoreEngine::get()->setGizmo(gt == IEditorCoreEngine::MODE_None ? NULL : this, gt);
      IEditorCoreEngine::get()->updateViewports();
      IEditorCoreEngine::get()->invalidateViewportCache();
    }
  }
}


Point3 ObjectEditor::getSurfMoveGizmoPos(const Point3 &obj_pos) const
{
  if (usesRendinstPlacement())
  {
    return selection[0]->getPos(); // placement forces object on surface
  }
  else
  {
    real dist = MAX_TRACE_DIST;
    if (IEditorCoreEngine::get()->traceRay(obj_pos + Point3(0, 0.01, 0), Point3(0, -1, 0), dist))
      return obj_pos - Point3(0, dist - 0.01, 0);
    return obj_pos;
  }
}


void ObjectEditor::setEditMode(int mode)
{
  editMode = mode;
  isGizmoValid = false;

  if (mode == CM_OBJED_MODE_SURF_MOVE)
    for (int i = 0; i < selection.size(); ++i)
      selection[i]->rememberSurfaceDist();

  updateToolbarButtons();
}


void ObjectEditor::invalidateObjectProps()
{
  saveEditorPropBarSettings();
  areObjectPropsValid = false;
  IEditorCoreEngine::get()->updateViewports();
  IEditorCoreEngine::get()->invalidateViewportCache();
}


void ObjectEditor::updateObjectProps()
{
  if (objectPropBar)
  {
    if (areObjectPropsValid)
    {
      objectPropBar->updateTransform();
    }
    else
    {
      objectPropBar->fillPanel();
      objectPropBar->loadSettings(objectPropSettings);
      areObjectPropsValid = true;
    }
  }
}


void ObjectEditor::selectByNameDlg() { IEditorCoreEngine::get()->showSelectWindow(this, objListOwnerName); }


void ObjectEditor::dropObjects()
{
  getUndoSystem()->begin();

  const Point3 dir = Point3(0, -1, 0);

  for (int i = 0; i < selection.size(); ++i)
  {
    RenderableEditableObject *obj = selection[i];

    if (obj)
    {
      real dist = MAX_TRACE_DIST;
      const Point3 pos = obj->getPos() + Point3(0, 0.2, 0);

      if (IEditorCoreEngine::get()->traceRay(pos, dir, dist))
      {
        obj->putMoveUndo();
        obj->setPos(pos + dir * dist);
      }
    }
  }

  getUndoSystem()->accept("Drop object(s)");
}


void ObjectEditor::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  IGenViewportWnd *vp = IEditorCoreEngine::get()->getCurrentViewport();
  if (vp)
    vp->activate();

  switch (pcb_id)
  {
    case CM_OBJED_MODE_SELECT:
    case CM_OBJED_MODE_MOVE:
    case CM_OBJED_MODE_SURF_MOVE:
    case CM_OBJED_MODE_ROTATE:
    case CM_OBJED_MODE_SCALE: setEditMode(pcb_id); break;

    case CM_OBJED_OBJPROP_PANEL:
      showPanel();
      EDITORCORE->managePropPanels();
      break;

    case CM_OBJED_SELECT_BY_NAME: selectByNameDlg(); break;

    case CM_OBJED_DROP: dropObjects(); break;

    case CM_OBJED_DELETE: deleteSelectedObjects(); break;
  }
}

void ObjectEditor::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case CM_OBJED_SELECT_FILTER:
    {
      filterString = panel->getText(CM_OBJED_SELECT_FILTER);
      panel->setBool(CM_OBJED_SELECT_FILTER, filterString.length() > 0);

      // parse comma-separated strings with optional '!' start symbol
      invFilter = filterString.length() > 0 && filterString[0] == '!';
      filterStrings.clear();
      const char *fs = filterString.str() + (invFilter ? 1 : 0);
      String s;
      while (fs)
      {
        if (const char *nfs = strchr(fs, ','))
        {
          s.printf(0, "%.*s", nfs - fs, fs);
          fs = nfs + 1;
        }
        else
        {
          s = fs;
          fs = NULL;
        }
        filterStrings.push_back() = s.toLower();
      }
      break;
    }
  }
}


void ObjectEditor::registerViewportAccelerators(IWndManager &wndManager)
{
  wndManager.addViewportAccelerator(CM_OBJED_SELECT_BY_NAME, 'H');
  wndManager.addViewportAccelerator(CM_OBJED_OBJPROP_PANEL, 'P');
}


void ObjectEditor::handleKeyPress(IGenViewportWnd *wnd, int vk, int modif)
{
  bool isAlt = wingw::is_key_pressed(wingw::V_ALT);
  bool isCtrl = wingw::is_key_pressed(wingw::V_CONTROL);
  bool isShift = wingw::is_key_pressed(wingw::V_SHIFT);

  if (!isAlt && !isCtrl && !isShift)
  {
    switch (vk)
    {
      case 'Q': onClick(CM_OBJED_MODE_SELECT, NULL); break;
      case 'W': onClick(CM_OBJED_MODE_MOVE, NULL); break;
      case 'E': onClick(CM_OBJED_MODE_ROTATE, NULL); break;
      case 'R': onClick(CM_OBJED_MODE_SCALE, NULL); break;
      case wingw::V_DELETE: onClick(CM_OBJED_DELETE, NULL); break;
    }
  }

  else if (isCtrl && !isShift && isAlt)
  {
    switch (vk)
    {
      case 'D': onClick(CM_OBJED_DROP, NULL); break;

      case 'W': onClick(CM_OBJED_MODE_SURF_MOVE, NULL); break;
    }
  }
}


bool ObjectEditor::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (creator)
    return creator->handleMouseLBPress(wnd, x, y, inside, buttons, key_modif);
  if (!inside)
    return false;

  if (sample)
  {
    justCreated = true;
    return true;
  }

  Tab<RenderableEditableObject *> objs(tmpmem);
  if (pickObjects(wnd, x, y, objs))
  {
    getUndoSystem()->begin();

    if (wingw::is_key_pressed(wingw::V_ALT))
    {
      // unselect first selected
      for (int i = 0; i < objs.size(); ++i)
        if (objs[i]->isSelected())
        {
          objs[i]->selectObject(false);
          break;
        }
    }
    else if (wingw::is_key_pressed(wingw::V_CONTROL))
    {
      // unselect last selected or select first

      int i;
      for (i = objs.size() - 1; i >= 0; --i)
        if (objs[i]->isSelected())
        {
          objs[i]->selectObject(false);
          break;
        }

      if (i < 0)
        objs[0]->selectObject();
    }
    else if (objs.size() > 1 || !objs[0]->isSelected())
    {
      // select next object after selected one
      int i;
      for (i = 0; i < objs.size(); ++i)
        if (objs[i]->isSelected())
          break;

      ++i;
      if (i >= objs.size())
        i = 0;

      unselectAll();
      objs[i]->selectObject();
    }
    else if (objs.size() && objs[0]->isSelected() && (objs[0]->objFlags & RenderableEditableObject::FLG_WANTRESELECT))
      objs[0]->selectObject();

    getUndoSystem()->accept("select");

    updateGizmo();

    if (editMode != CM_OBJED_MODE_SELECT)
    {
      int i;
      for (i = 0; i < objs.size(); ++i)
        if (objs[i]->isSelected())
          break;

      if (i < objs.size())
        IEditorCoreEngine::get()->startGizmo(wnd, x, y, inside, buttons, key_modif);
    }
    else
      wnd->startRectangularSelection(x, y, key_modif);
  }
  else
    wnd->startRectangularSelection(x, y, key_modif);

  return true;
}


bool ObjectEditor::handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (creator)
    return creator->handleMouseLBRelease(wnd, x, y, inside, buttons, key_modif);
  EcRect rect;
  int type;

  if (sample)
  {
    TMatrix tm = TMatrix::IDENT;
    tm.setcol(3, sample->getPos());
    createObjectBySample(sample);
    sample->setWtm(tm);
    rotDy = 0;
    scaleDy = 0;
    createRot = 0;
    createScale = 1.0;
    justCreated = false;
    canTransformOnCreate = false;
  }

  if (wnd->endRectangularSelection(&rect, &type))
  {
    if (rect.r - rect.l > 3 || rect.b - rect.t > 3)
    {
      getUndoSystem()->begin();

      for (int i = 0; i < objects.size(); ++i)
      {
        RenderableEditableObject *obj = objects[i];
        if (!canSelectObj(obj))
          continue;
        if (obj->isSelectedByRectangle(wnd, rect))
        {
          // hit
          if (type & wingw::M_ALT)
            obj->selectObject(false);
          else if (type & wingw::M_CTRL)
            obj->selectObject();
          else
            obj->selectObject();
        }
        else
        {
          // missed
          if (type & wingw::M_ALT)
            ; // no-op
          else if (type & wingw::M_CTRL)
            ; // no-op
          else
            obj->selectObject(false);
        }
      }

      getUndoSystem()->accept("Select");

      updateGizmo();
    }
    else if (!(type & (wingw::M_ALT | wingw::M_CTRL)))
    {
      RenderableEditableObject *obj = pickObject(wnd, (rect.l + rect.r + 1) / 2, (rect.t + rect.b + 1) / 2);

      if (!obj)
      {
        getUndoSystem()->begin();
        unselectAll();
        getUndoSystem()->accept("Select");

        updateGizmo();
      }
    }
  }

  return true;
}


bool ObjectEditor::handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (!wnd->isActive())
    return false;

  if (creator || (sample && !justCreated))
  {
    setCreateMode(NULL);
    setCreateBySampleMode(NULL);
    setEditMode(CM_OBJED_MODE_SELECT);
  }

  EcRect rect;
  int type;

  bool rectSelectionActive = wnd->endRectangularSelection(&rect, &type);
  if (!rectSelectionActive)
  {
    auto *menu = wnd->getContextMenu();
    if (menu)
      fillSelectionMenu(wnd, menu);
  }

  return true;
}


bool ObjectEditor::handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (creator || (sample && !justCreated))
  {
    setCreateMode(NULL);
    setCreateBySampleMode(NULL);
    setEditMode(CM_OBJED_MODE_SELECT);

    return true;
  }
  return false;
}

TMatrix ObjectEditor::getPlacementRotationMatrix()
{
  TMatrix matrix = TMatrix::IDENT;
  if (selectedPlacementRotation != PlacementRotation::NO_ROTATION)
  {
    Point3 targetDir = normalize(sample->savedPlacementNormal);
    Point3 startingDir = Point3(0, 0, 0);
    startingDir[static_cast<int>(selectedPlacementRotation)] = 1;
    Point3 rotationAxis = normalize(cross(startingDir, targetDir));
    float rotationAmount = acos(dot(startingDir, targetDir));
    matrix.makeTM(rotationAxis, rotationAmount);
  }
  return matrix;
}

bool ObjectEditor::handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (creator)
    return creator->handleMouseMove(wnd, x, y, inside, buttons, key_modif, false);

  if (sample)
  {
    if (justCreated)
    {
      bool doTransform = false;
      int wrapedX = 0;
      int wrapedY = 0;

      if (wrapedY)
        lastY = y;

      switch (buttons)
      {
        // rotate
        case wingw::V_LBUTTON: // This is actually MK_LBUTTON.
          rotDy += lastY - y;

          if (!canTransformOnCreate && (rotDy > SAFE_TRASHOLD || rotDy < -SAFE_TRASHOLD))
          {
            rotDy = 0;
            canTransformOnCreate = true;
          }

          lastY = y;
          createRot = rotDy / ROTATE_AND_SCALE_DIVIZER;

          if (IEditorCoreEngine::get()->getGrid().getRotateSnap())
            createRot = IEditorCoreEngine::get()->snapToAngle(Point3(0, createRot, 0)).y;

          doTransform = true;
          break;

        // scale
        case wingw::V_LBUTTON | wingw::V_RBUTTON: // This is actually MK_LBUTTON | MK_RBUTTON.
          scaleDy += lastY - y;

          if (!canTransformOnCreate && (scaleDy > SAFE_TRASHOLD || scaleDy < -SAFE_TRASHOLD))
          {
            scaleDy = 0;
            canTransformOnCreate = true;
          }

          lastY = y;
          createScale = 1.0 + scaleDy / ROTATE_AND_SCALE_DIVIZER;

          if (createScale < MIN_SCALE)
          {
            createScale = MIN_SCALE;
            scaleDy = (MIN_SCALE - 1) * ROTATE_AND_SCALE_DIVIZER;
          }

          if (IEditorCoreEngine::get()->getGrid().getScaleSnap())
            createScale = IEditorCoreEngine::get()->snapToAngle(Point3(createScale, createScale, createScale)).x;

          doTransform = true;
          break;
      }

      if (doTransform && canTransformOnCreate)
      {
        TMatrix tm;

        tm.setcol(0, Point3(createScale, 0, 0));
        tm.setcol(1, Point3(0, createScale, 0));
        tm.setcol(2, Point3(0, 0, createScale));

        tm *= getPlacementRotationMatrix();

        switch (selectedPlacementRotation)
        {
          case PlacementRotation::X_TO_NORMAL: tm *= rotxTM(createRot); break;
          case PlacementRotation::Z_TO_NORMAL: tm *= rotzTM(createRot); break;
          default: tm *= rotyTM(createRot); break;
        }

        tm.setcol(3, sample->getPos());
        sample->setWtm(tm);
      }
    }
    else
    {
      Point3 world;
      Point3 dir;
      real dist = MAX_TRACE_DIST;
      Point2 mouse = Point2(x, y);

      wnd->clientToWorld(mouse, world, dir);

      sample->savedPlacementNormal = Point3(0, 1, 0);
      IEditorCoreEngine::get()->setupColliderParams(1, BBox3());
      bool traceSuccess = IEditorCoreEngine::get()->traceRay(world, dir, dist, &sample->savedPlacementNormal);
      IEditorCoreEngine::get()->setupColliderParams(0, BBox3());
      if (traceSuccess)
      {
        const Point3 worldPos = world + dir * dist;
        TMatrix matrix = getPlacementRotationMatrix();
        matrix.setcol(3,
          IEditorCoreEngine::get()->getGrid().getMoveSnap() ? IEditorCoreEngine::get()->snapToGrid(worldPos) : worldPos);
        sample->setWtm(matrix);
      }
      lastY = mouse.y;
    }
    IEditorCoreEngine::get()->updateViewports();
    IEditorCoreEngine::get()->invalidateViewportCache();
    return true;
  }

  if (inside)
  {
    RenderableEditableObject *obj = pickObject(wnd, x, y);
    return true;
  }

  return false;
}


void ObjectEditor::getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types)
{
  for (int i = 0; i < objects.size(); ++i)
  {
    RenderableEditableObject *o = objects[i];
    if (!canSelectObj(o))
      continue;

    names.push_back() = o->getName();
    if (o->isSelected())
      sel_names.push_back() = o->getName();
  }
}


void ObjectEditor::onSelectedNames(const Tab<String> &names)
{
  getUndoSystem()->begin();

  unselectAll();
  for (int i = 0; i < names.size(); ++i)
  {
    RenderableEditableObject *o = getObjectByName(names[i]);
    if (!canSelectObj(o))
      continue;
    o->selectObject();
  }

  getUndoSystem()->accept("Select");

  updateGizmo();
}


void ObjectEditor::renameObject(RenderableEditableObject *obj, const char *new_name, bool use_undo)
{
  if (!obj || !new_name || !new_name[0])
    return;

  if (strcmp(obj->getName(), new_name) == 0)
    return;

  if (!obj->mayRename())
    return;

  String name(new_name);

  if (use_undo)
  {
    getUndoSystem()->begin();
    getUndoSystem()->put(new UndoObjectEditorRename(this, obj));

    // make name unique
    while (getObjectByName(name))
      ::make_next_numbered_name(name, suffixDigitsCount);
  }

  String oldName(obj->getName());
  obj->setName(name);

  for (int i = 0; i < objectCount(); ++i)
  {
    RenderableEditableObject *o = getObject(i);
    o->onObjectNameChange(obj, oldName, name);
  }

  if (use_undo)
    getUndoSystem()->accept("Rename");

  if (objectPropBar)
    objectPropBar->updateName(name);
}


void ObjectEditor::addButton(PropPanel::ContainerPropertyControl *tb, int id, const char *bmp_name, const char *hint, bool check)
{
  G_ASSERT(tb);

  check ? tb->createCheckBox(id, hint) : tb->createButton(id, hint);

  tb->setButtonPictures(id, bmp_name);
}


void ObjectEditor::enableButton(int id, bool state)
{
  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(toolBarId);
  if (tb)
    tb->setEnabledById(id, state);
}


void ObjectEditor::setButton(int id, bool state)
{
  if (toolBarId == -1)
  {
    debug("ObjectEditor::setButtonCheck(): toolbar id == -1!");
    return;
  }

  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(toolBarId);
  if (tb)
    tb->setBool(id, state);
}


void ObjectEditor::setRadioButton(int id, int value_id) { setButton(id, (value_id == id)); }


void ObjectEditor::updateToolbarButtons()
{
  setRadioButton(CM_OBJED_MODE_SELECT, editMode);
  setRadioButton(CM_OBJED_MODE_MOVE, editMode);
  setRadioButton(CM_OBJED_MODE_SURF_MOVE, editMode);
  setRadioButton(CM_OBJED_MODE_ROTATE, editMode);
  setRadioButton(CM_OBJED_MODE_SCALE, editMode);
}


ObjectEditorPropPanelBar *ObjectEditor::createEditorPropBar(void *handle)
{
  return new ObjectEditorPropPanelBar(this, handle, "Object Properties");
}


void ObjectEditor::saveEditorPropBarSettings()
{
  if (objectPropBar)
  {
    objectPropSettings.reset();
    objectPropBar->saveSettings(objectPropSettings);
  }
}


void *ObjectEditor::onWmCreateWindow(int type)
{
  switch (type)
  {
    case PROPBAR_EDITOR_WTYPE:
    {
      if (objectPropBar)
        return nullptr;

      objectPropBar = createEditorPropBar(nullptr);
      objectPropBar->fillPanel();
      objectPropBar->loadSettings(objectPropSettings);
      areObjectPropsValid = true;

      setButton(CM_OBJED_OBJPROP_PANEL, true);
      return objectPropBar;
    }
    break;
  }

  return nullptr;
}


bool ObjectEditor::onWmDestroyWindow(void *window)
{
  if (window == objectPropBar)
  {
    saveEditorPropBarSettings();
    del_it(objectPropBar);
    setButton(CM_OBJED_OBJPROP_PANEL, false);
    return true;
  }

  return false;
}


void ObjectEditor::initUi(int toolbar_id)
{
  if (toolbar_id == -1)
    return;

  if (toolBarId != -1)
  {
    debug("ObjectEditor::initUi(%d) - recall! already initialize with (%d)!", toolbar_id, toolBarId);
    return;
  }

  IWndManager *manager = EDITORCORE->getWndManager();
  G_ASSERT(manager);
  manager->registerWindowHandler(this);

  toolBarId = toolbar_id;
  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(toolbar_id);
  G_ASSERT(tb);

  tb->setEventHandler(this);
  tb->clear();
  fillToolBar(tb);

  setEditMode(CM_OBJED_MODE_SELECT);
}


void ObjectEditor::closeUi()
{
  if (toolBarId == -1)
    return;

  IWndManager *manager = EDITORCORE->getWndManager();
  G_ASSERT(manager);

  if (objectPropBar)
  {
    saveEditorPropBarSettings();
    EDITORCORE->removePropPanel(objectPropBar);
  }

  manager->unregisterWindowHandler(this);

  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(toolBarId);
  G_ASSERT(tb);

  tb->clear();
  tb->setEventHandler(NULL);

  toolBarId = -1;
}


void ObjectEditor::fillToolBar(PropPanel::ContainerPropertyControl *toolbar)
{
  PropPanel::ContainerPropertyControl *tb = toolbar->createToolbarPanel(0, "");

  addButton(tb, CM_OBJED_MODE_SELECT, "select", "Select (Q)", true);

  tb->createSeparator();

  addButton(tb, CM_OBJED_MODE_MOVE, "move", "Move (W). Use \"Shift\" for clone mode, \"Ctrl\" for snap points mode.", true);
  addButton(tb, CM_OBJED_MODE_SURF_MOVE, "move_on_surface", "Move over surface (Ctrl+Alt+W). Use \"Shift\" for clone mode.", true);
  addButton(tb, CM_OBJED_MODE_ROTATE, "rotate", "Rotate (E). Use \"Shift\" for clone mode.", true);
  addButton(tb, CM_OBJED_MODE_SCALE, "scale", "Scale (R). Use \"Shift\" for clone mode.", true);
  addButton(tb, CM_OBJED_DROP, "drop", "Drop object (Ctrl+Alt+D)");

  tb->createSeparator();

  addButton(tb, CM_OBJED_SELECT_BY_NAME, "select_by_name", "Select objects by name (H)");

  tb->createSeparator();

  addButton(tb, CM_OBJED_OBJPROP_PANEL, "show_panel", "Show/hide object props panel (P)", true);

  setButton(CM_OBJED_OBJPROP_PANEL, (bool)objectPropBar);

  tb->createEditBox(CM_OBJED_SELECT_FILTER, "", filterString);
  tb->setBool(CM_OBJED_SELECT_FILTER, filterString.length() > 0);
}


void ObjectEditor::showPanel()
{
  if (!objectPropBar)
    EDITORCORE->addPropPanel(PROPBAR_EDITOR_WTYPE, hdpi::_pxScaled(PROPBAR_WIDTH));
  else
    EDITORCORE->removePropPanel(objectPropBar);
}


PropPanel::ContainerPropertyControl *ObjectEditor::createPanelGroup(int pid)
{
  return isPanelShown() ? objectPropBar->createPanelGroup(pid) : nullptr;
}


void ObjectEditor::createPanelTransform(int mode)
{
  if (isPanelShown())
    objectPropBar->createPanelTransform(mode);
}


void ObjectEditor::loadPropPanelSettings(const DataBlock &settings)
{
  objectPropSettings.setFrom(&settings);
  if (objectPropBar)
    objectPropBar->loadSettings(objectPropSettings);
}


void ObjectEditor::savePropPanelSettings(DataBlock &settings)
{
  saveEditorPropBarSettings();
  settings.setFrom(&objectPropSettings);
}


void ObjectEditor::setCreateMode(IObjectCreator *creator_)
{
  del_it(creator);
  creator = creator_;
}


void ObjectEditor::setCreateBySampleMode(RenderableEditableObject *sample_)
{
  if (sample)
    removeObject(sample, false);

  // This must be before addObject, so sub-classes can check in addObject whether the added object is a sample object.
  sample = sample_;

  if (sample_)
    addObject(sample_, false);

  rotDy = 0;
  scaleDy = 0;
  createRot = 0;
  createScale = 1.0;
  justCreated = false;
  canTransformOnCreate = false;
}


void ObjectEditor::createObject(IObjectCreator *creator_)
{
  del_it(creator);
  setEditMode(CM_OBJED_MODE_SELECT);
}

void ObjectEditor::createObjectBySample(RenderableEditableObject *sample) {}

static String lastNameSample;
static String lastNumberedName;

bool ObjectEditor::setUniqName(RenderableEditableObject *o, const char *n)
{
  if (!o || !n || !*n)
    return true;

  RenderableEditableObject *o2 = getObjectByName(n);
  if (o2 == o)
    return true;

  if (!o2)
  {
    o->setName(n);
    return true;
  }

  String newName(n);
  if (!strcmp(n, lastNameSample.str()))
  {
    if (getObjectByName(lastNumberedName))
      newName = lastNumberedName;
  }
  else
    lastNameSample.setStr(n);

  while (getObjectByName(newName))
    ::make_next_numbered_name(newName, suffixDigitsCount);

  lastNumberedName = newName;
  o->setName(newName);
  return false;
}
