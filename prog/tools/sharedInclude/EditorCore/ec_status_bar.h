// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_math3d.h>
#include <util/dag_string.h>
#include <EditorCore/ec_interface.h>


/// Toolbar manager.
/// Used to display toolbar with Gizmo parameters, 'Navigate' button, etc on it.
/// @ingroup EditorCore
/// @ingroup EditorBaseClasses
class ToolBarManager
{
public:
  /// Constructor.
  ToolBarManager();

  virtual ~ToolBarManager() {}


  //*******************************************************
  ///@name Toolbar management
  //@{
  /// Init toolbar.
  /// @param[in] toolbar_id - toolbar id
  void init(int toolbar_id);
  //@}

  //*******************************************************
  ///@name Gizmo methods
  //@{
  /// Set interface IGizmoClient and Gizmo type.
  /// Caled by editor core.
  /// @param[in] gc - ponter to interface IGizmoClient
  /// @param[in] tp - Gizmo type (see IEditorCoreEngine::ModeType)
  void setGizmoClient(IGizmoClient *gc, IEditorCoreEngine::ModeType tp);

  /// Acts the ToolBarManager.
  /// In this function ToolBarManager sets proper values in edit boxes located
  /// on assigned toolbar and enables/disables toolbar's controls.
  /// Called by editor core.
  void act();

  /// Get Gizmo's center type.
  /// @return Gizmo's center type (see IEditorCoreEngine::CenterType)
  IEditorCoreEngine::CenterType getCenterType() const;

  /// Get Gizmo's basis type
  /// @return Gizmo's basis type (see IEditorCoreEngine::BasisType)
  IEditorCoreEngine::BasisType getBasisType() const;

  /// Get Gizmo's basis type for a specific mode
  /// @return Gizmo's basis type for a specific mode (see IEditorCoreEngine::BasisType and see IEditorCoreEngine::ModeType)
  IEditorCoreEngine::BasisType getGizmoBasisTypeForMode(IEditorCoreEngine::ModeType tp) const;
  //@}


  //*******************************************************
  ///@name Functions called by editor core
  //@{
  /// Push / pull 'Move snap toggle' button.
  virtual void setMoveSnap();

  /// Push / pull 'Scale snap toggle' button.
  virtual void setScaleSnap();

  /// Push / pull 'Rotate snap toggle' button.
  virtual void setRotateSnap();

  //@}

  /// Editor should call onChange in toolbar collback
  bool onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);

protected:
  int tbId;

  bool isEnabled;
  Point3 gizmoPos;

  int gizmoBasisType;
  int gizmoCenterType;

  Tab<String> itemsBasis;
  Tab<String> itemsCenter;

  bool isEnabledBtnRotateCenterAndObj;

  bool controlsInserted;

  IEditorCoreEngine::ModeType type;
  IGizmoClient *client;
  int availableTypes;

  int moveGizmo;
  int moveSurfGizmo;
  int scaleGizmo;
  int rotateGizmo;

  void setClientValues(Point3 &val);

  void refillTypes();
  void setGizmoBasisAndCenter(int bas, int center);
  void setEnabled(bool enable);

  IEditorCoreEngine::CenterType getCenterTypeByName(const char *name, bool enableRotObj) const;
  IEditorCoreEngine::BasisType getBasisTypeByName(const char *name) const;

  const char *getCenterNameByType(IEditorCoreEngine::CenterType type) const;
  const char *getBasisNameByType(IEditorCoreEngine::BasisType type) const;

  const char *getBasisWorldCaption() const { return "World"; }
  const char *getBasisLocalCaption() const { return "Local"; }

  const char *getCenterPivotCaption() const { return "Pivot"; }
  const char *getCenterSelectionCaption() const { return "Selection"; }
  const char *getCenterCoordCaption() const { return "Coord"; }

  int getMoveGizmoDef() const;
  int getMoveSurfGizmoDef() const;
  int getScaleGizmoDef() const;
  int getRotateGizmoDef() const;

  Point3 getClientGizmoValue();
  void setGizmoToToolbar(Point3 value);
};
