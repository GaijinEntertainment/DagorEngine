// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_status_bar.h>
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/ec_camera_elem.h>

#include <propPanel/control/container.h>

#include <debug/dag_debug.h>

#include <coolConsole/coolConsole.h>


#define DEF_MOVE      IEditorCoreEngine::BASIS_World | IEditorCoreEngine::CENTER_Pivot
#define DEF_MOVE_SURF IEditorCoreEngine::BASIS_World | IEditorCoreEngine::CENTER_Pivot
#define DEF_SCALE     IEditorCoreEngine::BASIS_Local | IEditorCoreEngine::CENTER_Pivot
#define DEF_ROTATE    IEditorCoreEngine::BASIS_World | IEditorCoreEngine::CENTER_Pivot

const float SPIN_CHANGE_SENSE = 1e-4;


//==============================================================================
ToolBarManager::ToolBarManager() :
  tbId(-1),
  availableTypes(0),
  client(NULL),
  controlsInserted(false),
  isEnabled(false),
  moveGizmo(DEF_MOVE),
  moveSurfGizmo(DEF_MOVE_SURF),
  scaleGizmo(DEF_SCALE),
  rotateGizmo(DEF_ROTATE),
  isEnabledBtnRotateCenterAndObj(true),
  itemsBasis(midmem),
  itemsCenter(midmem)
{
  gizmoBasisType = IEditorCoreEngine::BASIS_None;
  gizmoCenterType = IEditorCoreEngine::CENTER_None;
}


//==============================================================================
void ToolBarManager::init(int toolbar_id)
{
  tbId = toolbar_id;
  PropPanel::ContainerPropertyControl *tool = EDITORCORE->getCustomPanel(tbId);
  G_ASSERT(tool);

  PropPanel::ContainerPropertyControl *tb1 = tool->createToolbarPanel(0, "");

  Tab<String> temp(tmpmem);
  tb1->createCombo(CM_GIZMO_BASIS, "", temp, 0);
  tb1->createCombo(CM_GIZMO_CENTER, "", temp, 0);

  tb1->createSeparator();

  tb1->createCheckBox(CM_ROTATE_CENTER_AND_OBJ, "Rotate");
  tb1->setButtonPictures(CM_ROTATE_CENTER_AND_OBJ, "rotate");
  tb1->setBool(CM_ROTATE_CENTER_AND_OBJ, true);
  tb1->createSeparator();

  tb1->createEditFloat(CM_GIZMO_X, "x :");
  tb1->createEditFloat(CM_GIZMO_Y, "y :");
  tb1->createEditFloat(CM_GIZMO_Z, "z :");

  PropPanel::ContainerPropertyControl *tb2 = tool->createToolbarPanel(0, "");
  tb2->createSeparator();

  tb2->createCheckBox(CM_VIEW_GRID_MOVE_SNAP, "Move snap (S)");
  tb2->setButtonPictures(CM_VIEW_GRID_MOVE_SNAP, "snap_move");
  tb2->createCheckBox(CM_VIEW_GRID_ANGLE_SNAP, "Rotate snap (A)");
  tb2->setButtonPictures(CM_VIEW_GRID_ANGLE_SNAP, "snap_rotate");
  tb2->createCheckBox(CM_VIEW_GRID_SCALE_SNAP, "Scale snap (Shift + 5)");
  tb2->setButtonPictures(CM_VIEW_GRID_SCALE_SNAP, "snap_scale");
  tb2->createSeparator();

  GridObject &grid = IEditorCoreEngine::get()->getGrid();
  tb2->setBool(CM_VIEW_GRID_MOVE_SNAP, grid.getMoveSnap());
  tb2->setBool(CM_VIEW_GRID_ANGLE_SNAP, grid.getRotateSnap());
  tb2->setBool(CM_VIEW_GRID_SCALE_SNAP, grid.getScaleSnap());

  tb2->createButton(CM_ZOOM_AND_CENTER, "Zoom and center (Z or Ctrl+Shift+Z)");
  tb2->setButtonPictures(CM_ZOOM_AND_CENTER, "zoom_and_center");

  tb2->createButton(CM_NAVIGATE, "Navigate");
  tb2->setButtonPictures(CM_NAVIGATE, "navigate");
  tb2->createSeparator();

  tb2->createRadio(CM_CAMERAS_FREE, "Fly mode (Spacebar)");
  tb2->setButtonPictures(CM_CAMERAS_FREE, "freecam");
  tb2->createRadio(CM_CAMERAS_FPS, "FPS mode (Ctrl+Spacebar)");
  tb2->setButtonPictures(CM_CAMERAS_FPS, "fpscam");
  tb2->createRadio(CM_CAMERAS_TPS, "TPS mode (Ctrl+Shift+Spacebar)");
  tb2->setButtonPictures(CM_CAMERAS_TPS, "tpscam");
  tb2->createRadio(CM_CAMERAS_CAR, "Car mode (Shift+Spacebar)");
  tb2->setButtonPictures(CM_CAMERAS_CAR, "asset_vehicle");
  tb2->createSeparator();

  tb2->createButton(CM_STATS, "Show level statistics");
  tb2->setButtonPictures(CM_STATS, "stats");
  tb2->createSeparator();

  tb2->createButton(CM_CHANGE_VIEWPORT, "Viewport layout 1/4 (Alt+W)");
  tb2->setButtonPictures(CM_CHANGE_VIEWPORT, "change_viewport");

  tb2->createButton(CM_CHANGE_FOV, "Change FOV");
  tb2->setButtonPictures(CM_CHANGE_FOV, "change_fov");
  tb2->createSeparator();

  tb2->createButton(CM_CREATE_SCREENSHOT, "Create screenshot (Ctrl+P)");
  tb2->setButtonPictures(CM_CREATE_SCREENSHOT, "screenshot");

  tb2->createButton(CM_CREATE_CUBE_SCREENSHOT, "Create cube screenshot (Ctrl+Shift+P)");
  tb2->setButtonPictures(CM_CREATE_CUBE_SCREENSHOT, "screenshot_cube");
  tb2->createSeparator();

  tb2->createButton(CM_ENVIRONMENT_SETTINGS, "Environment settings");
  tb2->setButtonPictures(CM_ENVIRONMENT_SETTINGS, "environment_settings");
  tb2->createSeparator();

  tb2->createButton(CM_CONSOLE, "Show/hide console");
  tb2->setButtonPictures(CM_CONSOLE, "console");

  setEnabled(false);

  controlsInserted = true;
}


