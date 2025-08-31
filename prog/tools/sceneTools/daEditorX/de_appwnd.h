// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>

#include <propPanel/c_control_event_handler.h>
#include <propPanel/messageQueue.h>

#include <libTools/util/filePathname.h>

#include <coolConsole/coolConsole.h>

#include <EditorCore/ec_geneditordata.h>
#include <EditorCore/ec_genappwnd.h>
#include <EditorCore/ec_wndPublic.h>

#include <generic/dag_tab.h>

#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <util/dag_fastIntList.h>
#include <util/dag_fastPtrList.h>
#include <util/dag_oaHashNameMap.h>

#include <shaders/dag_IRenderWorld.h>
#include <shaders/dag_postFxRenderer.h>

#define HOTKEY_COUNT (16 + 1)


class AboutDlg;
class IWaterService;
class StartupDlg;

enum
{
  GUI_MAIN_TOOLBAR_ID,
  GUI_PLUGIN_TOOLBAR_ID,
};

enum
{
  MAX_PLUGIN_COUNT = 100,
  TAB_PANEL_ID = 99000,
  TAB_BASE_ID,
  TAB_TOOLBAR_BASE_ID = TAB_BASE_ID + MAX_PLUGIN_COUNT,
};

class CToolWindow;

struct Plugin
{
  IGenEditorPlugin *p;
  DagorEdPluginData *data;
};

struct UnLoadedPlugin
{
  IGenEditorPlugin *p;
  String name;
  String correctName;
  bool hasBinaryData;
};


struct PlatformExportPath
{
  int platfId;
  String path;
};


extern const char *hotkey_names[];
extern int hotkey_values[];
extern const char *hotkey_int_to_char(int hk);
extern void (*on_batch_exit)();


class TextureRemapHelper : public ITextureNumerator
{
public:
  TextureRemapHelper(int target);
  ~TextureRemapHelper();

  void setupTexQualityFromLevelBlk(const DataBlock &level_blk);
  int getTexQ(const char *tex_name) const;
  int getCustomMetricsTexCount() const { return customMetricsTexCount; }
  float getCustomMetricsTexSizeMb() const { return customMetricsTexSizeMb; }

  bool saveTextures(mkbindump::BinDumpSaveCB &cwr, const char *exp_fname_without_ext, ILogWriter &log) const;
  unsigned getTexturesSize(unsigned target) const;
  int getTexturesCount() const { return texname.nameCount(); }
  const char *getTextureName(int ordinal) { return texname.getName(ordinal); }

  int getDDSxTexturesCount() const { return ddsxTexName.nameCount(); }
  const char *getDDSxTextureName(int i) const { return ddsxTexName.getName(i); }
  int getDDSxTextureSize(int i) const;

  int addTextureName(const char *fname) override;
  int addDDSxTexture(const char *fname, ddsx::Buffer &b) override;
  int getTextureOrdinal(const char *fname) const override;

  int getTargetCode() const override { return targetCode; }

private:
  NameMap texname;
  NameMap ddsxTexName;
  NameMap lqTexNames, mqTexNames;
  int customMetricsTexCount = -1;
  float customMetricsTexSizeMb = -1.0f;
  Tab<ddsx::Buffer> ddsxTex;
  int targetCode;
  bool validateTexture_(const char *file_name);
};


