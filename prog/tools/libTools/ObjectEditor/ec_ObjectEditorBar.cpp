// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/util/strUtil.h>

#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <EditorCore/ec_rendEdObject.h>
#include <EditorCore/ec_cm.h>

#include <debug/dag_debug.h>
#include <de3_interface.h>

enum
{
  ID_NAME = 10000,
  ID_COUNT,
  ID_CID,
  ID_CID_REFILL,

  ID_TRANSFORM_LOC = RenderableEditableObject::PID_TRANSFORM_GROUP + 1,
  ID_TRANSFORM_ROT = ID_TRANSFORM_LOC + 3,
  ID_TRANSFORM_SCA = ID_TRANSFORM_ROT + 3,
  ID_TRANSFORM_LAST = ID_TRANSFORM_SCA + 3
};

template <class T>
inline dag::Span<T *> mk_slice(PtrTab<T> &t)
{
  return make_span<T *>(reinterpret_cast<T **>(t.data()), t.size());
}

ObjectEditorPropPanelBar::ObjectEditorPropPanelBar(ObjectEditor *obj_ed, void *hwnd, const char *caption) :
  objEd(obj_ed), objects(midmem)
{
  IWndManager *manager = IEditorCoreEngine::get()->getWndManager();
  G_ASSERT(manager && "ObjectEditorPropPanelBar ctor: WndManager is NULL!");
  manager->setCaption(hwnd, caption);

  propPanel = IEditorCoreEngine::get()->createPropPanel(this, caption);
}


ObjectEditorPropPanelBar::~ObjectEditorPropPanelBar()
{
  if (objects.size())
    objects[0]->onPPClose(*propPanel, mk_slice(objects));

  IEditorCoreEngine::get()->deleteCustomPanel(propPanel);
  propPanel = NULL;
}


void ObjectEditorPropPanelBar::getObjects()
{
  objects.resize(objEd->selectedCount());

  for (int i = 0; i < objEd->selectedCount(); ++i)
    objects[i] = objEd->getSelected(i);
}


void ObjectEditorPropPanelBar::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == ID_NAME)
  {
    objEd->renameObject(objEd->getSelected(0), panel->getText(pcb_id).str());
  }
  else if (ID_TRANSFORM_LOC <= pcb_id && pcb_id < ID_TRANSFORM_LAST)
  {
    if (objEd->selectedCount() > 0)
    {
      if (pcb_id >= ID_TRANSFORM_SCA)
        onTransformChange(pcb_id, panel, CM_OBJED_MODE_SCALE);
      else if (pcb_id >= ID_TRANSFORM_ROT)
        onTransformChange(pcb_id, panel, CM_OBJED_MODE_ROTATE);
      else // if (pcb_id >= ID_TRANSFORM_LOC)
        onTransformChange(pcb_id, panel, CM_OBJED_MODE_MOVE);
    }
  }
  else if (objects.size())
  {
    objEd->getUndoSystem()->begin();
    objects[0]->onPPChange(pcb_id, true, *panel, mk_slice(objects));
    objEd->getUndoSystem()->accept("Params change");
  }
}


void ObjectEditorPropPanelBar::onTransformChange(int pcb_id, PropPanel::ContainerPropertyControl *panel, int mode)
{
  Point3 v = getObjectTransform(mode);

  int pid = getPidForEditMode(mode);
  const int PID_X = pid;
  const int PID_Y = pid + 1;
  const int PID_Z = pid + 2;
  if (pcb_id == PID_X)
    v.x = panel->getFloat(pcb_id);
  if (pcb_id == PID_Y)
    v.y = panel->getFloat(pcb_id);
  if (pcb_id == PID_Z)
    v.z = panel->getFloat(pcb_id);

  setObjectTransform(mode, v);
}


Point3 ObjectEditorPropPanelBar::getObjectTransform(int mode)
{
  Point3 v(0, 0, 0);
  switch (mode)
  {
    case CM_OBJED_MODE_MOVE: v = objEd->getPt(); break;
    case CM_OBJED_MODE_ROTATE:
      if (objEd->getRot(v))
        v = Point3(RadToDeg(v.x), RadToDeg(v.y), RadToDeg(v.z));
      break;
    case CM_OBJED_MODE_SCALE:
      v = Point3(100, 100, 100);
      if (objEd->getScl(v))
        v *= 100;
      break;
  }
  return v;
}