//==============================================================================
void ToolBarManager::setEnabled(bool enable)
{
  PropPanel::ContainerPropertyControl *tool = EDITORCORE->getCustomPanel(tbId);
  if (!tool)
    return;

  isEnabled = enable;

  tool->setEnabledById(CM_GIZMO_X, enable);
  tool->setEnabledById(CM_GIZMO_Y, enable);
  tool->setEnabledById(CM_GIZMO_Z, enable);

  tool->setEnabledById(CM_GIZMO_BASIS, enable);
  tool->setEnabledById(CM_GIZMO_CENTER, enable);
}


//==============================================================================
IEditorCoreEngine::CenterType ToolBarManager::getCenterType() const
{
  if (gizmoCenterType != -1)
    return (IEditorCoreEngine::CenterType)gizmoCenterType;

  return IEditorCoreEngine::CENTER_None;
}


IEditorCoreEngine::CenterType ToolBarManager::getCenterTypeByName(const char *name, bool enableRotObj) const
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


const char *ToolBarManager::getCenterNameByType(IEditorCoreEngine::CenterType type) const
{
  switch (type)
  {
    case IEditorCoreEngine::CENTER_Pivot: return getCenterPivotCaption();
    case IEditorCoreEngine::CENTER_Selection:
    case IEditorCoreEngine::CENTER_SelectionNotRotObj: return getCenterSelectionCaption();
    case IEditorCoreEngine::CENTER_Coordinates: return getCenterCoordCaption();
  }

  return NULL;
}


//==============================================================================
IEditorCoreEngine::BasisType ToolBarManager::getBasisType() const
{
  if (gizmoBasisType != -1)
    return (IEditorCoreEngine::BasisType)gizmoBasisType;

  return IEditorCoreEngine::BASIS_None;
}


