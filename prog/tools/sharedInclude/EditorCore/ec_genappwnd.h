// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_decl.h>

#include <EditorCore/ec_geneditordata.h>
#include <EditorCore/ec_gridobject.h>
#include <EditorCore/ec_workspace.h>
#include <EditorCore/ec_wndPublic.h>
#include <EditorCore/ec_gizmoSettings.h>
#include <EditorCore/ec_screenshot.h>

#include <util/dag_string.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/util/undo.h>

#include <math/dag_TMatrix.h>
#include <math/integer/dag_IPoint2.h>
#include <drv/3d/dag_resId.h>

#include <coolConsole/coolConsole.h>

#include <EASTL/unique_ptr.h>

namespace PropPanel
{
class IMenu;
}

/// Editor's main window.
/// In EditorCore based editors main window is usually derived from
/// GenericEditorAppWindow and IEditorCoreEngine classes.
/// @ingroup EditorCore
/// @ingroup EditorBaseClasses
/// @ingroup AppWindow
class GenericEditorAppWindow : public PropPanel::IMenuEventHandler, public IUndoRedoWndClient
{
public:
  /// Constructor.
  /// @param[in] open_fname - path to the project to open on application start
  GenericEditorAppWindow(const char *open_fname, IWndManager *manager);
  /// Destructor.
  virtual ~GenericEditorAppWindow();

  /// Test whether application window may be closed
  /// (that is the editor may be exited).
  /// @return @b true if application window may be closed, @b false in other case
  virtual bool canClose();

  /// Show dialog window on application start, where one may select
  /// start up action:.
  /// \n - Create new project
  /// \n - Open existing project
  /// \n - Open recent project
  virtual void startWith(const char *select_workspace = nullptr);

  /// update Undo / Redo menu items.
  /// update undo / redo menu text - set names for undo / redo operations.
  void updateUndoRedoMenu() override;

  /// Save viewports parameters to BLK file.
  /// @param[in] blk - Data Block that contains data to save (see DataBlock)
  void saveViewportsParams(DataBlock &blk) const;

  /// Load viewports parameters from BLK file.
  /// @param[in] blk - Data Block that contains data to load (see DataBlock)
  void loadViewportsParams(const DataBlock &blk);

  void setQuietMode(bool mode) { quietMode = mode; };

  // theme settings
  String getThemeFileName() const;
  void saveThemeSettings(DataBlock &blk) const;
  void loadThemeSettings(const DataBlock &blk);

  static constexpr const char *defaultThemeName = "light";

  // save/load screenshot options
  void saveScreenshotSettings(DataBlock &blk) const;
  void loadScreenshotSettings(const DataBlock &blk);

  bool isRenderingOrtScreenshot() { return isRenderingOrtScreenshot_; }
  bool isOrthogonalPreview() { return mScrCells.sizeInTiles != IPoint2(0, 0); }

  EditorWorkspace &getWorkspace() { return mWSpace; }

  class AppEventHandler;
  struct OrtMultiScrData
  {
    OrtMultiScrData() :
      useIt(false), renderObjects(false), res(1.0), mapPos(-1000, -1000), mapSize(2000, 2000), tileSize(1024), q_type(0), mipLevels(0)
    {}

    bool useIt, renderObjects;
    Point2 mapPos, mapSize;
    int tileSize;
    float res;
    int q_type;
    int mipLevels;
  };

  struct OrtScrCells
  {
    OrtScrCells() { reset(); }
    void reset()
    {
      mapPos = mapSize = tileInMeters = Point2(0, 0);
      sizeInTiles = IPoint2(0, 0);
    }

    Point2 mapPos, mapSize;
    Point2 tileInMeters;
    IPoint2 sizeInTiles;
  };

protected:
  IWndManager *mManager;

  GizmoEventFilter *gizmoEH;
  BrushEventFilter *brushEH;
  IGenEventHandler *appEH;

  char sceneFname[260];
  bool shouldLoadFile;

  bool shouldUpdateViewports;

  UndoSystem *undoSystem;

  struct WorldCursorParams
  {
    Point3 pt, norm;
    bool visible, xz_based;
    TEXTUREID texId;
    real size;
  } cursor, cursorLast;

  GeneralEditorData ged;
  GridObject grid;

  CoolConsole *console;
  bool quietMode;

  // theme settings
  String themeName;

  // screenshot settings
  ScreenshotConfig screenshotCfg;
  ScreenshotConfig cubeScreenshotCfg;

  OrtMultiScrData mScrData;
  OrtScrCells mScrCells;

  bool isRenderingOrtScreenshot_;

  // user-overridable routines
  virtual bool handleNewProject(bool edit = false) = 0;
  virtual bool handleOpenProject(bool edit = false) = 0;

  virtual void getDocTitleText(String &text);
  virtual void setDocTitle();

  virtual bool createNewProject(const char *filename) = 0;
  virtual bool loadProject(const char *filename) = 0;
  virtual bool saveProject(const char *filename) = 0;

  virtual bool canCloseScene(const char *title);

  // helper routines
  virtual void init(const char *select_workspace = nullptr);

  PropPanel::IMenu *getMainMenu();

  virtual void fillMenu(PropPanel::IMenu *menu);
  virtual void updateMenu([[maybe_unused]] PropPanel::IMenu *menu) {}

  virtual void addExitCommand(PropPanel::IMenu *menu);

  void onChangeFov();
  void onShowConsole();

  void fillCommonToolbar(PropPanel::ContainerPropertyControl &tb);

  // screenshot routine
  virtual String getScreenshotNameMask(bool cube) const = 0;
  virtual void screenshotRender(bool skip_debug_objects) = 0;

  String getScreenshotName(bool cube) const;
  void setScreenshotOptions(ScreenshotDlgMode mode);
  void closeScreenshotSettingsDialog();

  Texture *renderInTex(int w, int h, const TMatrix *tm, bool skip_debug_objects = false,
    bool should_make_orthogonal_screenshot = false, bool should_use_z_buffer = true, float world_x0 = 0.f, float world_x1 = 0.f,
    float world_z0 = 0.f, float world_z1 = 0.f);

  void createScreenshot();
  void createCubeScreenshot();
  void createOrthogonalScreenshot(const char *dest_folder = NULL, const float *x0z0x1z1 = NULL);
  void saveOrthogonalScreenshot(const char *fn, int mip_level, float _x, float _z, float tile_in_meters);
  void renderOrtogonalCells();
  int getMaxRTSize();

  // IMenuEventHandler
  int onMenuItemClick(unsigned id) override;

private:
  class FovDlg;

  EditorWorkspace mWSpace;
  eastl::unique_ptr<PropPanel::IMenu> mainMenu;
};
