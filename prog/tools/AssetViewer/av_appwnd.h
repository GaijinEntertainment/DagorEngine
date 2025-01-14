// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "av_environment.h"
#include "Entity/compositeEditor.h"

#include <sepGui/wndPublic.h>

#include <propPanel/messageQueue.h>
#include <propPanel/control/panelWindow.h>

#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_genappwnd.h>

#include <libTools/dagFileRW/textureNameResolver.h>

#include <assets/assetMgr.h>
#include <assets/assetChangeNotify.h>
#include <assetsGui/av_client.h>

#include <util/dag_string.h>
#include <util/dag_simpleString.h>

#include <EASTL/unique_ptr.h>

enum
{
  GUI_PLUGIN_TOOLBAR_ID,
};


void init_all_editor_plugins();

class ImpostorGenerator;

namespace plod
{
class PointCloudGenerator;
} // namespace plod

struct ImpostorOptions;

class CompositeEditor;
class ColorDialogAppMat;
class MainAssetSelector;

class AssetViewerApp;
AssetViewerApp &get_app();


class AssetViewerApp : public GenericEditorAppWindow,
                       public IEditorCoreEngine,
                       public ITextureNameResolver,
                       public IDagorAssetChangeNotify,
                       public PropPanel::ControlEventHandler,
                       public IAssetBaseViewClient,
                       public IAssetSelectorContextMenuHandler,
                       public IConsoleCmd,
                       public IWndManagerWindowHandler,
                       public PropPanel::IDelayedCallbackHandler,
                       public IMainWindowImguiRenderingService
{
public:
  AssetViewerApp(IWndManager *manager, const char *open_fname = NULL);
  ~AssetViewerApp();

  static const char *build_version;

  inline void repaint();
  inline PropPanel::ContainerPropertyControl *getPropPanel() const;

  void fillPropPanel();
  void setScriptChangeFlag() { scriptChangeFlag = true; }
  bool getScriptChangeFlag() { return scriptChangeFlag; }
  void fillToolBar();

  virtual void init();

  void renderGrid(bool render) { grid.setVisible(render, 0); }
  bool isGridVisible() { return grid.isVisible(0); }
  const char *getMatParamsPath() { return matParamsPath; }

  // IEditorCoreEngine
  // ==========================================================================
  // query/get interfaces
  virtual void *queryEditorInterfacePtr(unsigned huid);

  virtual void screenshotRender();

  // register/unregister plugins(every plugin should be registered once)
  virtual bool registerPlugin(IGenEditorPlugin *plugin);
  virtual bool unregisterPlugin(IGenEditorPlugin *plugin);

  // plugins management.
  virtual int getPluginCount();
  virtual IGenEditorPlugin *getPlugin(int idx);
  virtual int getPluginIdx(IGenEditorPlugin *plug) const;
  virtual IGenEditorPlugin *curPlugin();

  virtual IGenEditorPluginBase *getPluginBase(int idx);
  virtual IGenEditorPluginBase *curPluginBase();

  virtual void *getInterface(int interface_uid);
  virtual void getInterfaces(int interface_uid, Tab<void *> &interfaces);

  // UI management
  virtual IWndManager *getWndManager() const;
  virtual PropPanel::ContainerPropertyControl *getCustomPanel(int id) const;
  virtual void addPropPanel(int type, hdpi::Px width);
  virtual void removePropPanel(void *hwnd);
  virtual void managePropPanels() {}
  virtual void skipManagePropPanels(bool skip) {}
  virtual PropPanel::PanelWindowPropertyControl *createPropPanel(PropPanel::ControlEventHandler *eh, const char *caption) override;
  virtual PropPanel::IMenu *getMainMenu() override;
  virtual void deleteCustomPanel(PropPanel::ContainerPropertyControl *panel) {}
  virtual PropPanel::DialogWindow *createDialog(hdpi::Px w, hdpi::Px h, const char *title) override;
  virtual void deleteDialog(PropPanel::DialogWindow *dlg) override;

  // viewport methods
  virtual void updateViewports();
  virtual void setViewportCacheMode(ViewportCacheMode mode);
  virtual void invalidateViewportCache();
  virtual int getViewportCount();
  virtual IGenViewportWnd *getViewport(int n);
  virtual IGenViewportWnd *getRenderViewport();
  virtual IGenViewportWnd *getCurrentViewport();
  virtual void setViewportZnearZfar(real zn, real zf);

  // ray tracing methods
  virtual IGenViewportWnd *screenToViewport(int &x, int &y);
  virtual bool screenToWorldTrace(int x, int y, Point3 &world, real maxdist = 1000.0, Point3 *out_norm = NULL) { return false; }
  virtual bool clientToWorldTrace(IGenViewportWnd *wnd, int x, int y, Point3 &world, real maxdist = 1000.0, Point3 *out_norm = NULL)
  {
    return false;
  }
  virtual void setupColliderParams(int mode, const BBox3 &area);
  virtual bool traceRay(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm = NULL, bool use_zero_plane = true);

  virtual void setColliders(dag::ConstSpan<IDagorEdCustomCollider *> c, unsigned filter_mask) const {};
  virtual void restoreEditorColliders() const {}
  virtual float getMaxTraceDistance() const { return 1500; }

  virtual bool getSelectionBox(BBox3 &box);
  virtual void zoomAndCenter();

  virtual String getScreenshotNameMask(bool cube) const;

  // gizmo methods
  virtual void setGizmo(IGizmoClient *gc, ModeType type);
  virtual void startGizmo(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual ModeType getGizmoModeType(); //{ return MODE_Move; }
  virtual BasisType getGizmoBasisType() { return BASIS_World; }
  virtual BasisType getGizmoBasisTypeForMode(ModeType tp) { return BASIS_World; }
  virtual CenterType getGizmoCenterType() { return CENTER_Pivot; }
  virtual bool isGizmoOperationStarted() const override;

  // brush methods
  virtual void beginBrushPaint() {}
  virtual void renderBrush() {}
  virtual void setBrush(Brush *brush) {}
  virtual void endBrushPaint() {}
  virtual Brush *getBrush() const { return NULL; }
  virtual bool isBrushPainting() const { return false; }

  // spatial cursor handling
  virtual void showUiCursor(bool vis) {}
  virtual void setUiCursorPos(const Point3 &pos, const Point3 *norm = NULL) {}
  virtual void getUiCursorPos(Point3 &pos, Point3 &norm) {}
  virtual void setUiCursorTex(TEXTUREID tex_id) {}
  virtual void setUiCursorProps(float size, bool always_xz) {}

  // internal interface
  virtual void actObjects(real dt);
  virtual void beforeRenderObjects();
  virtual void renderObjects();
  virtual void renderIslDecalObjects() {}
  virtual void renderTransObjects();

  virtual void showSelectWindow(IObjectsList *obj_list, const char *obj_list_owner_name);

  virtual UndoSystem *getUndoSystem() { return undoSystem; }
  virtual CoolConsole &getConsole()
  {
    G_ASSERT(console);
    return *console;
  }
  virtual GridObject &getGrid() { return grid; }

  virtual Point3 snapToGrid(const Point3 &p) const;
  virtual Point3 snapToAngle(const Point3 &p) const;
  virtual Point3 snapToScale(const Point3 &p) const;

  virtual const char *getLibDir() const { return NULL; }
  virtual class LibCache *getLibCachePtr() { return NULL; }
  virtual Tab<struct WspLibData> *getLibData() { return NULL; }
  virtual const EditorWorkspace &getBaseWorkspace() { return GenericEditorAppWindow::getWorkspace(); }

  // ==========================================================================

  // ITextureNameResolver
  virtual bool resolveTextureName(const char *src_name, String &out_str);

  // IDagorAssetChangeNotify
  virtual void onAssetRemoved(int asset_name_id, int asset_type);
  virtual void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type);

  // IWndManagerWindowHandler
  virtual void *onWmCreateWindow(int type) override;
  virtual bool onWmDestroyWindow(void *window) override;

  // IConsoleCmd
  virtual bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params);
  virtual const char *onConsoleCommandHelp(const char *cmd);

  void drawAssetInformation(IGenViewportWnd *wnd);
  void afterUpToDateCheck(bool changed);
  const DagorAssetMgr &getAssetMgr() const { return assetMgr; }
  bool trackChangesContinuous(int assets_to_check);
  void invalidateAssetIfChanged(DagorAsset &a);

  const DagorAsset *getCurAsset() const { return curAsset; }
  bool reloadAsset(const DagorAsset &asset, int asset_name_id, int asset_type);
  void refillTree();
  void selectAsset(const DagorAsset &asset);

  CompositeEditor &getCompositeEditor() { return compositeEditor; }
  ImpostorGenerator *getImpostorGenerator() const { return impostorApp.get(); }
  plod::PointCloudGenerator *getPointcloudGenerator() const { return pointCloudGen.get(); }
  bool canRenderEnvi() const { return !skipRenderEnvi; }
  bool isCompositeEditorShown() const;
  void showCompositeEditor(bool show);
  void setShowMessageAt(int, int, const SimpleString &) {}
  void showMessageAt() {}

