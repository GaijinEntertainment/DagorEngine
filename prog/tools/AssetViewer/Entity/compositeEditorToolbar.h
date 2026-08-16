// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <EditorCore/ec_interface.h>

namespace PropPanel
{
class ContainerPropertyControl;
class ControlEventHandler;
} // namespace PropPanel

class CompositeEditorToolbar
{
public:
  void initUi(PropPanel::ControlEventHandler &event_handler, IGizmoClient *gc, int toolbar_id);
  void closeUi();
  bool isInited() const;
  void updateGizmoToolbarButtons(bool canTransform);
  void updateSnapToolbarButtons();

  bool onChange(int pcb_id);
  void setGizmoBasisAndCenter(int basis, int center);

  void setGizmoClientType(IEditorCoreEngine::ModeType tp);
  void refillTypes(bool canTransform);

  // TODO: change to a shared implementation with ToolBarManager
  IEditorCoreEngine::CenterType getCenterType() const;
  IEditorCoreEngine::CenterType getGizmoCenterTypeForMode(IEditorCoreEngine::ModeType tp) const;
  IEditorCoreEngine::BasisType getBasisType() const;
  IEditorCoreEngine::BasisType getGizmoBasisTypeForMode(IEditorCoreEngine::ModeType tp) const;

private:
  void addCheckButton(PropPanel::ContainerPropertyControl &tb, int id, const char *editor_command_id, const char *bmp_name,
    const char *hint);
  void addButton(PropPanel::ContainerPropertyControl &tb, int id, const char *editor_command_id, const char *bmp_name,
    const char *hint);
  void setButtonState(int id, bool checked, bool enabled);

  int toolBarId = -1;

  IEditorCoreEngine::ModeType type = IEditorCoreEngine::MODE_None;
  IGizmoClient *client = nullptr;
  int availableTypes = 0;

  int moveGizmo;
  int moveSurfGizmo;
  int scaleGizmo;
  int rotateGizmo;

  int gizmoBasisType = -1;
  int gizmoCenterType = -1;

  Tab<String> itemsBasis;
  Tab<String> itemsCenter;

  // TODO: change to a shared implementation with ToolBarManager
  IEditorCoreEngine::CenterType getCenterTypeByName(const char *name, bool enableRotObj) const;
  IEditorCoreEngine::BasisType getBasisTypeByName(const char *name) const;

  const char *getCenterNameByType(IEditorCoreEngine::CenterType type) const;
  const char *getBasisNameByType(IEditorCoreEngine::BasisType type) const;

  const char *getBasisWorldCaption() const { return "World"; }
  const char *getBasisLocalCaption() const { return "Local"; }
  const char *getBasisParentCaption() const { return "Parent"; }

  const char *getCenterPivotCaption() const { return "Pivot"; }
  const char *getCenterSelectionCaption() const { return "Selection"; }
  const char *getCenterCoordCaption() const { return "Coord"; }
};
