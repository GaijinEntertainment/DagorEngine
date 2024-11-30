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
  else if (objects.size())
  {
    objEd->getUndoSystem()->begin();
    objects[0]->onPPChange(pcb_id, true, *panel, mk_slice(objects));
    objEd->getUndoSystem()->accept("Params change");
  }
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


void ObjectEditorPropPanelBar::updateName(const char *name)
{
  G_ASSERT(propPanel && "ObjectEditorPropPanelBar::fillPanel: ppanel is NULL!");

  SimpleString curName(propPanel->getText(ID_NAME));
  if (curName != name)
    propPanel->setText(ID_NAME, name);
}