protected:
  virtual bool handleNewProject(bool edit = false) { return false; }
  virtual bool handleOpenProject(bool edit = false) { return false; }
  // virtual bool canCreateNewProject(const char *filename) { return false; }  // not used

  virtual bool createNewProject(const char *filename) { return false; }
  virtual bool loadProject(const char *filename);
  virtual bool saveProject(const char *filename);

  virtual void fillMenu(PropPanel::IMenu *menu) override;
  virtual void updateMenu(PropPanel::IMenu *menu) override;

  virtual void getDocTitleText(String &text);
  virtual bool canCloseScene(const char *title);

  void addAccelerators();

  int findPlugin(IGenEditorPlugin *p);
  void sortPlugins() {}

  void terminateInterface();
  void switchToPlugin(int id);
  void splitProjectFilename(const char *filename, String &path, String &name);

  // ControlEventHandler
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);

  // IAssetBaseViewClient
  virtual void onAvClose() override {}
  virtual void onAvAssetDblClick(DagorAsset *asset, const char *asset_name) override {}
  virtual void onAvSelectAsset(DagorAsset *asset, const char *asset_name) override;
  virtual void onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name) override;

  // IAssetSelectorContextMenuHandler
  virtual bool onAssetSelectorContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) override;

  void onAssetSelectionChanged(DagorAsset *asset, DagorAssetFolder *asset_folder);

  // Menu
  int onMenuItemClick(unsigned id);

  void makeDefaultLayout();
  void fillTree();
  void saveTreeState();

  void applyDiscardAssetTexMode();

