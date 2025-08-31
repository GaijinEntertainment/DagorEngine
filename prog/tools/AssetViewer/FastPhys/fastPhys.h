// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_wndPublic.h>
#include "../av_plugin.h"

#include "fastPhysEd.h"


class FPPanel;
class ActionsTreeCB;
namespace PropPanel
{
class TreeBaseWindow;
}

//------------------------------------------------------------------

class FastPhysPlugin : public IGenEditorPlugin, public IWndManagerWindowHandler
{
public:
  FastPhysPlugin();
  ~FastPhysPlugin() override;

  const char *getInternalName() const override { return "FastPhysViewer"; }
  bool supportAssetType(const DagorAsset &asset) const override;

  void registered() override {}
  void unregistered() override {}

  bool havePropPanel() override { return true; }
  bool haveToolPanel() override { return true; }

  void refillActionTree();
  void clearTree();

  void refillPanel();
  void updateActionPanel();
  FastPhysEditor *getEditor() { return &mFastPhysEditor; }
  DagorAsset *getAsset() { return mAsset; }


  bool begin(DagorAsset *asset) override;
  bool end() override;

  void registerMenuAccelerators() override;
  void handleViewportAcceleratorCommand([[maybe_unused]] IGenViewportWnd &wnd, [[maybe_unused]] unsigned id) override;

  void clearObjects() override {}
  void onSaveLibrary() override {}
  void onLoadLibrary() override {}

  bool getSelectionBox(BBox3 &box) const override;

  void actObjects(float dt) override { mFastPhysEditor.update(dt); }
  void beforeRenderObjects() override { mFastPhysEditor.beforeRender(); }
  void renderObjects() override { mFastPhysEditor.render(); }
  void renderTransObjects() override { mFastPhysEditor.renderTrans(); }
  void renderGeometry(Stage stage) override { mFastPhysEditor.renderGeometry(stage); }
  void updateImgui() override;

  void fillPropPanel(PropPanel::ContainerPropertyControl &panel) override;
  void postFillPropPanel() override {}

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  // IWndManagerWindowHandler
  void *onWmCreateWindow(int type) override;
  bool onWmDestroyWindow(void *window) override;

protected:
  void addTreeAction(PropPanel::TLeafHandle parent, FpdAction *action);

private:
  DagorAsset *mAsset;

  FastPhysEditor mFastPhysEditor;
  FPPanel *propPanel;

public:
  Point3 mWindVel;
  float mWindPower, mWindTurb;
  bool mRespondsToWind;

private:
  PropPanel::TreeBaseWindow *mActionTree;
  ActionsTreeCB *mActionTreeCB;
};


//------------------------------------------------------------------
