// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _DE2_PLUGIN_IVYGEN_OBJEDITOR_H_
#define _DE2_PLUGIN_IVYGEN_OBJEDITOR_H_
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
  virtual ~IvyObjectEditor();

  // ObjectEditor interface implementation
  virtual void fillToolBar(PropertyContainerControlBase *toolbar);
  virtual void updateToolbarButtons();

  // virtual void handleCommand(int cmd);
  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual void gizmoStarted();
  virtual void gizmoEnded(bool apply);

  virtual void setEditMode(int cm);

  virtual void beforeRender();
  virtual void render();
  virtual void renderTrans();

  virtual void update(real dt);

  void save(DataBlock &blk);
  void load(const DataBlock &blk);

  bool findTargetPos(IGenViewportWnd *wnd, int x, int y, Point3 &out);

  virtual void getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types);
  virtual void getTypeNames(Tab<String> &names);
  virtual void onSelectedNames(const Tab<String> &names);

  void gatherStaticGeometry(StaticGeometryContainer &cont, int flags);

  inline int getNextObjId()
  {
    int uid = nextPtUid++;
    return uid;
  }

  inline bool isCloneMode() { return cloneMode; }

  virtual void onClick(int pcb_id, PropPanel2 *panel);

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

#endif
