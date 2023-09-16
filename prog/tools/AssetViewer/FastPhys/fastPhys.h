#pragma once

#include <sepGui/wndPublic.h>
#include "../av_plugin.h"

#include "fastPhysEd.h"


class FPPanel;
class ActionsTreeCB;
class TreeBaseWindow;

//------------------------------------------------------------------

class FastPhysPlugin : public IGenEditorPlugin, public IWndManagerWindowHandler
{
public:
  FastPhysPlugin();
  ~FastPhysPlugin();

  virtual const char *getInternalName() const { return "FastPhysViewer"; }
  virtual bool supportAssetType(const DagorAsset &asset) const;

  virtual void registered() {}
  virtual void unregistered() {}

  virtual bool havePropPanel() { return true; }
  virtual bool haveToolPanel() { return true; }

  void refillActionTree();
  void clearTree();

  void refillPanel();
  void updateActionPanel();
  FastPhysEditor *getEditor() { return &mFastPhysEditor; }
  DagorAsset *getAsset() { return mAsset; }


  virtual bool begin(DagorAsset *asset);
  virtual bool end();

  virtual void clearObjects() {}
  virtual void onSaveLibrary() {}
  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const;

  virtual void actObjects(float dt) { mFastPhysEditor.update(dt); }
  virtual void beforeRenderObjects() { mFastPhysEditor.beforeRender(); }
  virtual void renderObjects() { mFastPhysEditor.render(); }
  virtual void renderTransObjects() { mFastPhysEditor.renderTrans(); }
  virtual void renderGeometry(Stage stage) { mFastPhysEditor.renderGeometry(stage); }

  virtual void fillPropPanel(PropertyContainerControlBase &panel);
  virtual void postFillPropPanel() {}

  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif);
  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  // IWndManagerWindowHandler
  virtual IWndEmbeddedWindow *onWmCreateWindow(void *handle, int type);
  virtual bool onWmDestroyWindow(void *handle);

protected:
  void addTreeAction(TLeafHandle parent, FpdAction *action);

private:
  DagorAsset *mAsset;
  void *hwndPanel;

  FastPhysEditor mFastPhysEditor;
  FPPanel *propPanel;

  TreeBaseWindow *mActionTree;
  ActionsTreeCB *mActionTreeCB;
};


//------------------------------------------------------------------
