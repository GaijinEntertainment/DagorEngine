// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_interface.h>
#include <oldEditor/de_common_interface.h>

#include <sepGui/wndPublic.h>
#include <propPanel/c_control_event_handler.h>
#include <propPanel/messageQueue.h>

#include <libTools/util/filePathname.h>

#include <coolConsole/coolConsole.h>

#include <EditorCore/ec_geneditordata.h>
#include <EditorCore/ec_genappwnd.h>

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
  bool saveTextures(mkbindump::BinDumpSaveCB &cwr, const char *exp_fname_without_ext, ILogWriter &log) const;
  unsigned getTexturesSize(unsigned target) const;
  int getTexturesCount() const { return texname.nameCount(); }
  const char *getTextureName(int ordinal) { return texname.getName(ordinal); }

  int getDDSxTexturesCount() const { return ddsxTexName.nameCount(); }
  const char *getDDSxTextureName(int i) const { return ddsxTexName.getName(i); }
  int getDDSxTextureSize(int i) const;

  virtual int addTextureName(const char *fname);
  virtual int addDDSxTexture(const char *fname, ddsx::Buffer &b);
  virtual int getTextureOrdinal(const char *fname) const;

  virtual int getTargetCode() const { return targetCode; }

private:
  NameMap texname;
  NameMap ddsxTexName;
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
  ~DagorEdAppWindow();

  // GenericEditorAppWindow
  virtual void init();
  virtual bool canClose();

  // IDagorEd2Engine
  // ==========================================================================
  virtual const char *getSdkDir() const;
  virtual const char *getGameDir() const;
  virtual const char *getLibDir() const;
  virtual const char *getSceneFileName() const { return sceneFname; }
  virtual const char *getSoundFileName() const;
  virtual const char *getSoundFxFileName() const;
  virtual const char *getScriptLibrary() const;
  virtual float getMaxTraceDistance() const;
  virtual dag::ConstSpan<unsigned> getAdditionalPlatforms() const;

  virtual void addExportPath(int platf_id);
  virtual const char *getExportPath(int platf_id) const;

  virtual const DeWorkspace &getWorkspace() const;
  virtual const EditorWorkspace &getBaseWorkspace();

  virtual bool isInBatchOp() { return on_batch_exit != NULL; }

  virtual void disablePluginsRender();
  virtual void enablePluginsRender();

  virtual void preparePluginsListmenu();
  virtual void startWithWorkspace(const char *def_workspace_name);

  virtual int getPluginCount();
  virtual IGenEditorPlugin *getPlugin(int idx);
  virtual DagorEdPluginData *getPluginData(int idx);
  virtual IGenEditorPlugin *curPlugin() { return (curPluginId < 0) ? NULL : plugin[curPluginId].p; }
  virtual IGenEditorPlugin *getPluginByName(const char *name) const;
  virtual IGenEditorPlugin *getPluginByMenuName(const char *name) const;

  // project files management
  virtual void getPluginProjectPath(const IGenEditorPlugin *plugin, String &base_path) const;
  virtual String getPluginFilePath(const IGenEditorPlugin *plugin, const char *fname) const;
  virtual void getProjectFolderPath(String &base_path);
  virtual String getProjectFileName();

  virtual bool copyFileToProject(const char *from_filename, const char *to_filename, bool overwrite, bool add_to_cvs);

  virtual bool addFileToProject(const char *filename);
  virtual bool removeFileFromProject(const char *filename, bool remove_from_cvs);

  // addition viewport camera interface
  virtual void setViewportCameraMode(unsigned viewport_no, bool camera_mode);
  virtual void setViewportCameraViewProjection(unsigned viewport_no, TMatrix &view, float fov);
  virtual void switchCamera(const unsigned int from, const unsigned int to);
  virtual void setViewportCustomCameras(ICustomCameras *customCameras);

  virtual void updateViewports();
  virtual void setViewportCacheMode(ViewportCacheMode mode);
  virtual void invalidateViewportCache();
  virtual void setAnimateViewports(bool animate);

  // static geometry lighting routine
  virtual bool isUseDirectLight() const { return useDirectLight; }
  virtual void setUseDirectLight(bool use) { useDirectLight = use; }

  // custom colliders
  virtual void registerCustomCollider(IDagorEdCustomCollider *coll) const;
  virtual void unregisterCustomCollider(IDagorEdCustomCollider *coll) const;
  virtual void enableCustomShadow(const char *name) const;
  virtual void disableCustomShadow(const char *name) const;
  virtual void enableCustomCollider(const char *name) const;
  virtual void disableCustomCollider(const char *name) const;

  virtual bool isCustomShadowEnabled(const IDagorEdCustomCollider *collider) const;
  virtual int getCustomCollidersCount() const;
  virtual IDagorEdCustomCollider *getCustomCollider(int idx) const;
  virtual bool fillCustomCollidersList(PropPanel::ContainerPropertyControl &panel, const char *grp_caption, int grp_pid,
    int collider_pid, bool shadow, bool open_grp) const;

  virtual bool onPPColliderCheck(int pid, const PropPanel::ContainerPropertyControl &panel, int collider_pid, bool shadow) const;

  virtual bool getUseOnlyVisibleColliders() const;
  virtual void setUseOnlyVisibleColliders(bool use);

  virtual void setColliders(dag::ConstSpan<IDagorEdCustomCollider *> c, unsigned filter_mask) const;
  virtual void restoreEditorColliders() const;

  virtual dag::ConstSpan<IDagorEdCustomCollider *> loadColliders(const DataBlock &blk, unsigned &out_filter_mask,
    const char *blk_name = "colliders") const;

  virtual dag::ConstSpan<IDagorEdCustomCollider *> getCurColliders(unsigned &out_filter_mask) const;
  virtual void saveColliders(DataBlock &blk, dag::ConstSpan<IDagorEdCustomCollider *> coll, unsigned filter_mask,
    bool save_filters = true) const;

  // repaints viewports
  virtual void repaint();

  virtual void correctCursorInSurfMove(const Point3 &delta);
  virtual bool shadowRayHitTest(const Point3 &src, const Point3 &dir, real dist);
  virtual int getNextUniqueId();
  virtual bool spawnEvent(unsigned event_huid, void *user_data);
  virtual unsigned getLeftDockNodeId() const override { return leftDockNodeId; }
  // ==========================================================================


  // IEditorCoreEngine
  // ==========================================================================
  // register/unregister plugins (every plugin should be registered once)
  virtual bool registerDllPlugin(IGenEditorPlugin *plugin, void *dll_handle);
  virtual bool registerPlugin(IGenEditorPlugin *plugin);
  virtual bool unregisterPlugin(IGenEditorPlugin *plugin);

  virtual void *getInterface(int huid);
  virtual void getInterfaces(int huid, Tab<void *> &interfaces);

  virtual IWndManager *getWndManager() const;

  virtual PropPanel::ContainerPropertyControl *getCustomPanel(int id) const;

  virtual void addPropPanel(int type, hdpi::Px width);
  virtual void removePropPanel(void *hwnd);
  virtual void managePropPanels();
  virtual void skipManagePropPanels(bool skip);
  virtual PropPanel::PanelWindowPropertyControl *createPropPanel(PropPanel::ControlEventHandler *eh, const char *caption) override;
  virtual PropPanel::IMenu *getMainMenu() override;

  void *addRawPropPanel(hdpi::Px width);

  virtual void deleteCustomPanel(PropPanel::ContainerPropertyControl *panel);

  virtual PropPanel::DialogWindow *createDialog(hdpi::Px w, hdpi::Px h, const char *title) override;
  virtual void deleteDialog(PropPanel::DialogWindow *dlg) override;

  // viewport methods
  virtual int getViewportCount();
  virtual IGenViewportWnd *getViewport(int n);
  virtual IGenViewportWnd *getRenderViewport();
  virtual IGenViewportWnd *getCurrentViewport();

  virtual void setViewportZnearZfar(real zn, real zf);

  // trace functions
  virtual IGenViewportWnd *screenToViewport(int &x, int &y);
  virtual bool screenToWorldTrace(int x, int y, Point3 &world, real maxdist = 1000, Point3 *out_norm = NULL);
  virtual bool clientToWorldTrace(IGenViewportWnd *wnd, int x, int y, Point3 &world, float maxdist = 1000, Point3 *out_norm = NULL);
  virtual void setupColliderParams(int mode, const BBox3 &area);
  virtual bool traceRay(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm = NULL, bool use_zero_plane = true);
  virtual float clipCapsuleStatic(const Capsule &c, Point3 &cap_pt, Point3 &world_pt);

  virtual bool getSelectionBox(BBox3 &box);

  virtual void zoomAndCenter();

  // gizmo methods
  virtual void setGizmo(IGizmoClient *gc, ModeType type);
  virtual void startGizmo(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual ModeType getGizmoModeType();
  virtual BasisType getGizmoBasisType();
  virtual BasisType getGizmoBasisTypeForMode(ModeType tp);
  virtual CenterType getGizmoCenterType();
  virtual bool isGizmoOperationStarted() const override;

  // brush routines
  virtual void beginBrushPaint();
  virtual void renderBrush();
  virtual void setBrush(Brush *brush);
  virtual Brush *getBrush() const;
  virtual void endBrushPaint();
  virtual bool isBrushPainting() const;

  // spatial cursor handling
  virtual void showUiCursor(bool vis);
  virtual void setUiCursorPos(const Point3 &pos, const Point3 *norm = NULL);
  virtual void getUiCursorPos(Point3 &pos, Point3 &norm);
  virtual void setUiCursorTex(TEXTUREID tex_id);
  virtual void setUiCursorProps(float size, bool always_xz);

  virtual void showSelectWindow(IObjectsList *obj_list, const char *obj_list_owner_name);

  // virtual DynRenderBuffer *getDRB() { return drb; }
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

  virtual void actObjects(float dt);
  virtual void beforeRenderObjects();
  virtual void renderObjects();
  virtual void renderTransObjects();
  virtual void renderIslDecalObjects() {}

  virtual void *queryEditorInterfacePtr(unsigned huid);
  // ==========================================================================

  virtual void renderGrid();

  virtual IClipping *getClipping();


  void getSortedListForBuild(Tab<IGenEditorPlugin *> &list, Tab<bool *> &export_list, Tab<bool *> &extern_list);

  void getFiltersList(Tab<IGenEditorPlugin *> &list, Tab<bool *> &use);

  void onTabSelChange(int id);

  virtual void screenshotRender();

  static void splitProjectFilename(const char *filename, String &path, String &name);
  bool loadWorkspace(const char *wsp_name);
  DeWorkspace *getWorkspace() { return wsp; }

  void onBeforeReset3dDevice();
  bool onDropFiles(const dag::Vector<String> &files);

  bool forceSaveProject();
  bool reloadProject();

  // IConsoleCmd
  virtual bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params);
  virtual const char *onConsoleCommandHelp(const char *cmd);

  // IWndManagerWindowHandler
  virtual void *onWmCreateWindow(int type) override;
  virtual bool onWmDestroyWindow(void *window) override;

  virtual void setShowMessageAt(int x, int y, const SimpleString &msg);
  virtual void showMessageAt();
  using GenericEditorAppWindow::renderInTex;