void ObjectEditorPropPanelBar::setObjectTransform(int mode, Point3 val)
{
  const bool updateViewportGizmo = objEd->getUpdateViewportGizmo();
  bool resetEdit = false;
  int editMode = objEd->getEditMode();
  if (editMode != mode)
  {
    resetEdit = true;
    objEd->setUpdateViewportGizmo(false);
    objEd->setEditMode(mode);
    objEd->updateGizmo();
  }

  objEd->gizmoStarted();

  Point3 pt;
  switch (mode)
  {
    case CM_OBJED_MODE_ROTATE:
      if (objEd->getRot(pt))
        objEd->changed(Point3(DegToRad(val.x) - pt.x, DegToRad(val.y) - pt.y, DegToRad(val.z) - pt.z));
      break;

    case CM_OBJED_MODE_SCALE:
      if (objEd->getScl(pt))
      {
        const Point3 scale(val / 100.0);
        objEd->changed(Point3(scale.x / pt.x, scale.y / pt.y, scale.z / pt.z));
      }
      break;

    case CM_OBJED_MODE_MOVE: objEd->changed(val - objEd->getPt()); break;
  }

  objEd->gizmoEnded(true);

  if (resetEdit)
  {
    objEd->setEditMode(editMode);
    objEd->updateGizmo();
    objEd->setUpdateViewportGizmo(updateViewportGizmo);
  }

  IEditorCoreEngine::get()->updateViewports();
  IEditorCoreEngine::get()->invalidateViewportCache();
}


void ObjectEditorPropPanelBar::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (objects.size())
  {
    objEd->getUndoSystem()->begin();
    objects[0]->onPPBtnPressed(pcb_id, *panel, mk_slice(objects));
    objEd->getUndoSystem()->accept("Params change");
  }
}


void ObjectEditorPropPanelBar::onPostEvent(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == ID_CID_REFILL)
    fillPanel();
}


void ObjectEditorPropPanelBar::refillPanel()
{
  if (propPanel)
    propPanel->setPostEvent(ID_CID_REFILL);
}


void ObjectEditorPropPanelBar::fillPanel()
{
  G_ASSERT(propPanel && "ObjectEditorPropPanelBar::fillPanel: ppanel is NULL!");

  if (objects.size() && objects[0].get() && propPanel->getById(ID_NAME))
    objects[0]->onPPClear(*propPanel, mk_slice(objects));

  clear_and_shrink(objects);

  propPanel->clear();

  PropPanel::ContainerPropertyControl *commonGrp =
    propPanel->createGroup(RenderableEditableObject::PID_COMMON_GROUP, "Common parameters");
  commonGrp->setBoolValue(false);

  commonGrp->createEditBox(ID_NAME, "Name:", "", false);

  if (objEd->selectedCount())
  {
    if (objEd->selectedCount() == 1)
    {
      commonGrp->setEnabledById(ID_NAME, true);
      commonGrp->setText(ID_NAME, objEd->getSelected(0)->getName());
    }
    else
      commonGrp->setText(ID_NAME, String(100, "%d objects selected", objEd->selectedCount()));

    getObjects();

    // propPanel->createEditInt(ID_COUNT, "Num objects:", objects.size(), false);

    if (objects.size())
    {
      dag::Span<RenderableEditableObject *> o = mk_slice(objects);
      DClassID commonClassId = objects[0]->getCommonClassId(o.data(), o.size());
      // propPanel->createEditBox(ID_CID, "Common CID:",
      //   String(32, "%08X", commonClassId.id).str(), false);
      objects[0]->fillProps(*propPanel, commonClassId, o);
    }
    else
      commonGrp->setEnabledById(ID_NAME, false);
  }
}


void ObjectEditorPropPanelBar::loadSettings(DataBlock &settings)
{
  if (propPanel)
    propPanel->loadState(settings);
}


void ObjectEditorPropPanelBar::saveSettings(DataBlock &settings) const
{
  if (propPanel)
    propPanel->saveState(settings);
}


