// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "decl.h"
#include <EditorCore/ec_ObjectEditor.h>
#include <de3_occluderGeomProvider.h>

class StaticGeometryContainer;
class GeomObject;
class Node;

class occplugin::ObjEd : public ObjectEditor
{
public:
  ObjEd();
  virtual ~ObjEd();

  // ObjectEditor interface implementation
  virtual void fillToolBar(PropPanel::ContainerPropertyControl *toolbar);
  virtual void updateToolbarButtons();

  // virtual void handleCommand(int cmd);
  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif);
  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif);
  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual void gizmoStarted();
  virtual void gizmoEnded(bool apply);

  virtual void beforeRender();
  virtual void render() {}
  virtual void renderTrans() {}

  virtual void update(real dt);

  void save(DataBlock &blk);
  void load(const DataBlock &blk);

  virtual void getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types);
  virtual void getTypeNames(Tab<String> &names);
  virtual void onSelectedNames(const Tab<String> &names);

  virtual void addButton(PropPanel::ContainerPropertyControl *tb, int id, const char *bmp_name, const char *hint, bool check = false);

  void reset();
  inline bool isCloneMode() { return cloneMode; }

  void objRender(dag::ConstSpan<TMatrix> ob, dag::ConstSpan<IOccluderGeomProvider::Quad> oq);
  void objRenderTr(dag::ConstSpan<TMatrix> ob, dag::ConstSpan<IOccluderGeomProvider::Quad> oq);

  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);

public:
  bool showLocalOccluders, showGlobalOccluders;

protected:
  bool cloneMode;

  real secTime;
  DynRenderBuffer *ptDynBuf;
  IObjectCreator *objCreator;
};
