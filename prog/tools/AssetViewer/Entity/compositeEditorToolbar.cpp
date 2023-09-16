#include "compositeEditorToolbar.h"
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_interface.h>
#include <propPanel2/c_panel_base.h>

void CompositeEditorToolbar::initUi(ControlEventHandler &event_handler, int toolbar_id)
{
  if (toolBarId >= 0)
    return;

  G_ASSERT(toolbar_id >= 0);
  toolBarId = toolbar_id;
  PropertyContainerControlBase *panel = EDITORCORE->getCustomPanel(toolbar_id);
  G_ASSERT(panel);
  panel->setEventHandler(&event_handler);
  panel->clear();

  PropertyContainerControlBase *tb = panel->createToolbarPanel(0, "");
  G_ASSERT(tb);
  addCheckButton(*tb, CM_OBJED_MODE_SELECT, "select", "Select (Q)");
  tb->createSeparator();
  addCheckButton(*tb, CM_OBJED_MODE_MOVE, "move", "Move (W)");
  addCheckButton(*tb, CM_OBJED_MODE_ROTATE, "rotate", "Rotate (E)");
  addCheckButton(*tb, CM_OBJED_MODE_SCALE, "scale", "Scale (R)");
}

void CompositeEditorToolbar::closeUi()
{
  if (toolBarId < 0)
    return;

  PropertyContainerControlBase *panel = EDITORCORE->getCustomPanel(toolBarId);
  if (panel)
  {
    panel->clear();
    panel->setEventHandler(nullptr);
  }

  toolBarId = -1;
}

void CompositeEditorToolbar::updateToolbarButtons(bool canTransform)
{
  const IEditorCoreEngine::ModeType mode = IEditorCoreEngine::get()->getGizmoModeType();

  setButtonState(CM_OBJED_MODE_SELECT, mode == IEditorCoreEngine::ModeType::MODE_None, true);
  setButtonState(CM_OBJED_MODE_MOVE, mode == IEditorCoreEngine::ModeType::MODE_Move, canTransform);
  setButtonState(CM_OBJED_MODE_ROTATE, mode == IEditorCoreEngine::ModeType::MODE_Rotate, canTransform);
  setButtonState(CM_OBJED_MODE_SCALE, mode == IEditorCoreEngine::ModeType::MODE_Scale, canTransform);
}

void CompositeEditorToolbar::addCheckButton(PropertyContainerControlBase &tb, int id, const char *bmp_name, const char *hint)
{
  tb.createCheckBox(id, hint);
  tb.setButtonPictures(id, bmp_name);
}

void CompositeEditorToolbar::setButtonState(int id, bool checked, bool enabled)
{
  if (toolBarId < 0)
    return;

  PropertyContainerControlBase *tb = EDITORCORE->getCustomPanel(toolBarId);
  if (tb)
  {
    tb->setBool(id, checked);
    tb->setEnabledById(id, enabled);
  }
}