private:
  SimpleString matParamsPath;
  IGenEditorPlugin *getTypeSupporter(const DagorAsset *asset) const;

  // from EditorCore
  Tab<IGenEditorPlugin *> plugin;
  int curPluginId;

  DagorAssetMgr assetMgr;

  DagorAsset *curAsset;
  String curAssetPackName;
  String curAssetPkgName;

  int allUpToDateFlags;

  MainAssetSelector *mTreeView;
  DataBlock propPanelState;
  PropPanel::PanelWindowPropertyControl *mPropPanel;
  PropPanel::ContainerPropertyControl *mToolPanel;
  PropPanel::ContainerPropertyControl *mPluginTool;

  eastl::unique_ptr<ImpostorGenerator> impostorApp;
  eastl::unique_ptr<plod::PointCloudGenerator> pointCloudGen;
  eastl::unique_ptr<ColorDialogAppMat> colorPaletteDlg;
  CompositeEditor compositeEditor;

  // int allUpToDateFlags;

  bool blockSave;
  bool fillPropPanelHasBeenCalled = false;
  bool scriptChangeFlag;
  bool autoZoomAndCenter;
  bool discardAssetTexMode;
  bool skipRenderObjects = false;
  bool skipRenderEnvi = false;
  bool skipSetViewProj = false;

  AssetLightData assetLtData, assetDefaultLtData;

  void onClosing();

  void renderGrid();

  void blockModifyRoutine(bool block);

  void showPropWindow(bool is_show);
  void showAdditinalToolWindow(bool is_show);

  bool runShadersListVars(dag::ConstSpan<const char *> params);
  bool runShadersSetVar(dag::ConstSpan<const char *> params);
  bool runShadersReload(dag::ConstSpan<const char *> params);

  void generate_impostors(const ImpostorOptions &options);
  void clear_impostors(const ImpostorOptions &options);

  void generate_point_cloud(DagorAsset *asset);

  void createAssetsTree();
  void createToolbar();

  void renderUIViewportSplitter(const Point2 &regionAvailable, float leftWidth, float rightWidth, float topHeight, float bottomHeight,
    float itemSpacing);
  void renderUIViewports(bool vr_mode);
  void renderUI();

  // IMainWindowImguiRenderingService
  virtual void beforeUpdateImgui() override;
  virtual void updateImgui() override;

  // PropPanel::IDelayedCallbackHandler
  virtual void onImguiDelayedCallback(void *user_data) override;

  Point2 viewportSplitRatio = Point2(0.5f, 0.5f);
  bool dockPositionsInitialized = false;
  bool imguiDebugWindowsVisible = false;
  String assetToInitiallySelect;
};


//=============================================================================
// inine functions
//=============================================================================
inline IGenEditorPlugin *AssetViewerApp::curPlugin() { return curPluginId == -1 ? NULL : plugin[curPluginId]; }


//=============================================================================
inline void AssetViewerApp::repaint()
{
  updateViewports();
  invalidateViewportCache();
}


//=============================================================================
inline PropPanel::ContainerPropertyControl *AssetViewerApp::getPropPanel() const { return mPropPanel; }
