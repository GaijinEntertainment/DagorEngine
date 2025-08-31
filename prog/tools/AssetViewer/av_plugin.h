// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_interface_ex.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include "av_script_panel.h"

namespace PropPanel
{
class ContainerPropertyControl;
}

class DagorAsset;
class CoolConsole;
class IWndManager;
class AvTree;


class IGenEditorPlugin : public IGenEditorPluginBase, public IGenEventHandler, public IRenderingService
{
public:
  IGenEditorPlugin() : spEditor(NULL) {}
  ~IGenEditorPlugin() override { del_it(spEditor); }

  virtual const char *getInternalName() const = 0;

  virtual void registered() = 0;
  virtual void unregistered() = 0;

  // Plugins can save and load their settings with these functions. Each plugin has its own block within the main
  // application settings, so any name can be used freely in the provided DataBlock.
  virtual void loadSettings([[maybe_unused]] const DataBlock &settings) {}
  virtual void saveSettings([[maybe_unused]] DataBlock &settings) const {}

  virtual bool begin(DagorAsset *asset) = 0;
  virtual bool end() = 0;
  virtual IGenEventHandler *getEventHandler() { return this; }

  // called when user requests switch to this plugin
  virtual void registerMenuAccelerators() {}

  virtual void handleViewportAcceleratorCommand([[maybe_unused]] IGenViewportWnd &wnd, [[maybe_unused]] unsigned id) {}

  virtual bool reloadOnAssetChanged([[maybe_unused]] const DagorAsset *changed_asset) { return false; }
  virtual bool reloadAsset([[maybe_unused]] DagorAsset *asset) { return false; }

  virtual bool havePropPanel() { return false; }
  virtual bool haveToolPanel() { return false; }

  virtual void clearObjects() = 0;
  virtual void onSaveLibrary() = 0;
  virtual void onLoadLibrary() = 0;

  virtual bool getSelectionBox(BBox3 &box) const = 0;
  virtual void actObjects(real dt) = 0;
  virtual void beforeRenderObjects() = 0;
  virtual void renderObjects() = 0;
  virtual void renderTransObjects() = 0;
  void renderUI() override {}
  virtual void updateImgui() {}

  virtual bool supportAssetType([[maybe_unused]] const DagorAsset &asset) const { return false; }
  virtual bool supportEditing() const { return true; }

  virtual void fillPropPanel(PropPanel::ContainerPropertyControl &propPanel) = 0;
  virtual void postFillPropPanel() = 0;
  virtual void onPropPanelClear(PropPanel::ContainerPropertyControl &propPanel);

  virtual bool hasScriptPanel();
  virtual void fillScriptPanel(PropPanel::ContainerPropertyControl &propPanel);
  void initScriptPanelEditor(const char *scheme, const char *panel_caption = NULL);

  // IGenEventHandler
  //===========================================================================
  bool handleMouseMove(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
  bool handleMouseLBPress(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
  bool handleMouseLBRelease(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
  bool handleMouseRBPress(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
  bool handleMouseRBRelease(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
  bool handleMouseCBPress(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
  bool handleMouseCBRelease(IGenViewportWnd *, int, int, bool, int, int) override { return false; }
  bool handleMouseWheel(IGenViewportWnd *, int, int, int, int) override { return false; }
  bool handleMouseDoubleClick(IGenViewportWnd *, int, int, int) override { return false; }

  void handleViewportPaint(IGenViewportWnd *wnd) override { drawInfo(wnd); }
  void handleViewChange(IGenViewportWnd *) override {}
  //===========================================================================

  // IGenEditorPluginBase
  bool getVisible() const override;
  void *queryInterfacePtr(unsigned huid) override
  {
    return huid == IRenderingService::HUID ? static_cast<IRenderingService *>(this) : NULL;
  }

  // IRenderingService
  void renderGeometry(Stage) override {}

  static void drawInfo(IGenViewportWnd *wnd);
  static void repaintView();
  static PropPanel::ContainerPropertyControl *getPropPanel();
  static PropPanel::ContainerPropertyControl *getPluginPanel();
  static void fillPluginPanel();
  static CoolConsole &getMainConsole();
  static IWndManager &getWndManager();

protected:
  AVScriptPanelEditor *spEditor;
};
