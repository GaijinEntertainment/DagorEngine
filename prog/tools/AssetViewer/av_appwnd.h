// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "av_environment.h"
#include "Entity/compositeEditor.h"

#include <propPanel/messageQueue.h>
#include <propPanel/control/panelWindow.h>

#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_genappwnd.h>
#include <EditorCore/ec_wndPublic.h>

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
  ~AssetViewerApp() override;

  static const char *build_version;

  inline void repaint();
  inline PropPanel::ContainerPropertyControl *getPropPanel() const;

  void fillPropPanel();
  void setScriptChangeFlag() { scriptChangeFlag = true; }
  bool getScriptChangeFlag() { return scriptChangeFlag; }
  void fillToolBar();

  void init(const char *select_workspace) override;

  void renderGrid(bool render) { grid.setVisible(render, 0); }
  bool isGridVisible() { return grid.isVisible(0); }
  const char *getMatParamsPath() { return matParamsPath; }

  // IEditorCoreEngine
  // ==========================================================================
  // query/get interfaces
  void *queryEditorInterfacePtr(unsigned huid) override;

  void screenshotRender(bool skip_debug_objects) override;

  // register/unregister plugins(every plugin should be registered once)
  bool registerPlugin(IGenEditorPlugin *plugin) override;
  bool unregisterPlugin(IGenEditorPlugin *plugin) override;

  // plugins management.
  int getPluginCount() override;
  IGenEditorPlugin *getPlugin(int idx) override;
  int getPluginIdx(IGenEditorPlugin *plug) const;
  IGenEditorPlugin *curPlugin() override;

  IGenEditorPluginBase *getPluginBase(int idx) override;
  IGenEditorPluginBase *curPluginBase() override;

  void *getInterface(int interface_uid) override;
  void getInterfaces(int interface_uid, Tab<void *> &interfaces) override;

  // UI management
  IWndManager *getWndManager() const override;
  PropPanel::ContainerPropertyControl *getCustomPanel(int id) const override;
  void addPropPanel(int type, hdpi::Px width) override;
  void removePropPanel(void *hwnd) override;
  void managePropPanels() override {}
  void skipManagePropPanels(bool) override {}
  PropPanel::PanelWindowPropertyControl *createPropPanel(PropPanel::ControlEventHandler *eh, const char *caption) override;
  PropPanel::IMenu *getMainMenu() override;
  void deleteCustomPanel(PropPanel::ContainerPropertyControl *) override {}
  PropPanel::DialogWindow *createDialog(hdpi::Px w, hdpi::Px h, const char *title) override;
  void deleteDialog(PropPanel::DialogWindow *dlg) override;

  // viewport methods
  void updateViewports() override;
  void setViewportCacheMode(ViewportCacheMode mode) override;
  void invalidateViewportCache() override;
  int getViewportCount() override;
  IGenViewportWnd *getViewport(int n) override;
  IGenViewportWnd *getRenderViewport() override;
  IGenViewportWnd *getCurrentViewport() override;
  void setViewportZnearZfar(real zn, real zf) override;

  // ray tracing methods
  IGenViewportWnd *screenToViewport(int &x, int &y) override;
  bool screenToWorldTrace(int, int, Point3 &, real, Point3 *) override { return false; }
  bool clientToWorldTrace(IGenViewportWnd *, int, int, Point3 &, real, Point3 *) override { return false; }
  void setupColliderParams(int mode, const BBox3 &area) override;
  bool traceRay(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm = NULL, bool use_zero_plane = true) override;

  void setColliders(dag::ConstSpan<IDagorEdCustomCollider *>, unsigned) const override {}
  void restoreEditorColliders() const override {}
  float getMaxTraceDistance() const override { return 1500; }

  bool getSelectionBox(BBox3 &box) override;
  void zoomAndCenter() override;

  String getScreenshotNameMask(bool cube) const override;

  // gizmo methods
  void setGizmo(IGizmoClient *gc, ModeType type) override;
  void startGizmo(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void endGizmo(bool apply) override;
  ModeType getGizmoModeType() override; //{ return MODE_Move; }
  BasisType getGizmoBasisType() override { return BASIS_World; }
  BasisType getGizmoBasisTypeForMode(ModeType) override { return BASIS_World; }
  CenterType getGizmoCenterType() override { return CENTER_Pivot; }
  CenterType getGizmoCenterTypeForMode(ModeType) override { return CENTER_Pivot; }
  bool isGizmoOperationStarted() const override;

  // brush methods
  void beginBrushPaint() override {}
  void renderBrush() override {}
  void setBrush(Brush *) override {}
  void endBrushPaint() override {}
  Brush *getBrush() const override { return NULL; }
  bool isBrushPainting() const override { return false; }

  // spatial cursor handling
  void showUiCursor(bool) override {}
  void setUiCursorPos(const Point3 &, const Point3 *) override {}
  void getUiCursorPos(Point3 &, Point3 &) override {}
  void setUiCursorTex(TEXTUREID) override {}
  void setUiCursorProps(float, bool) override {}

  // internal interface
  void actObjects(real dt) override;
  void beforeRenderObjects() override;
  void renderObjects() override;
  void renderIslDecalObjects() override {}
  void renderTransObjects() override;

  void showSelectWindow(IObjectsList *obj_list, const char *obj_list_owner_name) override;

  UndoSystem *getUndoSystem() override { return undoSystem; }
  CoolConsole &getConsole() override
  {
    G_ASSERT(console);
    return *console;
  }
  GridObject &getGrid() override { return grid; }

  GizmoEventFilter &getGizmoEventFilter() { return *gizmoEH; }

  Point3 snapToGrid(const Point3 &p) const override;
  Point3 snapToAngle(const Point3 &p) const override;
  Point3 snapToScale(const Point3 &p) const override;

  const char *getLibDir() const override { return NULL; }
  class LibCache *getLibCachePtr() override { return NULL; }
  Tab<struct WspLibData> *getLibData() override { return NULL; }
  const EditorWorkspace &getBaseWorkspace() override { return GenericEditorAppWindow::getWorkspace(); }

  // ==========================================================================

  // ITextureNameResolver
  bool resolveTextureName(const char *src_name, String &out_str) override;

  // IDagorAssetChangeNotify
  void onAssetRemoved(int asset_name_id, int asset_type) override;
  void onAssetChanged(const DagorAsset &asset, int asset_name_id, int asset_type) override;

  // IWndManagerWindowHandler
  void *onWmCreateWindow(int type) override;
  bool onWmDestroyWindow(void *window) override;

  // IConsoleCmd
  bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params) override;
  const char *onConsoleCommandHelp(const char *cmd) override;

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
  void setShowMessageAt(int, int, const SimpleString &) override {}
  void showMessageAt() override {}
  bool dngBasedSceneRenderUsed() const { return useDngBasedSceneRender; }
  void setCurrentTexQualityLimit(TexQL ql);
  TexQL getCurrentTexQualityLimit() const { return curMaxTexQL; }