IEditorCoreEngine::BasisType ToolBarManager::getGizmoBasisTypeForMode(IEditorCoreEngine::ModeType tp) const
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
  }

  return IEditorCoreEngine::BASIS_None;
}


IEditorCoreEngine::BasisType ToolBarManager::getBasisTypeByName(const char *name) const
{
  if (!strcmp(name, getBasisWorldCaption()))
    return IEditorCoreEngine::BASIS_World;
  else if (!strcmp(name, getBasisLocalCaption()))
    return IEditorCoreEngine::BASIS_Local;

  return IEditorCoreEngine::BASIS_None;
}


const char *ToolBarManager::getBasisNameByType(IEditorCoreEngine::BasisType type) const
{
  switch (type)
  {
    case IEditorCoreEngine::BASIS_World: return getBasisWorldCaption();
    case IEditorCoreEngine::BASIS_Local: return getBasisLocalCaption();
  }

  return NULL;
}


//==============================================================================
void ToolBarManager::setGizmoClient(IGizmoClient *gc, IEditorCoreEngine::ModeType tp)
{
  switch (type)
  {
    case IEditorCoreEngine::MODE_Move: moveGizmo = getBasisType() | getCenterType(); break;

    case IEditorCoreEngine::MODE_MoveSurface: moveSurfGizmo = getBasisType() | getCenterType(); break;

    case IEditorCoreEngine::MODE_Scale: scaleGizmo = getBasisType() | getCenterType(); break;

    case IEditorCoreEngine::MODE_Rotate: rotateGizmo = getBasisType() | getCenterType(); break;
  }

  client = gc;
  type = tp;

  if (client)
  {
    availableTypes = client->getAvailableTypes();
    gizmoPos = getClientGizmoValue();
    setGizmoToToolbar(gizmoPos);
  }
  else
    availableTypes = 0;

  refillTypes();

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
  }
}


void ToolBarManager::act()
{
  if (!controlsInserted)
    return;

  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(tbId);
  if (!tb)
    return;

  Point3 v = getClientGizmoValue();
  bool doEnable = true;
  bool enableRotObj = false;

  if (client)
  {
    switch (type & IEditorCoreEngine::GIZMO_MASK_Mode)
    {
      case IEditorCoreEngine::MODE_Rotate:
        if ((getCenterType() == IEditorCoreEngine::CENTER_Selection ||
              getCenterType() == IEditorCoreEngine::CENTER_SelectionNotRotObj) &&
            availableTypes & IEditorCoreEngine::CENTER_SelectionNotRotObj)
          enableRotObj = true;
        break;

      case IEditorCoreEngine::MODE_Scale:
      case IEditorCoreEngine::MODE_Move: break;

      default: doEnable = false; break;
    }
  }

  if (isEnabledBtnRotateCenterAndObj != enableRotObj)
  {
    isEnabledBtnRotateCenterAndObj = enableRotObj;
    tb->setEnabledById(CM_ROTATE_CENTER_AND_OBJ, enableRotObj);
  }

  if (!client || !doEnable)
  {
    if (isEnabled)
      setEnabled(false);
  }
  else
  {
    if (!isEnabled)
      setEnabled(true);
  }

  if (fabsf(gizmoPos.x - v.x) > SPIN_CHANGE_SENSE)
  {
    gizmoPos.x = v.x;
    tb->setFloat(CM_GIZMO_X, v.x);
  }
  if (fabsf(gizmoPos.y - v.y) > SPIN_CHANGE_SENSE)
  {
    gizmoPos.y = v.y;
    tb->setFloat(CM_GIZMO_Y, v.y);
  }
  if (fabsf(gizmoPos.z - v.z) > SPIN_CHANGE_SENSE)
  {
    gizmoPos.z = v.z;
    tb->setFloat(CM_GIZMO_Z, v.z);
  }

  if (!client && ((type & IEditorCoreEngine::GIZMO_MASK_Mode) == IEditorCoreEngine::MODE_None))
  {
    IGenViewportWnd *wp = EDITORCORE->getCurrentViewport();
    if (wp)
    {
      TMatrix tm;
      wp->getCameraTransform(tm);
      setGizmoToToolbar(tm.getcol(3));
    }
  }
}


