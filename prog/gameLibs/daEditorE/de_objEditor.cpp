// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daEditorE/de_objEditor.h>
#include <daEditorE/de_interface.h>
#include <daEditorE/de_objCreator.h>
#include <generic/dag_sort.h>
#include <libTools/util/strUtil.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiKeybData.h>
#include <drv/hid/dag_hiPointingData.h>
#include <math/dag_mathAng.h>
#include <debug/dag_debug.h>
#include <util/dag_convar.h>


CONSOLE_FLOAT_VAL("daEd4", debug_boxes_max_distance, 1000);

#define MAX_TRACE_DIST 1000

#define ROTATE_AND_SCALE_DIVIZER 100.0
#define MIN_SCALE                0.05
#define SAFE_TRASHOLD            15

ObjectEditor::~ObjectEditor()
{
  sample = NULL;
  del_it(creator);

  // closeUi();
  DAEDITOR4.setGizmo(NULL, DAEDITOR4.MODE_none, NULL);
}

EditableObject *ObjectEditor::getObjectByName(const char *name) const
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


bool ObjectEditor::getSelectionBoxFocused(BBox3 &box) const
{
  if (!selection.size())
    return false;

  bool gotBox = false;

  for (int i = 0; i < selection.size(); ++i)
  {
    if (selectionFocus && selection[i] != selectionFocus)
      continue;

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
}


void ObjectEditor::selectAll()
{
  DAEDITOR4.undoSys.begin();
  unselectAll();
  for (int i = 0; i < objects.size(); ++i)
  {
    EditableObject *o = objects[i];
    if (!canSelectObj(o))
      continue;
    o->selectObject();
  }
  updateGizmo();
  DAEDITOR4.undoSys.accept("Select");
}


void ObjectEditor::unselectAll()
{
  for (int i = 0; i < objects.size(); ++i)
    objects[i]->selectObject(false);
}


void ObjectEditor::update(float dt)
{
  updateGizmo();
  updateObjectProps();

  for (int i = 0; i < objects.size(); ++i)
  {
    if (objects[i]->isValid())
      objects[i]->update(dt);
    else
      removeObject(objects[i], false);
  }

  if (creator && creator->isFinished())
  {
    if (creator->isOk())
      createObject(creator);
    else
    {
      del_it(creator);
      setEditMode(CM_OBJED_MODE_SELECT);
    }
  }
}


void ObjectEditor::removeInvalidObjects()
{
  for (int i = 0; i < objects.size(); ++i)
  {
    if (!objects[i]->isValid())
      removeObject(objects[i], false);
  }
}


void ObjectEditor::beforeRender()
{
  for (int i = 0; i < objects.size(); ++i)
    objects[i]->beforeRender();
}


void ObjectEditor::render(const Frustum &frustum, const Point3 &camera_pos)
{
  for (int i = 0; i < objects.size(); ++i)
    if (!objects[i]->isHidden())
    {
      BBox3 bbox;
      if (!objects[i]->getWorldBox(bbox))
        bbox = objects[i]->getLbb();
      const float distanceSq = (camera_pos - bbox.center()).length();
      if (debug_boxes_max_distance.get() > 0 && debug_boxes_max_distance.get() < distanceSq)
        continue;
      if (frustum.testBoxB(bbox))
        objects[i]->render();
    }
  if (creator)
    creator->render();
  if (editMode == CM_OBJED_MODE_POINT_ACTION)
    renderPointActionPreview();
}


void ObjectEditor::renderTrans()
{
  for (int i = 0; i < objects.size(); ++i)
    if (!objects[i]->isHidden())
      objects[i]->renderTrans();
}


void ObjectEditor::updateSelection()
{
  selection.clear();
  selectionFocus = nullptr;

  for (int i = 0; i < objects.size(); ++i)
  {
    EditableObject *o = objects[i];
    if (o->isSelected())
      selection.push_back(o);
  }

  selectionChanged();
}


void ObjectEditor::setSelectionFocus(EditableObject *obj)
{
  selectionFocus = nullptr;

  for (int i = 0; i < selection.size(); ++i)
  {
    EditableObject *o = selection[i];
    if (o->isSelected() && o == obj)
    {
      selectionFocus = obj;
      break;
    }
  }
}


void ObjectEditor::onObjectFlagsChange(EditableObject *obj, int changed_flags)
{
  if (changed_flags & EditableObject::FLG_SELECTED)
  {
    if (obj->isSelected())
    {
      selection.push_back(obj);
      selectionFocus = nullptr;
      selectionChanged();
    }
    else
    {
      for (int i = 0; i < selection.size(); ++i)
        if (selection[i] == obj)
        {
          erase_items(selection, i, 1);
          selectionFocus = nullptr;
          selectionChanged();
          break;
        }
    }
  }
}


void ObjectEditor::onObjectGeomChange(EditableObject *obj)
{
  G_UNUSED(obj);
  if (!isGizmoStarted)
    isGizmoValid = false;
}


void ObjectEditor::_addObjects(EditableObject **obj, int num, bool use_undo)
{
  if (num <= 0)
    return;

  UndoAddObjects *undo = NULL;
  if (use_undo)
    undo = new (midmem) UndoAddObjects(this, num);

  objects.reserve(num);

  for (int i = 0; i < num; ++i)
  {
    EditableObject *o = obj[i];
    if (!o)
      continue;

    int j;
    for (j = 0; j < objects.size(); ++j)
      if (objects[j] == o)
        break;

    if (j < objects.size())
      continue;

    // G_ASSERT(!o->objEditor);
    o->onAdd(this);
    o->objEditor = this;

    objects.push_back(o);

    if (undo)
      undo->objects.push_back(o);

    if (o->isSelected())
    {
      selection.push_back(o);
      selectionFocus = nullptr;
      selectionChanged();
    }
  }

  if (undo)
  {
    undo->objects.shrink_to_fit();
    DAEDITOR4.undoSys.put(undo);
  }
}


void ObjectEditor::_removeObjects(EditableObject **obj, int num, bool use_undo)
{
  if (num <= 0)
    return;

  UndoRemoveObjects *undo = NULL;
  if (use_undo)
    undo = new (midmem) UndoRemoveObjects(this, num);

  for (int i = 0; i < num; ++i)
  {
    Ptr<EditableObject> o = obj[i];
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

    o->onRemove(this);

    if (undo)
      undo->objects.push_back(o);

    for (j = 0; j < selection.size(); ++j)
      if (selection[j] == o)
      {
        erase_items(selection, j, 1);
        selectionFocus = nullptr;
        selectionChanged();
        break;
      }
  }

  if (undo)
  {
    undo->objects.shrink_to_fit();
    DAEDITOR4.undoSys.put(undo);
  }
}


void ObjectEditor::removeAllObjects(bool use_undo)
{
  if (!objects.size())
    return;
  PtrTab<EditableObject> objs(tmpmem);
  objs = objects;

  // PVS says: An odd explicit type casting: (EditableObject * *) & objs[0]. Consider verifying it.
  removeObjects((EditableObject **)&objs[0], objs.size(), use_undo); // -V580
}


void ObjectEditor::deleteSelectedObjects(bool use_undo)
{
  Tab<EditableObject *> list(tmpmem);
  list.reserve(selection.size());

  DAEDITOR4.undoSys.begin();

  for (int i = 0; i < selection.size(); ++i)
    if (selection[i]->mayDelete())
      list.push_back(selection[i]);

  if (!list.size())
  {
    DAEDITOR4.undoSys.cancel();
    return;
  }
  fillDeleteDepsObjects(list);
  for (int i = 0; i < list.size(); ++i)
    list[i]->selectObject(false);

  removeObjects(&list[0], list.size(), use_undo);
  DAEDITOR4.undoSys.accept("Delete");
}


UndoSystem *ObjectEditor::getUndoSystem() { return &DAEDITOR4.undoSys; }


bool ObjectEditor::checkObjSelFilter(EditableObject &obj)
{
  if (filterString.length())
  {
    String objName(obj.getName());
    return strstr(objName.toLower().str(), filterString.toLower().str());
  }

  return true;
}


bool ObjectEditor::canSelectObj(EditableObject *o) { return o && !o->isHidden() && !o->isFrozen() && checkObjSelFilter(*o); }


EditableObject *ObjectEditor::pickObject(int x, int y)
{
  Tab<EditableObject *> objs;
  if (pickObjects(x, y, objs))
    return objs[0];
  return NULL;
}

Point2 ObjectEditor::cmp_obj_z_pt;
int ObjectEditor::cmp_obj_z(EditableObject *const *a, EditableObject *const *b)
{
  Point3 pt_a = a[0]->getPos();
  Point3 pt_b = b[0]->getPos();

  Point3 tmp;
  Point3 dir;
  DAEDITOR4.clientToWorld(cmp_obj_z_pt, tmp, dir);

  BBox3 box;
  if (a[0]->getWorldBox(box))
    pt_a += dir * length(box.width()) / 2;
  if (b[0]->getWorldBox(box))
    pt_b += dir * length(box.width()) / 2;

  Point2 scr_a, scr_b;
  float a_z = 0, b_z = 0;
  if (!DAEDITOR4.worldToClient(pt_a, scr_a, &a_z))
    a_z = 100000;
  else if (a_z < 0)
    a_z = 100000 - a_z;
  if (!DAEDITOR4.worldToClient(pt_b, scr_b, &b_z))
    b_z = 100000;
  else if (b_z < 0)
    b_z = 100000 - b_z;

  return (a_z < b_z) ? 1 : (a_z > b_z ? -1 : 0);
}

bool ObjectEditor::pickObjects(int x, int y, Tab<EditableObject *> &objs)
{
  bool picked = false;

  for (int i = 0; i < objects.size(); ++i)
  {
    EditableObject *o = objects[i];
    if (!canSelectObj(o))
      continue;

    if (!o->isSelectedByPointClick(x, y))
      continue;

    objs.push_back(o);
    picked = true;
  }
  if (objs.size() > 1)
  {
    cmp_obj_z_pt.set((real)x, (real)y);
    sort(objs, &cmp_obj_z);
  }
  return picked;
}


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
  gizmoScl = gizmoScl / float(selection.size());
  p = gizmoScl;
  return true;
}


