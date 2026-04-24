// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorToolbar.h"
#include "../av_cm.h"
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/ec_interface.h>
#include <propPanel/control/container.h>

void CompositeEditorToolbar::initUi(PropPanel::ControlEventHandler &event_handler, int toolbar_id)
{
  if (isInited())
    return;

  G_ASSERT(toolbar_id >= 0);
  toolBarId = toolbar_id;
  PropPanel::ContainerPropertyControl *panel = EDITORCORE->getCustomPanel(toolbar_id);
  G_ASSERT(panel);
  panel->setEventHandler(&event_handler);
  panel->clear();

  PropPanel::ContainerPropertyControl *tb = panel->createToolbarPanel(0, "");
  G_ASSERT(tb);
  addCheckButton(*tb, CM_OBJED_MODE_SELECT, EditorCommandIds::OBJED_MODE_SELECT, "select", "Select");
  tb->createSeparator();
  addCheckButton(*tb, CM_OBJED_MODE_MOVE, EditorCommandIds::OBJED_MODE_MOVE, "move", "Move");
  addCheckButton(*tb, CM_OBJED_MODE_ROTATE, EditorCommandIds::OBJED_MODE_ROTATE, "rotate", "Rotate");
  addCheckButton(*tb, CM_OBJED_MODE_SCALE, EditorCommandIds::OBJED_MODE_SCALE, "scale", "Scale");

  tb->createSeparator();
  addCheckButton(*tb, CM_VIEW_GRID_MOVE_SNAP, EditorCommandIds::VIEW_GRID_MOVE_SNAP, "snap_move", "Move snap");
  addCheckButton(*tb, CM_VIEW_GRID_ANGLE_SNAP, EditorCommandIds::VIEW_GRID_ANGLE_SNAP, "snap_rotate", "Rotate snap");
  addCheckButton(*tb, CM_VIEW_GRID_SCALE_SNAP, EditorCommandIds::VIEW_GRID_SCALE_SNAP, "snap_scale", "Scale snap");
  updateSnapToolbarButtons();
}

void CompositeEditorToolbar::closeUi()
{
  if (!isInited())
    return;

  PropPanel::ContainerPropertyControl *panel = EDITORCORE->getCustomPanel(toolBarId);
  if (panel)
  {
    panel->clear();
    panel->setEventHandler(nullptr);
  }

  toolBarId = -1;
}

bool CompositeEditorToolbar::isInited() const { return toolBarId >= 0; }

void CompositeEditorToolbar::updateGizmoToolbarButtons(bool canTransform)
{
  const IEditorCoreEngine::ModeType mode = IEditorCoreEngine::get()->getGizmoModeType();

  setButtonState(CM_OBJED_MODE_SELECT, mode == IEditorCoreEngine::ModeType::MODE_None, true);
  setButtonState(CM_OBJED_MODE_MOVE, mode == IEditorCoreEngine::ModeType::MODE_Move, canTransform);
  setButtonState(CM_OBJED_MODE_ROTATE, mode == IEditorCoreEngine::ModeType::MODE_Rotate, canTransform);
  setButtonState(CM_OBJED_MODE_SCALE, mode == IEditorCoreEngine::ModeType::MODE_Scale, canTransform);
}

void CompositeEditorToolbar::updateSnapToolbarButtons()
{
  GridObject &grid = IEditorCoreEngine::get()->getGrid();

  setButtonState(CM_VIEW_GRID_MOVE_SNAP, grid.getMoveSnap(), true);
  setButtonState(CM_VIEW_GRID_ANGLE_SNAP, grid.getRotateSnap(), true);
  setButtonState(CM_VIEW_GRID_SCALE_SNAP, grid.getScaleSnap(), true);
}

void CompositeEditorToolbar::addCheckButton(PropPanel::ContainerPropertyControl &tb, int id, const char *editor_command_id,
  const char *bmp_name, const char *hint)
{
  IEditorCommandSystem *commandSystem = EDITORCORE->queryEditorInterface<IEditorCommandSystem>();
  G_ASSERT(commandSystem);
  commandSystem->createToolbarToggleButton(tb, id, editor_command_id, hint);

  tb.setButtonPictures(id, bmp_name);
}

void CompositeEditorToolbar::setButtonState(int id, bool checked, bool enabled)
{
  if (toolBarId < 0)
    return;

  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(toolBarId);
  if (tb)
  {
    tb->setBool(id, checked);
    tb->setEnabledById(id, enabled);
  }
}
