// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_ObjectEditor.h>
#include <de3_occluderGeomProvider.h>

class StaticGeometryContainer;
class GeomObject;
class Node;

class ObjEd : public ObjectEditor
{
public:
  ObjEd();
  ~ObjEd() override;

  // ObjectEditor interface implementation
  void registerViewportAccelerators(IWndManager &wndManager) override;
  void fillToolBar(PropPanel::ContainerPropertyControl *toolbar) override;
  void updateToolbarButtons() override;

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void gizmoStarted() override;
  void gizmoEnded(bool apply) override;

  void beforeRender() override;
  void render() override {}
  void renderTrans() override {}

  void update(real dt) override;

  void save(DataBlock &blk);
  void load(const DataBlock &blk);

  void getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types) override;
  void getTypeNames(Tab<String> &names) override;
  void onSelectedNames(const Tab<String> &names) override;

  void addButton(PropPanel::ContainerPropertyControl *tb, int id, const char *bmp_name, const char *hint, bool check = false) override;

  void reset();
  inline bool isCloneMode() { return cloneMode; }

  void objRender();
  void objRenderTr();

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

public:
  bool showLocalBoxCSG;

protected:
  bool cloneMode;

  real secTime;
  DynRenderBuffer *ptDynBuf;
  IObjectCreator *objCreator;
};