bool ObjectEditor::getAxes(Point3 &ax, Point3 &ay, Point3 &az)
{
  if (DAEDITOR4.getGizmoBasisType() == IDaEditor4Engine::BASIS_local && selection.size() > 0)
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


void ObjectEditor::changed(const Point3 &delta)
{
  if (editMode != CM_OBJED_MODE_MOVE && editMode != CM_OBJED_MODE_SURF_MOVE)
  {
    if (DAEDITOR4.undoSys.is_holding())
      DAEDITOR4.undoSys.cancel();
    DAEDITOR4.undoSys.begin();
  }

  IDaEditor4Engine::BasisType basis = DAEDITOR4.getGizmoBasisType();

  Point3 origin = gizmoOrigin;

  switch (editMode)
  {
    case CM_OBJED_MODE_MOVE:
      for (int i = 0; i < selection.size(); ++i)
        selection[i]->moveObject(delta, basis);
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

  if (DAEDITOR4.isShiftKeyPressed() && editMode == CM_OBJED_MODE_MOVE && canCloneSelection())
  {
    cloneMode = true;
    cloneDelta = getPt();
  }

  DAEDITOR4.undoSys.begin();
  if (cloneMode)
  {
    clear_and_shrink(cloneObjs);
    cloneSelection();
  }

  if (!cloneMode)
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
  if (apply && cloneMode && selection.size())
  {
    if (int cloneCount = getCloneCount())
    {
      const Point3 delta = getPt() - cloneDelta;

      for (int cloneId = 2; cloneId <= cloneCount; ++cloneId)
      {
        for (int i = 0; i < selection.size(); ++i)
          setUniqName(selection[i], cloneName.empty() ? selection[i]->getName() : cloneName);

        cloneDelta = getPt();
        cloneSelection();
        for (int i = 0; i < selection.size(); ++i)
          selection[i]->moveObject(delta, IDaEditor4Engine::BASIS_world);
      }

      for (int i = 0; i < selection.size(); ++i)
        setUniqName(selection[i], cloneName.empty() ? selection[i]->getName() : cloneName);
      getUndoSystem()->accept("Clone object(s)");
    }
    else
    {
      for (int i = 0; i < cloneObjs.size(); ++i)
        removeObject(cloneObjs[i]);
      getUndoSystem()->cancel();
    }

    cloneMode = false;

    gizmoRotO = gizmoRot;
    gizmoSclO = gizmoScl;
  }
  else if (apply)
  {
    const char *name = "Gizmo Operation";

    switch (editMode)
    {
      case CM_OBJED_MODE_MOVE: name = "Move"; break;
      case CM_OBJED_MODE_SURF_MOVE: name = "Move over surface"; break;
      case CM_OBJED_MODE_ROTATE: name = "Rotate"; break;
      case CM_OBJED_MODE_SCALE: name = "Scale"; break;
    }

    DAEDITOR4.undoSys.accept(name);
    gizmoRotO = gizmoRot;
    gizmoSclO = gizmoScl;
  }
  else if (DAEDITOR4.undoSys.is_holding())
    DAEDITOR4.undoSys.cancel();

  isGizmoStarted = false;
  gizmoRotPoint = Point3(0, 0, 0);
  updateGizmo();
}


bool ObjectEditor::canStartChangeAt(int x, int y, int gizmo_sel)
{
  G_UNUSED(gizmo_sel);
  Tab<EditableObject *> objs(tmpmem);
  if (!pickObjects(x, y, objs))
    return false;

  EditableObject *obj;
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

  if (obj->objFlags & EditableObject::FLG_WANTRESELECT)
    obj->selectObject();
  return true;
}


int ObjectEditor::getAvailableTypes()
{
  int types = 0;

  switch (editMode)
  {
    case CM_OBJED_MODE_ROTATE:
    case CM_OBJED_MODE_MOVE:
    case CM_OBJED_MODE_SURF_MOVE: types |= IDaEditor4Engine::BASIS_world;
    case CM_OBJED_MODE_SCALE: types |= IDaEditor4Engine::BASIS_local; break;
  }

  if (types)
    types |= IDaEditor4Engine::CENTER_pivot | IDaEditor4Engine::CENTER_sel;

  return types;
}


void ObjectEditor::updateGizmo(int basis)
{
  if (isGizmoValid)
    return;

  if (basis == IDaEditor4Engine::BASIS_none)
    basis = DAEDITOR4.getGizmoBasisType();

  IDaEditor4Engine::ModeType gt;

  switch (editMode)
  {
    case CM_OBJED_MODE_MOVE: gt = IDaEditor4Engine::MODE_move; break;
    case CM_OBJED_MODE_SURF_MOVE: gt = IDaEditor4Engine::MODE_movesurf; break;
    case CM_OBJED_MODE_ROTATE: gt = IDaEditor4Engine::MODE_rotate; break;
    case CM_OBJED_MODE_SCALE: gt = IDaEditor4Engine::MODE_scale; break;
    default: gt = IDaEditor4Engine::MODE_none;
  }

  // calc gizmoPt
  if (selection.size())
  {
    switch (DAEDITOR4.getGizmoCenterType())
    {
      case IDaEditor4Engine::CENTER_pivot:
        if (editMode != CM_OBJED_MODE_SURF_MOVE)
          gizmoPt = selection[0]->getPos();
        else
          gizmoPt = selection[0]->getPos() - Point3(0, selection[0]->getSurfaceDist(), 0);
        break;

      case IDaEditor4Engine::CENTER_coord:
        if (basis == IDaEditor4Engine::BASIS_local)
          gizmoPt = selection[0]->getPos();
        else
          gizmoPt = Point3(0, 0, 0);
        break;

      case IDaEditor4Engine::CENTER_sel:
      {
        BBox3 box;
        getSelectionBox(box);

        if (editMode == CM_OBJED_MODE_ROTATE && basis == IDaEditor4Engine::BASIS_world)
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

      default: break;
    }
  }
  else
    gt = IDaEditor4Engine::MODE_none;

  if (DAEDITOR4.getGizmoModeType() != (gt & DAEDITOR4.GIZMO_MASK_Mode))
  {
    EditableObject *o = selection.empty() ? NULL : selection[0].get();
    DAEDITOR4.setGizmo(gt == DAEDITOR4.MODE_none ? NULL : this, gt, o);
  }
}


Point3 ObjectEditor::getSurfMoveGizmoPos(const Point3 &obj_pos) const
{
  float dist = MAX_TRACE_DIST;
  EditableObject *exclude = selection.empty() ? NULL : selection[0].get();
  if (DAEDITOR4.traceRayEx(obj_pos + Point3(0, 0.01, 0), Point3(0, -1, 0), dist, exclude))
    return obj_pos - Point3(0, dist - 0.01, 0);

  return obj_pos;
}


void ObjectEditor::setWorkMode(const char *mode)
{
  workMode = mode;

  updateToolbarButtons();
}


void ObjectEditor::setEditMode(int mode)
{
  if (editMode == CM_OBJED_MODE_POINT_ACTION)
    performPointAction(false, 0, 0, "finish");

  editMode = mode;
  isGizmoValid = false;

  if (mode == CM_OBJED_MODE_SURF_MOVE)
    for (int i = 0; i < selection.size(); ++i)
      selection[i]->rememberSurfaceDist();

  if (mode == CM_OBJED_MODE_POINT_ACTION)
    unselectAll();

  updateToolbarButtons();
}


void ObjectEditor::invalidateObjectProps() { areObjectPropsValid = false; }


void ObjectEditor::dropObjects()
{
  DAEDITOR4.undoSys.begin();

  const Point3 dir = Point3(0, -1, 0);

  for (int i = 0; i < selection.size(); ++i)
  {
    EditableObject *obj = selection[i];

    if (obj)
    {
      float dist = MAX_TRACE_DIST;
      const Point3 pos = obj->getPos() + Point3(0, 0.2, 0);

      if (DAEDITOR4.traceRayEx(pos, dir, dist, obj))
      {
        obj->putMoveUndo();
        obj->setPos(pos + dir * dist);
      }
    }
  }

  DAEDITOR4.undoSys.accept("Drop object(s)");
}

void ObjectEditor::dropObjectsNorm()
{
  DAEDITOR4.undoSys.begin();

  for (int i = 0; i < selection.size(); ++i)
  {
    EditableObject *obj = selection[i];

    if (obj)
    {
      float dist = MAX_TRACE_DIST;
      const Point3 up = obj->getMatrix().getcol(1);
      const Point3 dir = -up;
      const Point3 pos = obj->getPos() + up * 0.2f;

      Point3 norm = up;
      if (DAEDITOR4.traceRayEx(pos, dir, dist, obj, &norm))
      {
        obj->putRotateUndo();
        obj->setPos(pos + dir * dist);
        Matrix3 rm = obj->getMatrix();
        const Point3 scale(rm.getcol(0).length(), rm.getcol(1).length(), rm.getcol(2).length());

        rm.setcol(2, normalize(rm.getcol(0) % norm));
        rm.setcol(1, normalize(norm));
        rm.setcol(0, normalize(norm % rm.getcol(2)));

        rm.setcol(0, rm.getcol(0) * scale.x);
        rm.setcol(1, rm.getcol(1) * scale.y);
        rm.setcol(2, rm.getcol(2) * scale.z);

        obj->setMatrix(rm);
      }
    }
  }

  DAEDITOR4.undoSys.accept("Drop object(s) on normal");
}

void ObjectEditor::resetScale()
{
  DAEDITOR4.undoSys.begin();

  bool anySized = false;
  for (int i = 0; i < selection.size(); ++i)
  {
    EditableObject *obj = selection[i];
    if (obj)
    {
      const float sizedEpsilon = 0.0001f;

      Matrix3 rm = obj->getMatrix();
      if (fabsf(rm.col[0].length() - 1.0f) > sizedEpsilon || fabsf(rm.col[1].length() - 1.0f) > sizedEpsilon ||
          fabsf(rm.col[2].length() - 1.0f) > sizedEpsilon)
      {
        anySized = true;
      }
    }
  }

  for (int i = 0; i < selection.size(); ++i)
  {
    EditableObject *obj = selection[i];
    if (obj)
    {
      obj->putScaleUndo();

      if (anySized)
      {
        Matrix3 rm = obj->getMatrix();
        rm.setcol(0, normalize(rm.getcol(0)));
        rm.setcol(1, normalize(rm.getcol(1)));
        rm.setcol(2, normalize(rm.getcol(2)));
        obj->setMatrix(rm);
      }
      else
      {
        Matrix3 rm = obj->getMatrix();
        rm.setcol(0, normalize(cross(rm.getcol(1), rm.getcol(2))));
        rm.setcol(1, normalize(rm.getcol(1)));
        rm.setcol(2, normalize(cross(rm.getcol(0), rm.getcol(1))));
        obj->setMatrix(rm);
      }
    }
  }

  DAEDITOR4.undoSys.accept("Reset scale");
}

void ObjectEditor::resetRotate()
{
  DAEDITOR4.undoSys.begin();

  bool anyLeaning = false;
  for (int i = 0; i < selection.size(); ++i)
  {
    EditableObject *obj = selection[i];
    if (obj)
    {
      Matrix3 rm = obj->getMatrix();
      if (rm.col[0].y != 0.0f || rm.col[2].y != 0.0f)
        anyLeaning = true;
    }
  }

  for (int i = 0; i < selection.size(); ++i)
  {
    EditableObject *obj = selection[i];
    if (obj)
    {
      obj->putRotateUndo();

      Matrix3 rm = obj->getMatrix();
      const float sx = rm.getcol(0).length();
      const float sy = rm.getcol(1).length();
      const float sz = rm.getcol(2).length();

      if (anyLeaning)
      {
        // remove leaning only
        rm.setcol(0, sx * normalize(Point3::x0z(rm.getcol(0))));
        if (rm.getcol(0).length() < sx * 0.99f)
          rm.setcol(0, sx, 0.0f, 0.0f);
        rm.setcol(1, 0.0f, sy, 0.0f);
        rm.setcol(2, sz * normalize(cross(rm.getcol(0), rm.getcol(1))));
      }
      else
      {
        // full rotation reset (axis-aligned)
        rm.setcol(0, sx, 0.0f, 0.0f);
        rm.setcol(1, 0.0f, sy, 0.0f);
        rm.setcol(2, 0.0f, 0.0f, sz);
      }
      obj->setMatrix(rm);
    }
  }

  DAEDITOR4.undoSys.accept("Reset rotate");
}

void ObjectEditor::onClick(int pcb_id, PropertyContainerControlBase *panel)
{
  G_UNUSED(panel);
  switch (pcb_id)
  {
    case CM_OBJED_MODE_SELECT:
    case CM_OBJED_MODE_MOVE:
    case CM_OBJED_MODE_SURF_MOVE:
    case CM_OBJED_MODE_ROTATE:
    case CM_OBJED_MODE_SCALE: setEditMode(pcb_id); break;

    case CM_OBJED_DROP: dropObjects(); break;

    case CM_OBJED_DROP_NORM: dropObjectsNorm(); break;

    case CM_OBJED_DELETE: deleteSelectedObjects(); break;

    case CM_OBJED_RESET_SCALE: resetScale(); break;

    case CM_OBJED_RESET_ROTATE: resetRotate(); break;
  }
}

void ObjectEditor::handleKeyRelease(int dkey, int modif)
{
  G_UNUSED(modif);
  G_UNUSED(dkey);
  bool isAlt = DAEDITOR4.isAltKeyPressed();

  if (!isAlt)
    DAEDITOR4.setRotateSnap(false);
}

void ObjectEditor::handleKeyPress(int dkey, int modif)
{
  G_UNUSED(modif);
  bool isAlt = DAEDITOR4.isAltKeyPressed();
  bool isCtrl = DAEDITOR4.isCtrlKeyPressed();
  bool isShift = DAEDITOR4.isShiftKeyPressed();

  if (!isAlt && !isCtrl && !isShift)
  {
    switch (dkey)
    {
      case HumanInput::DKEY_Q: onClick(CM_OBJED_MODE_SELECT, NULL); break;
      case HumanInput::DKEY_W:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
          break;
        onClick(CM_OBJED_MODE_MOVE, NULL);
        break;
      case HumanInput::DKEY_E:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
          break;
        onClick(CM_OBJED_MODE_ROTATE, NULL);
        break;
      case HumanInput::DKEY_R:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
          break;
        onClick(CM_OBJED_MODE_SCALE, NULL);
        break;
      case HumanInput::DKEY_DELETE:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
        {
          performPointAction(true, mouseX, mouseY, "delete");
          break;
        }
        onClick(CM_OBJED_DELETE, NULL);
        break;
      case HumanInput::DKEY_Z:
      {
        BBox3 box;
        if (getSelectionBoxFocused(box))
          DAEDITOR4.zoomAndCenter(box);
      }
      break;

      case HumanInput::DKEY_X:
        if (isGizmoStarted)
          break;
#define BASIS(X) IDaEditor4Engine::BASIS_##X
        switch (editMode)
        {
          case CM_OBJED_MODE_MOVE:
          case CM_OBJED_MODE_SURF_MOVE:
          case CM_OBJED_MODE_ROTATE:
          case CM_OBJED_MODE_SCALE:
            switch (DAEDITOR4.getGizmoBasisType())
            {
              case BASIS(none):
              case BASIS(world):
                DAEDITOR4.setGizmoBasisType(BASIS(local));
                updateGizmo();
                break;
              case BASIS(local):
                DAEDITOR4.setGizmoBasisType(BASIS(world));
                updateGizmo();
                break;
            }
            break;
        }
#undef BASIS
        break;

      case HumanInput::DKEY_C:
        if (isGizmoStarted)
          break;
#define CENTER(X) IDaEditor4Engine::CENTER_##X
        switch (editMode)
        {
          case CM_OBJED_MODE_MOVE:
          case CM_OBJED_MODE_SURF_MOVE:
          case CM_OBJED_MODE_ROTATE:
          case CM_OBJED_MODE_SCALE:
            switch (DAEDITOR4.getGizmoCenterType())
            {
              case CENTER(none):
              case CENTER(coord):
              case CENTER(pivot):
                DAEDITOR4.setGizmoCenterType(CENTER(sel));
                updateGizmo();
                break;
              case CENTER(sel):
                DAEDITOR4.setGizmoCenterType(CENTER(pivot));
                updateGizmo();
                break;
            }
            break;
        }
#undef CENTER
        break;
    }
  }

  else if (isCtrl && !isShift && isAlt)
  {
    switch (dkey)
    {
      case HumanInput::DKEY_D:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
          break;
        onClick(CM_OBJED_DROP, NULL);
        break;

      case HumanInput::DKEY_E:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
          break;
        onClick(CM_OBJED_DROP_NORM, NULL);
        break;

      case HumanInput::DKEY_W:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
          break;
        onClick(CM_OBJED_MODE_SURF_MOVE, NULL);
        break;

      case HumanInput::DKEY_R:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
          break;
        onClick(CM_OBJED_RESET_SCALE, NULL);
        break;

      case HumanInput::DKEY_T: onClick(CM_OBJED_RESET_ROTATE, NULL); break;
    }
  }

  else if (isCtrl && !isShift && !isAlt)
  {
    switch (dkey)
    {
      case HumanInput::DKEY_A:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
          break;
        selectAll();
        break;

      case HumanInput::DKEY_D:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
          break;
        unselectAll();
        break;

      case HumanInput::DKEY_Z:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
        {
          performPointAction(false, 0, 0, "undo");
          break;
        }
        DAEDITOR4.undoSys.undo();
        break;

      case HumanInput::DKEY_Y:
        if (editMode == CM_OBJED_MODE_POINT_ACTION)
        {
          performPointAction(false, 0, 0, "redo");
          break;
        }
        DAEDITOR4.undoSys.redo();
        break;
    }
  }

  if (isAlt)
    DAEDITOR4.setRotateSnap(true);
}


bool ObjectEditor::handleMouseLBPress(int x, int y, bool inside, int buttons, int key_modif)
{
  if (creator)
    return creator->handleMouseLBPress(x, y, inside, buttons, key_modif);
  if (!inside)
    return false;

  if (sample)
  {
    justCreated = true;
    return true;
  }

  if (editMode == CM_OBJED_MODE_POINT_ACTION)
  {
    performPointAction(true, x, y, "action");
    return true;
  }

  Tab<EditableObject *> objs(tmpmem);
  if (pickObjects(x, y, objs))
  {
    DAEDITOR4.undoSys.begin();

    if (DAEDITOR4.isAltKeyPressed())
    {
      // unselect first selected
      for (int i = 0; i < objs.size(); ++i)
        if (objs[i]->isSelected())
        {
          objs[i]->selectObject(false);
          break;
        }
    }
    else if (DAEDITOR4.isCtrlKeyPressed())
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
    else if (objs.size() && objs[0]->isSelected() && (objs[0]->objFlags & EditableObject::FLG_WANTRESELECT))
      objs[0]->selectObject();

    DAEDITOR4.undoSys.accept("select");

    updateGizmo();

    if (editMode != CM_OBJED_MODE_SELECT)
    {
      int i;
      for (i = 0; i < objs.size(); ++i)
        if (objs[i]->isSelected())
          break;

      if (i < objs.size())
        DAEDITOR4.startGizmo(x, y, inside, buttons, key_modif);
    }
    else
      DAEDITOR4.startRectangularSelection(x, y, key_modif);
  }
  else
    DAEDITOR4.startRectangularSelection(x, y, key_modif);

  return true;
}


bool ObjectEditor::handleMouseLBRelease(int x, int y, bool inside, int buttons, int key_modif)
{
  if (creator)
    return creator->handleMouseLBRelease(x, y, inside, buttons, key_modif);
  IBBox2 rect;
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

  if (DAEDITOR4.endRectangularSelection(&rect, &type))
  {
    const bool isCrtlAltHeld = (type & (HumanInput::KeyboardRawState::CTRL_BITS | HumanInput::KeyboardRawState::ALT_BITS));
    if (rect.width().x > 3 || rect.width().y > 3)
    {
      DAEDITOR4.undoSys.begin();

      for (int i = 0; i < objects.size(); ++i)
      {
        EditableObject *obj = objects[i];
        if (!canSelectObj(obj))
          continue;
        if (obj->isSelectedByRectangle(rect))
        {
          // hit
          if (type & HumanInput::KeyboardRawState::ALT_BITS) // Alt
            obj->selectObject(false);
          else if (type & HumanInput::KeyboardRawState::CTRL_BITS) // Ctrl
          {
            if (type & HumanInput::KeyboardRawState::SHIFT_BITS)
              obj->selectObject(!obj->isSelected());
            else
              obj->selectObject(true);
          }
          else
            obj->selectObject();
        }
        else
        {
          // missed
          if (type & HumanInput::KeyboardRawState::ALT_BITS) // Alt
          {}
          else if (type & HumanInput::KeyboardRawState::CTRL_BITS) // Ctrl
          {}
          else
            obj->selectObject(false);
        }
      }

      DAEDITOR4.undoSys.accept("Select");

      updateGizmo();
    }
    else if (editMode == CM_OBJED_MODE_ROTATE && isCrtlAltHeld)
    {
      // Rotate selected objects to look at a point under the mouse cursor

      Point3 dir, world;
      float dist = MAX_TRACE_DIST;

      DAEDITOR4.clientToWorld(Point2(x, y), world, dir);

      if (DAEDITOR4.traceRay(world, dir, dist))
      {
        const Point3 lookAtPos = world + dir * dist;
        for (EditableObject *obj : selection)
        {
          const Quat lookAtQuat = dir_to_quat(lookAtPos - obj->getPos());
          const TMatrix lookAtTm = makeTM(lookAtQuat);
          obj->putRotateUndo();
          obj->setRotTm(lookAtTm);
        }
      }
    }
    else if (!isCrtlAltHeld)
    {
      EditableObject *obj = pickObject((rect.left() + rect.right() + 1) / 2, (rect.top() + rect.bottom() + 1) / 2);

      if (!obj)
      {
        DAEDITOR4.undoSys.begin();
        unselectAll();
        DAEDITOR4.undoSys.accept("Select");

        updateGizmo();
      }
    }
  }

  return true;
}


bool ObjectEditor::handleMouseRBRelease(int x, int y, bool inside, int buttons, int key_modif)
{
  G_UNUSED(x);
  G_UNUSED(y);
  G_UNUSED(inside);
  G_UNUSED(buttons);
  G_UNUSED(key_modif);

  IBBox2 rect;
  int type;

  DAEDITOR4.endRectangularSelection(&rect, &type);

  return true;
}


bool ObjectEditor::handleMouseRBPress(int x, int y, bool inside, int buttons, int key_modif)
{
  G_UNUSED(x);
  G_UNUSED(y);
  G_UNUSED(inside);
  G_UNUSED(buttons);
  G_UNUSED(key_modif);

  if (cancelCreateMode())
    return true;

  if (editMode == CM_OBJED_MODE_POINT_ACTION)
  {
    performPointAction(true, x, y, "context");
    return true;
  }

  return false;
}


bool ObjectEditor::handleMouseMove(int x, int y, bool inside, int buttons, int key_modif)
{
  mouseX = x;
  mouseY = y;

  if (creator)
    return creator->handleMouseMove(x, y, inside, buttons, key_modif, false);

  if (sample)
  {
    if (justCreated)
    {
      bool doTransform = false;
      switch (buttons)
      {
        // rotate
        case HumanInput::PointingRawState::Mouse::LBUTTON_BIT:
          rotDy += lastY - y;

          if (!canTransformOnCreate && (rotDy > SAFE_TRASHOLD || rotDy < -SAFE_TRASHOLD))
          {
            rotDy = 0;
            canTransformOnCreate = true;
          }

          lastY = y;
          createRot = rotDy / ROTATE_AND_SCALE_DIVIZER;

          if (DAEDITOR4.getRotateSnap())
            createRot = DAEDITOR4.snapToAngle(Point3(0, createRot, 0)).y;

          doTransform = true;
          break;

        // scale
        case HumanInput::PointingRawState::Mouse::LBUTTON_BIT | HumanInput::PointingRawState::Mouse::RBUTTON_BIT:
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

          if (DAEDITOR4.getScaleSnap())
            createScale = DAEDITOR4.snapToAngle(Point3(createScale, createScale, createScale)).x;

          doTransform = true;
          break;
      }

      if (doTransform && canTransformOnCreate)
      {
        TMatrix tm;

        tm.setcol(0, Point3(createScale, 0, 0));
        tm.setcol(1, Point3(0, createScale, 0));
        tm.setcol(2, Point3(0, 0, createScale));

        tm *= rotyTM(createRot);

        tm.setcol(3, sample->getPos());
        sample->setWtm(tm);
      }
    }
    else
    {
      Point3 world;
      Point3 dir;
      float dist = MAX_TRACE_DIST;
      Point2 mouse = Point2(x, y);

      DAEDITOR4.clientToWorld(mouse, world, dir);

      if (DAEDITOR4.traceRayEx(world, dir, dist, sample))
      {
        const Point3 worldPos = world + dir * dist;
        sample->setPos(DAEDITOR4.getMoveSnap() ? DAEDITOR4.snapToGrid(worldPos) : worldPos);
      }
      lastY = mouse.y;
    }
    return true;
  }

  if (inside)
  {
    // EditableObject *obj = pickObject(x, y);
    return true;
  }

  return false;
}


void ObjectEditor::getObjNames(Tab<SimpleString> &names, Tab<SimpleString> &sel_names, const Tab<int> &types)
{
  G_UNUSED(types);
  for (int i = 0; i < objects.size(); ++i)
  {
    EditableObject *o = objects[i];
    if (!canSelectObj(o))
      continue;

    names.push_back() = o->getName();
    if (o->isSelected())
      sel_names.push_back() = o->getName();
  }
}


void ObjectEditor::onSelectedNames(const Tab<SimpleString> &names)
{
  DAEDITOR4.undoSys.begin();

  unselectAll();
  for (int i = 0; i < names.size(); ++i)
  {
    EditableObject *o = getObjectByName(names[i]);
    if (o == nullptr || !canSelectObj(o))
      continue;
    o->selectObject();
  }

  DAEDITOR4.undoSys.accept("Select");

  updateGizmo();
}


void ObjectEditor::renameObject(EditableObject *obj, const char *new_name, bool use_undo)
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
    DAEDITOR4.undoSys.begin();
    DAEDITOR4.undoSys.put(new UndoObjectEditorRename(this, obj));

    // make name unique
    while (getObjectByName(name))
      ::make_next_numbered_name(name, suffixDigitsCount);
  }

  String oldName(obj->getName());
  obj->setName(name);

  for (int i = 0; i < objectCount(); ++i)
  {
    EditableObject *o = getObject(i);
    o->onObjectNameChange(obj, oldName, name);
  }

  if (use_undo)
    DAEDITOR4.undoSys.accept("Rename");

  /*
    if (objectPropBar)
      objectPropBar->updateName(name);
  */
}

EditableObject *ObjectEditor::cloneObject(EditableObject *o, bool use_undo)
{
  EditableObject *cloneObj = o->cloneObject();
  if (cloneObj)
  {
    addObject(cloneObj, use_undo);
    cloneObjs.push_back(cloneObj);
  }
  return cloneObj;
}

void ObjectEditor::cloneSelection()
{
  PtrTab<EditableObject> oldSel(selection);
  unselectAll();

  for (int i = 0; i < oldSel.size(); ++i)
    if (EditableObject *o = cloneObject(oldSel[i], true /*use_undo*/))
      o->selectObject();
}

void ObjectEditor::setCreateMode(IObjectCreator *creator_)
{
  del_it(creator);
  creator = creator_;
}


void ObjectEditor::setCreateBySampleMode(EditableObject *sample_)
{
  if (sample)
    removeObject(sample, false);

  if (sample_)
  {
    addObject(sample_, false);
    sample_->selectObject(true);
  }

  sample = sample_;

  rotDy = 0;
  scaleDy = 0;
  createRot = 0;
  createScale = 1.0;
  justCreated = false;
  canTransformOnCreate = false;
}


void ObjectEditor::createObject(IObjectCreator *creator_)
{
  G_UNUSED(creator_);
  del_it(creator);
  setEditMode(CM_OBJED_MODE_SELECT);
}

void ObjectEditor::createObjectBySample(EditableObject *sample_) { G_UNUSED(sample_); }

bool ObjectEditor::cancelCreateMode()
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

void ObjectEditor::performPointAction(bool has_pos, int x, int y, const char *op)
{
  Point3 world(0, 0, 0);
  Point3 dir(0, 0, 0);
  Point3 pos(0, 0, 0);

  if (has_pos)
  {
    float dist = MAX_TRACE_DIST;
    Point2 mouse = Point2(x, y);

    DAEDITOR4.clientToWorld(mouse, world, dir);

    if (DAEDITOR4.traceRay(world, dir, dist))
      pos = world + dir * dist;
    else
      has_pos = false;
  }

  DAEDITOR4.performPointAction(has_pos, world, dir, pos, op);
}

bool ObjectEditor::setUniqName(EditableObject *o, const char *n)
{
  if (!o || !n || !*n)
    return true;

  EditableObject *o2 = getObjectByName(n);
  if (o2 == o)
    return true;

  if (!o2)
  {
    o->setName(n);
    return true;
  }

  String newName(n);
  while (getObjectByName(newName))
    ::make_next_numbered_name(newName, suffixDigitsCount);

  o->setName(newName);
  return false;
}

void ObjectEditor::setPointActionPreview(const char *shape, float param)
{
  pointActionPreviewShape = -1;
  pointActionPreviewParam = param;

  if (shape && *shape)
    pointActionPreviewShape = parsePointActionPreviewShape(shape);
}

void ObjectEditor::renderPointActionPreview()
{
  if (pointActionPreviewShape < 0)
    return;

  Point3 world(0, 0, 0);
  Point3 dir(0, 0, 0);
  Point3 pos(0, 0, 0);

  float dist = MAX_TRACE_DIST;
  Point2 mouse = Point2(mouseX, mouseY);
  DAEDITOR4.clientToWorld(mouse, world, dir);
  if (!DAEDITOR4.traceRay(world, dir, dist))
    return;
  pos = world + dir * dist;

  renderPointActionPreviewShape(pointActionPreviewShape, pos, pointActionPreviewParam);
}