PropPanel::ContainerPropertyControl *ObjectEditorPropPanelBar::createPanelGroup(int pid)
{
  if (!propPanel)
    return nullptr;

  PropPanel::ContainerPropertyControl *grp = nullptr;
  if (pid == RenderableEditableObject::PID_COMMON_GROUP)
  {
    grp = propPanel->createGroup(pid, "Common parameters");
    grp->setBoolValue(false);
  }
  else if (pid == RenderableEditableObject::PID_TRANSFORM_GROUP)
  {
    grp = propPanel->createGroup(pid, "Transformation");
    grp->setBoolValue(false);
  }
  else if (pid == RenderableEditableObject::PID_SEED_GROUP)
  {
    grp = propPanel->createGroup(pid, "Seed");
    grp->setBoolValue(false);
  }
  else
  {
    grp = propPanel->createGroup(pid, "");
  }
  return grp;
}


void ObjectEditorPropPanelBar::createPanelTransform(int mode)
{
  if (!propPanel)
    return;

  PropPanel::ContainerPropertyControl *transformGrp = propPanel->getContainerById(RenderableEditableObject::PID_TRANSFORM_GROUP);
  if (transformGrp)
  {
    int pid;
    int prec = 2;
    switch (mode)
    {
      case CM_OBJED_MODE_MOVE:
        pid = ID_TRANSFORM_LOC;
        transformGrp->createStatic(0, "Location:");
        break;
      case CM_OBJED_MODE_ROTATE:
        pid = ID_TRANSFORM_ROT;
        transformGrp->createStatic(0, "Rotation:");
        break;
      case CM_OBJED_MODE_SCALE:
        pid = ID_TRANSFORM_SCA;
        prec = 0;
        transformGrp->createStatic(0, "Scale:");
        break;
      default: G_ASSERT_FAIL("ObjectEditorPropPanelBar::createPanelTransform: unsupported mode!"); return;
    }
    transformGrp->createEditFloatWidthEx(pid, "x:", 0.0f, prec, true, true, false);
    transformGrp->createEditFloatWidthEx(pid + 1, "y:", 0.0f, prec, true, false, false);
    transformGrp->createEditFloatWidthEx(pid + 2, "z:", 0.0f, prec, true, false, false);
  }
}


void ObjectEditorPropPanelBar::updateName(const char *name)
{
  G_ASSERT(propPanel && "ObjectEditorPropPanelBar::updateName: ppanel is NULL!");

  SimpleString curName(propPanel->getText(ID_NAME));
  if (curName != name)
    propPanel->setText(ID_NAME, name);
}


void ObjectEditorPropPanelBar::updateTransform()
{
  if (propPanel)
  {
    if (objEd->selectedCount() > 0)
    {
      updateTransformPart(CM_OBJED_MODE_MOVE);
      updateTransformPart(CM_OBJED_MODE_ROTATE);
      updateTransformPart(CM_OBJED_MODE_SCALE);
    }
  }
}


void ObjectEditorPropPanelBar::updateTransformPart(int mode)
{
  G_ASSERT(propPanel && "ObjectEditorPropPanelBar::updateTransformPart: ppanel is NULL!");

  Point3 v = getObjectTransform(mode);

  int pid = getPidForEditMode(mode);
  const int PID_X = pid;
  const int PID_Y = pid + 1;
  const int PID_Z = pid + 2;

  if (propPanel->getFloat(PID_X) != v.x)
    propPanel->setFloat(PID_X, v.x);
  if (propPanel->getFloat(PID_Y) != v.y)
    propPanel->setFloat(PID_Y, v.y);
  if (propPanel->getFloat(PID_Z) != v.z)
    propPanel->setFloat(PID_Z, v.z);
}


int ObjectEditorPropPanelBar::getPidForEditMode(int mode)
{
  switch (mode)
  {
    case CM_OBJED_MODE_MOVE: return ID_TRANSFORM_LOC;
    case CM_OBJED_MODE_ROTATE: return ID_TRANSFORM_ROT;
    case CM_OBJED_MODE_SCALE: return ID_TRANSFORM_SCA;
  }

  G_ASSERT_FAIL("ObjectEditorPropPanelBar::getPidForEditMode: unsupported mode!");
  return ID_TRANSFORM_LOC;
}
