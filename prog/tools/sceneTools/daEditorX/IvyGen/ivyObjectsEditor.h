// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <oldEditor/de_cm.h>
#include <sys/stat.h>
#include <shaders/dag_overrideStateId.h>

#include "ivyObject.h"

class StaticGeometryContainer;
class GeomObject;
class Node;

class IvyObjectEditor : public ObjectEditor
{
public:
  IvyObjectEditor();
  ~IvyObjectEditor() override;

  // ObjectEditor interface implementation
  void fillToolBar(PropPanel::ContainerPropertyControl *toolbar) override;
  void updateToolbarButtons() override;

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void gizmoStarted() override;
  void gizmoEnded(bool apply) override;

  void setEditMode(int cm) override;

  void beforeRender() override;
  void render() override;
  void renderTrans() override;

  void update(real dt) override;

  void save(DataBlock &blk);
  void load(const DataBlock &blk);

  bool findTargetPos(IGenViewportWnd *wnd, int x, int y, Point3 &out);

  void getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types) override;
  void getTypeNames(Tab<String> &names) override;
  void onSelectedNames(const Tab<String> &names) override;

  void gatherStaticGeometry(StaticGeometryContainer &cont, int flags);

  inline int getNextObjId()
  {
    int uid = nextPtUid++;
    return uid;
  }

  inline bool isCloneMode() { return cloneMode; }

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

protected:
  shaders::OverrideStateId zFuncLessStateId;

  bool cloneMode;
  Ptr<IvyObject> curPt;

  real secTime;
  int nextPtUid;
  bool inGizmo;
  DynRenderBuffer *ptDynBuf;
  Point3 locAx[3];
};
