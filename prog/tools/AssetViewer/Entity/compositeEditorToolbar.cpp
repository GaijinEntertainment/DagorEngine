// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorToolbar.h"
#include "../av_cm.h"
#include "entity_cm.h"
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_editorCommandSystem.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/ec_interface.h>
#include <propPanel/control/container.h>

static constexpr int DEF_MOVE = int(IEditorCoreEngine::BASIS_World) | int(IEditorCoreEngine::CENTER_Pivot);
static constexpr int DEF_MOVE_SURF = int(IEditorCoreEngine::BASIS_World) | int(IEditorCoreEngine::CENTER_Pivot);
static constexpr int DEF_SCALE = int(IEditorCoreEngine::BASIS_Local) | int(IEditorCoreEngine::CENTER_Pivot);
static constexpr int DEF_ROTATE = int(IEditorCoreEngine::BASIS_World) | int(IEditorCoreEngine::CENTER_Pivot);

void CompositeEditorToolbar::initUi(PropPanel::ControlEventHandler &event_handler, IGizmoClient *gc, int toolbar_id)
{
  if (isInited())
    return;

  G_ASSERT(toolbar_id >= 0);
  toolBarId = toolbar_id;
  PropPanel::ContainerPropertyControl *panel = EDITORCORE->getCustomPanel(toolbar_id);
  G_ASSERT(panel);
  panel->setEventHandler(&event_handler);
  panel->clear();

  PropPanel::ContainerPropertyControl *tb = panel->createToolbarPanel();
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
  addButton(*tb, CM_OPTIONS_GRID, EditorCommandIds::VIEW_GRID_SETTINGS, "snap_settings", "Grid settings");
  updateSnapToolbarButtons();

  client = gc;
  if (client)
    availableTypes = client->getAvailableTypes();
  else
    availableTypes = 0;

  moveGizmo = DEF_MOVE;
  moveSurfGizmo = DEF_MOVE_SURF;
  scaleGizmo = DEF_SCALE;
  rotateGizmo = DEF_ROTATE;

  tb->createSeparator();

  addButton(*tb, CM_COMPOSITE_EDITOR_CREATE_NODE, EditorCommandIds::ENTITY_CREATE_NODE, "create_cmp_node", "Create node");

  Tab<String> temp(tmpmem);
  tb->createCombo(CM_GIZMO_BASIS, "", temp, 0);
  tb->createCombo(CM_GIZMO_CENTER, "", temp, 0);
}