protected:
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
  virtual void startWith() {}

  virtual bool handleNewProject(bool edit = false);
  virtual bool handleOpenProject(bool edit = false);

  virtual bool createNewProject(const char *filename);
  virtual bool loadProject(const char *filename);
  virtual bool saveProject(const char *filename);

  virtual String getScreenshotNameMask(bool cube) const;

  virtual void getDocTitleText(String &text);
  virtual bool canCloseScene(const char *title);

  virtual void fillMenu(PropPanel::IMenu *menu) override;
  virtual void updateMenu(PropPanel::IMenu *menu) override;

  virtual int onMenuItemClick(unsigned id);

  // ControlEventHandler
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);

  void prepareLocalLevelBlk(const char *filename, String &localSetPath, DataBlock &localBlk);
  void autoSaveProject(const char *filename);

  void handleSaveAsProject();

  bool canCreateNewProject(const char *filename);


  void editorHelp() const;
  void pluginHelp(const char *url) const;

  int findPlugin(IGenEditorPlugin *p);
  void sortPlugins();
  void switchToPlugin(int plgId);

  void initPlugins(const DataBlock &global_settings);
  void fillPluginTabs();
  void switchPluginTab(bool next = true);

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
  IClipping *clipping;
  AboutDlg *aboutDlg;

  Tab<PlatformExportPath> exportPaths;


  Tab<String> toDagPlugNames;
  Tab<String> backupPluginColliders;

  Tab<String> activeColliders;
  // if true, GeomObject uses direct light, otherwise GeomObject uses SH3Light
  bool useDirectLight;

  void renderUIViewports();
  void renderUI();

  // IMainWindowImguiRenderingService
  virtual void beforeUpdateImgui() override;
  virtual void updateImgui() override;

  // PropPanel::IDelayedCallbackHandler
  virtual void onImguiDelayedCallback(void *user_data) override;

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

  void initDllPlugins(const char *plug_dir);

  void updateRecentMenu();
  void addToRecentList(const char *fname, bool menu_update = true);

  void updateExportMenuItems();

  void updateUndoRedoMenu();

  void updateUseOccluders();


  void makeDafaultLayout();
  void fillMainToolBar();

  void addCameraAccelerators();
  void addEditorAccelerators();

  static bool gracefulFatalExit(const char *msg, const char *call_stack, const char *file, int line);

  PropPanel::ContainerPropertyControl *mToolPanel, *mTabWindow, *mPlugTools;
  PropPanel::ContainerPropertyControl *mTabPanel;

  bool mNeedSuppress;
  int mMsgBoxResult;
  String mSuppressDlgResult;

  bool mPluginShowDialogIsShow;

  IWaterService *waterService;
  bool noWaterService;

  StartupDlg *startupDlgShown = nullptr;
  DataBlock screenshotMetaInfoToApply;

  int msgX, msgY;
  SimpleString messageAt;

  Point2 viewportSplitRatio = Point2(0.5f, 0.5f);
  bool dockPositionsInitialized = false;
  bool imguiDebugWindowsVisible = false;
  unsigned leftDockNodeId = 0; // ImGuiID
};

void send_event_error(const char *s, const char *callstack);