void ToolBarManager::setMoveSnap()
{
  GridObject &grid = IEditorCoreEngine::get()->getGrid();
  bool press = !grid.getMoveSnap();

  grid.setMoveSnap(press);

  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(tbId);
  if (tb)
    tb->setBool(CM_VIEW_GRID_MOVE_SNAP, press);
}


void ToolBarManager::setScaleSnap()
{
  GridObject &grid = IEditorCoreEngine::get()->getGrid();
  bool press = !grid.getScaleSnap();

  grid.setScaleSnap(press);

  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(tbId);
  if (tb)
    tb->setBool(CM_VIEW_GRID_SCALE_SNAP, press);
}


void ToolBarManager::setRotateSnap()
{
  GridObject &grid = IEditorCoreEngine::get()->getGrid();
  bool press = !grid.getRotateSnap();

  grid.setRotateSnap(press);

  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(tbId);
  if (tb)
    tb->setBool(CM_VIEW_GRID_ANGLE_SNAP, press);
}


//==============================================================================
void ToolBarManager::refillTypes()
{
  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(tbId);
  if (!tb || !controlsInserted)
    return;

  itemsBasis.clear();
  itemsCenter.clear();

  if (availableTypes)
  {
    if (availableTypes & IEditorCoreEngine::BASIS_World)
      itemsBasis.push_back() = getBasisWorldCaption();

    if (availableTypes & IEditorCoreEngine::BASIS_Local)
      itemsBasis.push_back() = getBasisLocalCaption();

    if (availableTypes & IEditorCoreEngine::CENTER_Pivot)
      itemsCenter.push_back() = getCenterPivotCaption();

    if (availableTypes & IEditorCoreEngine::CENTER_Selection || availableTypes & IEditorCoreEngine::CENTER_SelectionNotRotObj)
      itemsCenter.push_back() = getCenterSelectionCaption();

    if (availableTypes & IEditorCoreEngine::CENTER_Coordinates)
      itemsCenter.push_back() = getCenterCoordCaption();

    setEnabled(true);
  }
  else
  {
    setEnabled(false);
  }

  tb->setStrings(CM_GIZMO_BASIS, itemsBasis);
  tb->setStrings(CM_GIZMO_CENTER, itemsCenter);
}


//==============================================================================
void ToolBarManager::setGizmoBasisAndCenter(int basis, int center)
{
  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(tbId);
  if (!tb)
    return;

  if (!controlsInserted)
    return;

  if (gizmoBasisType != basis)
  {
    gizmoBasisType = basis;
    const char *basisName = NULL;

    switch (basis)
    {
      case IEditorCoreEngine::BASIS_None:
        tb->setInt(CM_GIZMO_BASIS, -1);
        basisName = NULL;
        break;

      case IEditorCoreEngine::BASIS_World: basisName = getBasisWorldCaption(); break;

      case IEditorCoreEngine::BASIS_Local: basisName = getBasisLocalCaption(); break;

      default: G_ASSERT(0);
    }

    if (basisName)
    {
      int ind = 0;
      for (int i = 0; i < itemsBasis.size(); ++i)
        if (itemsBasis[i] == basisName)
        {
          ind = i;
          break;
        }

      tb->setInt(CM_GIZMO_BASIS, ind);
      onChange(CM_GIZMO_BASIS, tb);
    }
  }

  if (gizmoCenterType != center)
  {
    gizmoCenterType = center;
    const char *centerName = NULL;

    switch (center)
    {
      case IEditorCoreEngine::CENTER_None:
        tb->setInt(CM_GIZMO_CENTER, -1);
        centerName = NULL;
        break;
      case IEditorCoreEngine::CENTER_Pivot: centerName = getCenterPivotCaption(); break;
      case IEditorCoreEngine::CENTER_Selection:
      case IEditorCoreEngine::CENTER_SelectionNotRotObj: centerName = getCenterSelectionCaption(); break;
      case IEditorCoreEngine::CENTER_Coordinates: centerName = getCenterCoordCaption(); break;
      default: G_ASSERT(0);
    }

    if (centerName)
    {
      int ind = 0;
      for (int i = 0; i < itemsCenter.size(); ++i)
        if (itemsCenter[i] == centerName)
        {
          ind = i;
          break;
        }

      tb->setInt(CM_GIZMO_CENTER, ind);
      onChange(CM_GIZMO_CENTER, tb);
    }
  }
}