void CompositeEditorToolbar::setGizmoClientType(IEditorCoreEngine::ModeType tp)
{
  // Save the current basis/center back to the per-mode variable before switching.
  // Only save if a value has actually been set (gizmoBasisType != -1); otherwise the
  // per-mode defaults set in initUi() are still correct and must not be overwritten.
  if (gizmoBasisType != -1)
  {
    switch (type)
    {
      case IEditorCoreEngine::MODE_Move: moveGizmo = int(getBasisType()) | int(getCenterType()); break;

      case IEditorCoreEngine::MODE_MoveSurface: moveSurfGizmo = int(getBasisType()) | int(getCenterType()); break;

      case IEditorCoreEngine::MODE_Scale: scaleGizmo = int(getBasisType()) | int(getCenterType()); break;

      case IEditorCoreEngine::MODE_Rotate: rotateGizmo = int(getBasisType()) | int(getCenterType()); break;

      case IEditorCoreEngine::MODE_None: break;
    }
  }

  type = tp;

  refillTypes(type != IEditorCoreEngine::MODE_None);

  if (availableTypes)
  {
    gizmoBasisType = -1;
    gizmoCenterType = -1;
  }

  switch (type)
  {
    case IEditorCoreEngine::MODE_Move:
      setGizmoBasisAndCenter(moveGizmo & IEditorCoreEngine::GIZMO_MASK_Basis, moveGizmo & IEditorCoreEngine::GIZMO_MASK_CENTER);
      break;

    case IEditorCoreEngine::MODE_MoveSurface:
      setGizmoBasisAndCenter(moveSurfGizmo & IEditorCoreEngine::GIZMO_MASK_Basis,
        moveSurfGizmo & IEditorCoreEngine::GIZMO_MASK_CENTER);
      break;

    case IEditorCoreEngine::MODE_Scale:
      setGizmoBasisAndCenter(scaleGizmo & IEditorCoreEngine::GIZMO_MASK_Basis, scaleGizmo & IEditorCoreEngine::GIZMO_MASK_CENTER);
      break;

    case IEditorCoreEngine::MODE_Rotate:
      setGizmoBasisAndCenter(rotateGizmo & IEditorCoreEngine::GIZMO_MASK_Basis, rotateGizmo & IEditorCoreEngine::GIZMO_MASK_CENTER);
      break;

    case IEditorCoreEngine::MODE_None: break; // to prevent the unhandled switch case error
  }
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

bool CompositeEditorToolbar::onChange(int pcb_id)
{
  PropPanel::ContainerPropertyControl *panel = EDITORCORE->getCustomPanel(toolBarId);
  if (!panel)
    return false;

  switch (pcb_id)
  {
    case CM_GIZMO_BASIS:
    {
      gizmoBasisType = getBasisTypeByName(panel->getText(CM_GIZMO_BASIS).str());
    }
      return true;
    case CM_GIZMO_CENTER:
    {
      // This toolbar has no CM_ROTATE_CENTER_AND_OBJ checkbox, so always use CENTER_Selection.
      gizmoCenterType = getCenterTypeByName(panel->getText(CM_GIZMO_CENTER).str(), true);
    }
      return true;
  }

  return false;
}

void CompositeEditorToolbar::updateGizmoToolbarButtons(bool canTransform)
{
  const IEditorCoreEngine::ModeType mode = IEditorCoreEngine::get()->getGizmoModeType();

  setButtonState(CM_OBJED_MODE_SELECT, mode == IEditorCoreEngine::ModeType::MODE_None, true);
  setButtonState(CM_OBJED_MODE_MOVE, mode == IEditorCoreEngine::ModeType::MODE_Move, canTransform);
  setButtonState(CM_OBJED_MODE_ROTATE, mode == IEditorCoreEngine::ModeType::MODE_Rotate, canTransform);
  setButtonState(CM_OBJED_MODE_SCALE, mode == IEditorCoreEngine::ModeType::MODE_Scale, canTransform);

  // Always rebuild the combo items since available types depend on the selected node
  // (e.g. BASIS_Parent only appears when the node has a parent).
  const bool showCombos = canTransform && mode != IEditorCoreEngine::ModeType::MODE_None;
  refillTypes(showCombos);
  if (showCombos)
  {
    // Restore the active selection after refillTypes reset the combo display.
    // Fall back to the per-mode default if no selection has been made yet.
    const int savedBasis = gizmoBasisType;
    const int savedCenter = gizmoCenterType;
    gizmoBasisType = -1;
    gizmoCenterType = -1;
    setGizmoBasisAndCenter(savedBasis != -1 ? savedBasis : int(getGizmoBasisTypeForMode(type)),
      savedCenter != -1 ? savedCenter : int(getGizmoCenterTypeForMode(type)));
  }
}

void CompositeEditorToolbar::refillTypes(bool canTransform)
{
  itemsBasis.clear();
  itemsCenter.clear();

  if (client)
    availableTypes = client->getAvailableTypes();
  else
    availableTypes = 0;

  if (availableTypes)
  {
    if (availableTypes & IEditorCoreEngine::BASIS_World)
      itemsBasis.push_back() = getBasisWorldCaption();

    if (availableTypes & IEditorCoreEngine::BASIS_Local)
      itemsBasis.push_back() = getBasisLocalCaption();

    if (availableTypes & IEditorCoreEngine::BASIS_Parent)
      itemsBasis.push_back() = getBasisParentCaption();

    if (availableTypes & IEditorCoreEngine::CENTER_Pivot)
      itemsCenter.push_back() = getCenterPivotCaption();

    if (availableTypes & IEditorCoreEngine::CENTER_Selection || availableTypes & IEditorCoreEngine::CENTER_SelectionNotRotObj)
      itemsCenter.push_back() = getCenterSelectionCaption();

    if (availableTypes & IEditorCoreEngine::CENTER_Coordinates)
      itemsCenter.push_back() = getCenterCoordCaption();
  }

  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(toolBarId);
  if (tb)
  {
    tb->setStrings(CM_GIZMO_BASIS, itemsBasis);
    tb->setEnabledById(CM_GIZMO_BASIS, canTransform);

    tb->setStrings(CM_GIZMO_CENTER, itemsCenter);
    tb->setEnabledById(CM_GIZMO_CENTER, canTransform);
  }
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

void CompositeEditorToolbar::addButton(PropPanel::ContainerPropertyControl &tb, int id, const char *editor_command_id,
  const char *bmp_name, const char *hint)
{
  IEditorCommandSystem *commandSystem = EDITORCORE->queryEditorInterface<IEditorCommandSystem>();
  G_ASSERT(commandSystem);
  commandSystem->createToolbarButton(tb, id, editor_command_id, hint);

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

IEditorCoreEngine::CenterType CompositeEditorToolbar::getCenterType() const
{
  if (gizmoCenterType != -1)
    return (IEditorCoreEngine::CenterType)gizmoCenterType;

  return IEditorCoreEngine::CENTER_None;
}

IEditorCoreEngine::CenterType CompositeEditorToolbar::getGizmoCenterTypeForMode(IEditorCoreEngine::ModeType tp) const
{
  if (type == tp)
  {
    if (gizmoCenterType != -1)
      return (IEditorCoreEngine::CenterType)gizmoCenterType;
  }

  switch (tp)
  {
    case IEditorCoreEngine::MODE_Move: return (IEditorCoreEngine::CenterType)(moveGizmo & IEditorCoreEngine::GIZMO_MASK_CENTER);

    case IEditorCoreEngine::MODE_MoveSurface:
      return (IEditorCoreEngine::CenterType)(moveSurfGizmo & IEditorCoreEngine::GIZMO_MASK_CENTER);

    case IEditorCoreEngine::MODE_Scale: return (IEditorCoreEngine::CenterType)(scaleGizmo & IEditorCoreEngine::GIZMO_MASK_CENTER);

    case IEditorCoreEngine::MODE_Rotate: return (IEditorCoreEngine::CenterType)(rotateGizmo & IEditorCoreEngine::GIZMO_MASK_CENTER);

    case IEditorCoreEngine::MODE_None: break; // to prevent the unhandled switch case error
  }

  return IEditorCoreEngine::CENTER_None;
}

IEditorCoreEngine::CenterType CompositeEditorToolbar::getCenterTypeByName(const char *name, bool enableRotObj) const
{
  if (!strcmp(name, getCenterPivotCaption()))
    return IEditorCoreEngine::CENTER_Pivot;
  else if (!strcmp(name, getCenterSelectionCaption()))
  {
    if (!enableRotObj)
      return IEditorCoreEngine::CENTER_SelectionNotRotObj;
    else
      return IEditorCoreEngine::CENTER_Selection;
  }
  else if (!strcmp(name, getCenterCoordCaption()))
    return IEditorCoreEngine::CENTER_Coordinates;

  return IEditorCoreEngine::CENTER_None;
}

const char *CompositeEditorToolbar::getCenterNameByType(IEditorCoreEngine::CenterType type) const
{
  switch (type)
  {
    case IEditorCoreEngine::CENTER_Pivot: return getCenterPivotCaption();
    case IEditorCoreEngine::CENTER_Selection:
    case IEditorCoreEngine::CENTER_SelectionNotRotObj: return getCenterSelectionCaption();
    case IEditorCoreEngine::CENTER_Coordinates: return getCenterCoordCaption();
    case IEditorCoreEngine::CENTER_None: break; // to prevent the unhandled switch case error
  }

  return NULL;
}

IEditorCoreEngine::BasisType CompositeEditorToolbar::getBasisType() const
{
  if (gizmoBasisType != -1)
    return (IEditorCoreEngine::BasisType)gizmoBasisType;

  return IEditorCoreEngine::BASIS_None;
}

IEditorCoreEngine::BasisType CompositeEditorToolbar::getGizmoBasisTypeForMode(IEditorCoreEngine::ModeType tp) const
{
  if (type == tp)
  {
    if (gizmoBasisType != -1)
      return (IEditorCoreEngine::BasisType)gizmoBasisType;
  }

  switch (tp)
  {
    case IEditorCoreEngine::MODE_Move: return (IEditorCoreEngine::BasisType)(moveGizmo & IEditorCoreEngine::GIZMO_MASK_Basis);

    case IEditorCoreEngine::MODE_MoveSurface:
      return (IEditorCoreEngine::BasisType)(moveSurfGizmo & IEditorCoreEngine::GIZMO_MASK_Basis);

    case IEditorCoreEngine::MODE_Scale: return (IEditorCoreEngine::BasisType)(scaleGizmo & IEditorCoreEngine::GIZMO_MASK_Basis);

    case IEditorCoreEngine::MODE_Rotate: return (IEditorCoreEngine::BasisType)(rotateGizmo & IEditorCoreEngine::GIZMO_MASK_Basis);

    case IEditorCoreEngine::MODE_None: break; // to prevent the unhandled switch case error
  }

  return IEditorCoreEngine::BASIS_None;
}

IEditorCoreEngine::BasisType CompositeEditorToolbar::getBasisTypeByName(const char *name) const
{
  if (!strcmp(name, getBasisWorldCaption()))
    return IEditorCoreEngine::BASIS_World;
  else if (!strcmp(name, getBasisLocalCaption()))
    return IEditorCoreEngine::BASIS_Local;
  else if (!strcmp(name, getBasisParentCaption()))
    return IEditorCoreEngine::BASIS_Parent;

  return IEditorCoreEngine::BASIS_None;
}

const char *CompositeEditorToolbar::getBasisNameByType(IEditorCoreEngine::BasisType type) const
{
  switch (type)
  {
    case IEditorCoreEngine::BASIS_World: return getBasisWorldCaption();
    case IEditorCoreEngine::BASIS_Local: return getBasisLocalCaption();
    case IEditorCoreEngine::BASIS_Parent: return getBasisParentCaption();
    case IEditorCoreEngine::BASIS_None: break; // to prevent the unhandled switch case error
  }

  return NULL;
}

void CompositeEditorToolbar::setGizmoBasisAndCenter(int basis, int center)
{
  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(toolBarId);
  if (!tb)
    return;

  if (gizmoBasisType != basis)
  {
    gizmoBasisType = basis;
    const char *basisName = getBasisNameByType((IEditorCoreEngine::BasisType)basis);
    if (!basisName)
    {
      tb->setInt(CM_GIZMO_BASIS, -1);
    }
    else
    {
      int ind = 0;
      for (int i = 0; i < itemsBasis.size(); ++i)
        if (itemsBasis[i] == basisName)
        {
          ind = i;
          break;
        }

      tb->setInt(CM_GIZMO_BASIS, ind);
      onChange(CM_GIZMO_BASIS);
    }
  }

  if (gizmoCenterType != center)
  {
    gizmoCenterType = center;
    const char *centerName = getCenterNameByType((IEditorCoreEngine::CenterType)center);
    if (!centerName)
    {
      tb->setInt(CM_GIZMO_CENTER, -1);
    }
    else
    {
      int ind = 0;
      for (int i = 0; i < itemsCenter.size(); ++i)
        if (itemsCenter[i] == centerName)
        {
          ind = i;
          break;
        }

      tb->setInt(CM_GIZMO_CENTER, ind);
      onChange(CM_GIZMO_CENTER);
    }
  }
}