protected:
  bool handleNewProject(bool) override { return false; }
  bool handleOpenProject(bool) override { return false; }

  bool createNewProject(const char *) override { return false; }
  bool loadProject(const char *filename) override;
  bool saveProject(const char *filename) override;

  void fillMenu(PropPanel::IMenu *menu) override;
  void updateMenu(PropPanel::IMenu *menu) override;

  void updateAssetBuildWarningsMenu();
  void updateThemeItems();

  void getDocTitleText(String &text) override;
  bool canCloseScene(const char *title) override;

  void addAccelerators();

  int findPlugin(IGenEditorPlugin *p);
  void sortPlugins() {}

  void terminateInterface();
  void switchToPlugin(int id);
  void splitProjectFilename(const char *filename, String &path, String &name);

  // ControlEventHandler
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // IAssetBaseViewClient
  void onAvClose() override {}
  void onAvAssetDblClick(DagorAsset *, const char *) override {}
  void onAvSelectAsset(DagorAsset *asset, const char *asset_name) override;
  void onAvSelectFolder(DagorAssetFolder *asset_folder, const char *asset_folder_name) override;

  // IAssetSelectorContextMenuHandler
  bool onAssetSelectorContextMenu(PropPanel::TreeBaseWindow &tree_base_window, PropPanel::ITreeInterface &tree) override;

  void onAssetSelectionChanged(DagorAsset *asset, DagorAssetFolder *asset_folder);

  // Menu
  int onMenuItemClick(unsigned id) override;

  void makeDefaultLayout();
  void fillTree();
  void saveTreeState();
  void loadGlobalSettings();
  void saveGlobalSettings();

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
  DataBlock propPanelStateOfTheAssetToInitiallySelect;
  PropPanel::PanelWindowPropertyControl *mPropPanel;
  PropPanel::ContainerPropertyControl *mToolPanel;
  PropPanel::ContainerPropertyControl *mPluginTool;
  PropPanel::ContainerPropertyControl *themeSwitcherToolPanel = nullptr;

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
  bool developerToolsEnabled = false;
  TexQL curMaxTexQL = TQL_uhq;

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
  void createThemeSwitcherToolbar();

  void renderUIViewport(ViewportWindow &viewport, const Point2 &size, float item_spacing, bool vr_mode);
  void renderUIViewports(bool vr_mode);
  void renderUI();

  // IMainWindowImguiRenderingService
  void beforeUpdateImgui() override;
  void updateImgui() override;

  // PropPanel::IDelayedCallbackHandler
  void onImguiDelayedCallback(void *user_data) override;

  enum class AssetBuildWarningDisplay
  {
    ShowWhenBuilding,
    ShowOncePerSession,
    ShowWhenSelected,
  };

  static const int LATEST_DOCK_SETTINGS_VERSION = 1; // Increasing this will reset the dock settings.

  Point2 viewportSplitRatio = Point2(0.5f, 0.5f);
  bool makingDefaultLayout = false;
  bool dockPositionsInitialized = false;
  bool consoleCommandsAndVariableWindowsVisible = false;
  bool consoleWindowVisible = false;
  bool imguiDebugWindowsVisible = false;
  bool useDngBasedSceneRender = false;
  AssetBuildWarningDisplay assetBuildWarningDisplay = AssetBuildWarningDisplay::ShowWhenBuilding;
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