class DagorEdAppWindow : public GenericEditorAppWindow,
                         public IDagorEd2Engine,
                         public IConsoleCmd,
                         public PropPanel::ControlEventHandler,
                         public IWndManagerWindowHandler,
                         public PropPanel::IDelayedCallbackHandler,
                         public IMainWindowImguiRenderingService
{
  friend class DagorEdAppEventHandler;

public:
  DagorEdAppWindow(IWndManager *manager, const char *open_fname);
  ~DagorEdAppWindow() override;

  // GenericEditorAppWindow
  void init(const char *select_workspace = nullptr) override;
  bool canClose() override;

  // IDagorEd2Engine
  // ==========================================================================
  const char *getSdkDir() const override;
  const char *getGameDir() const override;
  const char *getLibDir() const override;
  const char *getSceneFileName() const override { return sceneFname; }
  const char *getScriptLibrary() const override;
  float getMaxTraceDistance() const override;
  dag::ConstSpan<unsigned> getAdditionalPlatforms() const override;

  void addExportPath(int platf_id) override;
  const char *getExportPath(int platf_id) const override;

  const DeWorkspace &getWorkspace() const override;
  const EditorWorkspace &getBaseWorkspace() override;

  bool isInBatchOp() override { return on_batch_exit != NULL; }

  void disablePluginsRender() override;
  void enablePluginsRender() override;

  void preparePluginsListmenu() override;
  void startWithWorkspace(const char *def_workspace_name) override;

  int getPluginCount() override;
  IGenEditorPlugin *getPlugin(int idx) override;
  DagorEdPluginData *getPluginData(int idx) override;
  IGenEditorPlugin *curPlugin() override { return (curPluginId < 0) ? NULL : plugin[curPluginId].p; }
  IGenEditorPlugin *getPluginByName(const char *name) const override;
  IGenEditorPlugin *getPluginByMenuName(const char *name) const override;

  // project files management
  void getPluginProjectPath(const IGenEditorPlugin *plugin, String &base_path) const override;
  String getPluginFilePath(const IGenEditorPlugin *plugin, const char *fname) const override;
  void getProjectFolderPath(String &base_path) override;
  String getProjectFileName() override;

  bool copyFileToProject(const char *from_filename, const char *to_filename, bool overwrite, bool add_to_cvs) override;

  bool addFileToProject(const char *filename) override;
  bool removeFileFromProject(const char *filename, bool remove_from_cvs) override;

  // addition viewport camera interface
  void setViewportCameraMode(unsigned viewport_no, bool camera_mode) override;
  void setViewportCameraViewProjection(unsigned viewport_no, TMatrix &view, float fov) override;
  void switchCamera(const unsigned int from, const unsigned int to) override;
  void setViewportCustomCameras(ICustomCameras *customCameras) override;

  void updateViewports() override;
  void setViewportCacheMode(ViewportCacheMode mode) override;
  void invalidateViewportCache() override;
  void setAnimateViewports(bool animate) override;

  // static geometry lighting routine
  bool isUseDirectLight() const override { return useDirectLight; }
  void setUseDirectLight(bool use) override { useDirectLight = use; }

  // custom colliders
  void registerCustomCollider(IDagorEdCustomCollider *coll) const override;
  void unregisterCustomCollider(IDagorEdCustomCollider *coll) const override;
  void enableCustomShadow(const char *name) const override;
  void disableCustomShadow(const char *name) const override;
  void enableCustomCollider(const char *name) const override;
  void disableCustomCollider(const char *name) const override;

  bool isCustomShadowEnabled(const IDagorEdCustomCollider *collider) const override;
  int getCustomCollidersCount() const override;
  IDagorEdCustomCollider *getCustomCollider(int idx) const override;
  bool fillCustomCollidersList(PropPanel::ContainerPropertyControl &panel, const char *grp_caption, int grp_pid, int collider_pid,
    bool shadow, bool open_grp) const override;

  bool onPPColliderCheck(int pid, const PropPanel::ContainerPropertyControl &panel, int collider_pid, bool shadow) const override;

  bool getUseOnlyVisibleColliders() const override;
  void setUseOnlyVisibleColliders(bool use) override;

  void setColliders(dag::ConstSpan<IDagorEdCustomCollider *> c, unsigned filter_mask) const override;
  void restoreEditorColliders() const override;

  dag::ConstSpan<IDagorEdCustomCollider *> loadColliders(const DataBlock &blk, unsigned &out_filter_mask,
    const char *blk_name = "colliders") const override;

  dag::ConstSpan<IDagorEdCustomCollider *> getCurColliders(unsigned &out_filter_mask) const override;
  void saveColliders(DataBlock &blk, dag::ConstSpan<IDagorEdCustomCollider *> coll, unsigned filter_mask,
    bool save_filters = true) const override;

  // repaints viewports
  void repaint() override;

  void correctCursorInSurfMove(const Point3 &delta) override;
  bool shadowRayHitTest(const Point3 &src, const Point3 &dir, real dist) override;
  int getNextUniqueId() override;
  bool spawnEvent(unsigned event_huid, void *user_data) override;
  // ==========================================================================


  // IEditorCoreEngine
  // ==========================================================================
  // register/unregister plugins (every plugin should be registered once)
  bool registerDllPlugin(IGenEditorPlugin *plugin, void *dll_handle, const char *dll_path);
  bool registerPlugin(IGenEditorPlugin *plugin) override;
  bool unregisterPlugin(IGenEditorPlugin *plugin) override;

  void *getInterface(int huid) override;
  void getInterfaces(int huid, Tab<void *> &interfaces) override;

  IWndManager *getWndManager() const override;

  PropPanel::ContainerPropertyControl *getCustomPanel(int id) const override;

  void addPropPanel(int type, hdpi::Px width) override;
  void removePropPanel(void *hwnd) override;
  void managePropPanels() override;
  void skipManagePropPanels(bool skip) override;
  PropPanel::PanelWindowPropertyControl *createPropPanel(PropPanel::ControlEventHandler *eh, const char *caption) override;
  PropPanel::IMenu *getMainMenu() override;

  void deleteCustomPanel(PropPanel::ContainerPropertyControl *panel) override;

  PropPanel::DialogWindow *createDialog(hdpi::Px w, hdpi::Px h, const char *title) override;
  void deleteDialog(PropPanel::DialogWindow *dlg) override;

  // viewport methods
  int getViewportCount() override;
  IGenViewportWnd *getViewport(int n) override;
  IGenViewportWnd *getRenderViewport() override;
  IGenViewportWnd *getCurrentViewport() override;

  void setViewportZnearZfar(real zn, real zf) override;

  // trace functions
  IGenViewportWnd *screenToViewport(int &x, int &y) override;
  bool screenToWorldTrace(int x, int y, Point3 &world, real maxdist = 1000, Point3 *out_norm = NULL) override;
  bool clientToWorldTrace(IGenViewportWnd *wnd, int x, int y, Point3 &world, float maxdist = 1000, Point3 *out_norm = NULL) override;
  void setupColliderParams(int mode, const BBox3 &area) override;
  bool traceRay(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm = NULL, bool use_zero_plane = true) override;
  float clipCapsuleStatic(const Capsule &c, Point3 &cap_pt, Point3 &world_pt) override;

  bool getSelectionBox(BBox3 &box) override;

  void zoomAndCenter() override;

  // gizmo methods
  void setGizmo(IGizmoClient *gc, ModeType type) override;
  void startGizmo(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void endGizmo(bool apply) override;
  ModeType getGizmoModeType() override;
  BasisType getGizmoBasisType() override;
  BasisType getGizmoBasisTypeForMode(ModeType tp) override;
  CenterType getGizmoCenterType() override;
  CenterType getGizmoCenterTypeForMode(ModeType tp) override;
  bool isGizmoOperationStarted() const override;

  // brush routines
  void beginBrushPaint() override;
  void renderBrush() override;
  void setBrush(Brush *brush) override;
  Brush *getBrush() const override;
  void endBrushPaint() override;
  bool isBrushPainting() const override;

  // spatial cursor handling
  void showUiCursor(bool vis) override;
  void setUiCursorPos(const Point3 &pos, const Point3 *norm = NULL) override;
  void getUiCursorPos(Point3 &pos, Point3 &norm) override;
  void setUiCursorTex(TEXTUREID tex_id) override;
  void setUiCursorProps(float size, bool always_xz) override;

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

  void actObjects(float dt) override;
  void beforeRenderObjects() override;
  void renderObjects() override;
  void renderTransObjects() override;
  void renderIslDecalObjects() override {}

  void *queryEditorInterfacePtr(unsigned huid) override;
  // ==========================================================================

  void renderGrid();

  ICollision *getCollision();


  void getSortedListForBuild(Tab<IGenEditorPlugin *> &list, Tab<bool *> &export_list, Tab<bool *> &extern_list);

  void getFiltersList(Tab<IGenEditorPlugin *> &list, Tab<bool *> &use);

  void onTabSelChange(int id);

  void screenshotRender(bool skip_debug_objects) override;

  static void splitProjectFilename(const char *filename, String &path, String &name);
  bool loadWorkspace(const char *wsp_name);
  DeWorkspace *getWorkspace() { return wsp; }

  void onBeforeReset3dDevice();
  bool onDropFiles(const dag::Vector<String> &files);

  bool forceSaveProject();
  bool reloadProject();

  // IConsoleCmd
  bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params) override;
  const char *onConsoleCommandHelp(const char *cmd) override;

  // IWndManagerWindowHandler
  void *onWmCreateWindow(int type) override;
  bool onWmDestroyWindow(void *window) override;

  void setShowMessageAt(int x, int y, const SimpleString &msg) override;
  void showMessageAt() override;
  using GenericEditorAppWindow::renderInTex;

protected:
  enum class ModelessWindowResetReason
  {
    LoadingProject,
    ResettingLayout,
    ExitingApplication,
  };

  Tab<Plugin> plugin;
  Tab<int> panelsToAdd;
  Tab<int> panelsToAddWidth;
  Tab<void *> panelsToDelete;
  bool panelsSkipManage;
  int curPluginId;

  int lastUniqueId;

  DeWorkspace *wsp;
  bool animateViewports;
  bool shouldUseOccluders;

  SimpleString redirectPathTo;
  FastPtrList redirectPluginList;

  PostFxRenderer clearProc;

  // GenericEditorAppWindow
  void startWith(const char *) override {}

  bool handleNewProject(bool edit = false) override;
  bool handleOpenProject(bool edit = false) override;

  bool createNewProject(const char *filename) override;
  bool loadProject(const char *filename) override;
  bool saveProject(const char *filename) override;

  String getScreenshotNameMask(bool cube) const override;

  void getDocTitleText(String &text) override;
  bool canCloseScene(const char *title) override;

  void fillMenu(PropPanel::IMenu *menu) override;
  void updateMenu(PropPanel::IMenu *menu) override;

  int onMenuItemClick(unsigned id) override;

  // ControlEventHandler
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void prepareLocalLevelBlk(const char *filename, String &localSetPath, DataBlock &localBlk);
  void autoSaveProject(const char *filename);

  void handleSaveAsProject();

  bool canCreateNewProject(const char *filename);


  void editorHelp() const;
  void pluginHelp(const char *url) const;

  int findPlugin(IGenEditorPlugin *p);
  void sortPlugins();
  void switchToPlugin(int plgId);

  void initPlugins(const DataBlock &global_settings, const DataBlock &per_app_settings);
  void fillPluginSettings();
  void fillPluginTabs();
  void switchPluginTab(bool next = true);

  void resetModelessWindows(ModelessWindowResetReason reset_reason);
  void terminateInterface();
  void resetCore();

  bool selectWorkspace(const char *app_blk_path);

  bool gatherUsedResStats(dag::ConstSpan<IBinaryDataBuilder *> exp, unsigned target_code, const OAHashNameMap<true> &resList,
    TextureRemapHelper &out_trh, int64_t &out_texSize, int &out_texCount, bool verbose_to_console = false);
  void exportLevelToGame(int target_code);
  void exportLevelToDag(bool visual = true);

  bool traceZeroLevelPlane(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm);
  bool traceWaterPlane(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm);

  static int plugin_cmp_render_order(const Plugin *p1, const Plugin *p2);

  static FastNameMap plugin_forced_export_order;
  static int plugin_cmp_build_order(IGenEditorPlugin *const *p1, IGenEditorPlugin *const *p2);

  bool showPluginDlg(Tab<IGenEditorPlugin *> &build_plugin, Tab<bool *> &do_export, Tab<bool *> &is_external);


  bool ignorePlugin(IGenEditorPlugin *p);

  void setEnabledColliders(const Tab<String> &names) const;
  void getEnabledColliders(Tab<String> &names) const;

private:
  ICollision *collision;
  AboutDlg *aboutDlg;

  Tab<PlatformExportPath> exportPaths;


  Tab<String> toDagPlugNames;
  Tab<String> backupPluginColliders;

  Tab<String> activeColliders;
  // if true, GeomObject uses direct light, otherwise GeomObject uses SH3Light
  bool useDirectLight;

  void renderUIViewport(ViewportWindow &viewport, const Point2 &size, float item_spacing);
  void renderUIViewports();
  void renderUI();

  // IMainWindowImguiRenderingService
  void beforeUpdateImgui() override;
  void updateImgui() override;

  // PropPanel::IDelayedCallbackHandler
  void onImguiDelayedCallback(void *user_data) override;

  void showStats();

  void registerConsoleCommands();
  bool runExitCmd(dag::ConstSpan<const char *> params);
  bool runCameraPosCmd(dag::ConstSpan<const char *> params);
  bool runCameraDirCmd(dag::ConstSpan<const char *> params);
  bool runProjectOpenCmd(dag::ConstSpan<const char *> params);
  bool runProjectExportCmd(dag::ConstSpan<const char *> params, int export_fmt);
  bool runProjectExportCmd(dag::ConstSpan<const char *> params);
  bool runSetWorkspaceCmd(dag::ConstSpan<const char *> params);
  bool runShadersListVars(dag::ConstSpan<const char *> params);
  bool runShadersSetVar(dag::ConstSpan<const char *> params);
  bool runShadersReload(dag::ConstSpan<const char *> params);

  void copyFiles(const FilePathName &from, const FilePathName &to, bool overwrite);

  // Name of the application being edited.
  // For example it is "enlisted" for "d:\dagor\enlisted\application.blk".
  // It returns with an empty string if there is no loaded workspace.
  String getApplicationName() const;

  // Full path of the per application settings DataBlock file.
  // For example: "d:\dagor\tools\dagor_cdk\.local\de3_settings_enlisted.blk".
  // It returns with an empty string if there is no loaded workspace.
  String getPerApplicationSettingsBlkPath() const;

  void initDllPlugins(const char *plug_dir);

  void updateRecentMenu();
  void addToRecentList(const char *fname, bool menu_update = true);

  void updateExportMenuItems();

  void updateThemeItems();

  void updateUndoRedoMenu() override;

  void updateUseOccluders();


  void makeDefaultLayout();
  void fillMainToolBar();

  void addCameraAccelerators();
  void addEditorAccelerators();

  // From Editor means that not from the initial project selector dialog.
  void loadProjectFromEditor(const char *path);

  static bool gracefulFatalExit(const char *msg, const char *call_stack, const char *file, int line);

  static const int LATEST_DOCK_SETTINGS_VERSION = 1; // Increasing this will reset the dock settings.

  PropPanel::ContainerPropertyControl *mToolPanel, *mTabWindow, *mPlugTools;
  PropPanel::ContainerPropertyControl *mTabPanel;

  bool mNeedSuppress;
  int mMsgBoxResult;
  String mSuppressDlgResult;

  bool mPluginShowDialogIsShow;
  bool developerToolsEnabled = false;

  IWaterService *waterService;
  bool noWaterService;

  StartupDlg *startupDlgShown = nullptr;
  DataBlock screenshotMetaInfoToApply;

  int msgX, msgY;
  SimpleString messageAt;

  Point2 viewportSplitRatio = Point2(0.5f, 0.5f);
  bool dockPositionsInitialized = false;
  bool consoleCommandsAndVariableWindowsVisible = false;
  bool consoleWindowVisible = false;
  bool imguiDebugWindowsVisible = false;

  bool skipRenderObjects = false;
};

void send_event_error(const char *s, const char *callstack);