//==============================================================================
int ToolBarManager::getMoveGizmoDef() const { return DEF_MOVE; }


//==============================================================================
int ToolBarManager::getMoveSurfGizmoDef() const { return DEF_MOVE_SURF; }


//==============================================================================
int ToolBarManager::getScaleGizmoDef() const { return DEF_SCALE; }


//==============================================================================
int ToolBarManager::getRotateGizmoDef() const { return DEF_ROTATE; }


//==============================================================================
void ToolBarManager::setClientValues(Point3 &val)
{
  Point3 pt;
  client->gizmoStarted();

  switch (type & IEditorCoreEngine::GIZMO_MASK_Mode)
  {
    case IEditorCoreEngine::MODE_Rotate:
      if (client->getRot(pt))
        client->changed(Point3(DegToRad(val.x) - pt.x, DegToRad(val.y) - pt.y, DegToRad(val.z) - pt.z));
      break;

    case IEditorCoreEngine::MODE_Scale:
      if (client->getScl(pt))
      {
        const Point3 scale(val / 100.0);
        client->changed(Point3(scale.x / pt.x, scale.y / pt.y, scale.z / pt.z));
      }
      break;

    case IEditorCoreEngine::MODE_Move: client->changed(val - client->getPt()); break;
  }

  client->gizmoEnded(true);

  IEditorCoreEngine::get()->updateViewports();
  IEditorCoreEngine::get()->invalidateViewportCache();
}


Point3 ToolBarManager::getClientGizmoValue()
{
  Point3 v(0.f, 0.f, 0.f);

  if (client)
  {
    switch (type & IEditorCoreEngine::GIZMO_MASK_Mode)
    {
      case IEditorCoreEngine::MODE_Rotate:
        if (client->getRot(v))
          v = Point3(RadToDeg(v.x), RadToDeg(v.y), RadToDeg(v.z));
        break;

      case IEditorCoreEngine::MODE_Scale:
        if (client->getScl(v))
          v *= 100;
        break;

      case IEditorCoreEngine::MODE_Move: v = client->getPt(); break;
    }
  }

  return v;
}


//==============================================================================

void ToolBarManager::setGizmoToToolbar(Point3 value)
{
  PropPanel::ContainerPropertyControl *tb = EDITORCORE->getCustomPanel(tbId);
  if (!tb)
    return;

  if (tb->getFloat(CM_GIZMO_X) != value.x)
    tb->setFloat(CM_GIZMO_X, value.x);
  if (tb->getFloat(CM_GIZMO_Y) != value.y)
    tb->setFloat(CM_GIZMO_Y, value.y);
  if (tb->getFloat(CM_GIZMO_Z) != value.z)
    tb->setFloat(CM_GIZMO_Z, value.z);
}


bool ToolBarManager::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  switch (pcb_id)
  {
    case CM_GIZMO_X:
      gizmoPos.x = panel->getFloat(pcb_id);
      setClientValues(gizmoPos);
      return true;
    case CM_GIZMO_Y:
      gizmoPos.y = panel->getFloat(pcb_id);
      setClientValues(gizmoPos);
      return true;
    case CM_GIZMO_Z:
      gizmoPos.z = panel->getFloat(pcb_id);
      setClientValues(gizmoPos);
      return true;
    case CM_GIZMO_BASIS: gizmoBasisType = getBasisTypeByName(panel->getText(CM_GIZMO_BASIS).str()); return true;
    case CM_GIZMO_CENTER:
      gizmoCenterType = getCenterTypeByName(panel->getText(CM_GIZMO_CENTER).str(), panel->getBool(CM_ROTATE_CENTER_AND_OBJ));
      return true;
  }

  return false;
}
