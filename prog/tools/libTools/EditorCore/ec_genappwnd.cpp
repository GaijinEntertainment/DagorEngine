// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_genappwnd.h>
#include "ec_screenshotSettingsDialog.h"
#include "ec_gizmoRenderer.h"
#include <EditorCore/ec_cm.h>
#include <EditorCore/ec_brushfilter.h>
#include <EditorCore/ec_startDlg.h>
#include <EditorCore/ec_startup.h>
#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_wndGlobal.h>

#include <libTools/util/undo.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>
#include <libTools/util/iLogWriter.h>

#include <libTools/renderViewports/cachedViewports.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_basePath.h>

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_workCycle.h>
#include <debug/dag_debug.h>
#include <scene/dag_visibility.h>

#include <coolConsole/coolConsole.h>

#include <image/dag_tga.h>
#include <image/dag_jpeg.h>
#include <image/dag_png.h>
#include <image/dag_tiff.h>
#include <image/dag_texPixel.h>
#include <shaders/dag_shaderBlock.h>
#include <math/dag_cube_matrix.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_render.h>
#include <3d/ddsxTex.h>
#include <3d/dag_lockTexture.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <propPanel/colors.h>
#include <render/dag_cur_view.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <debug/dag_debug3d.h>
#include <startup/dag_globalSettings.h>

extern void update_visibility_finder(VisibilityFinder &vf);

#define ORTHOGONAL_SCREENSHOT_HEIGHT 1000.f

const float OS_PREVIEW_H = 10000.0;
const float OS_CAMERA_H = 1000.0;
const float OSGR_H = 5000.0;
const float OSSEL_H = OSGR_H;

extern CachedRenderViewports *ec_cached_viewports;
static const char EMPTY_PATH[] = "";
static const int TEX_SIZES_CNT = 8;
static int tex_sizes[TEX_SIZES_CNT] = {8192, 4096, 2048, 1024, 512, 256, 128, 64};
static const int TEX_QUALITY_TYPE_CNT = 4;
static const char *tex_qt[TEX_QUALITY_TYPE_CNT] = {"tga", "jpeg 100%", "jpeg 90%", "jpeg 40%"};
static eastl::unique_ptr<ScreenshotSettingsDialog> screenshot_settings_dialog;

enum
{
  SCR_TYPE_QUALITY_TGA = 0,
  SCR_TYPE_QUALITY_JPEG_100,
  SCR_TYPE_QUALITY_JPEG_90,
  SCR_TYPE_QUALITY_JPEG_40,
};

using hdpi::_pxScaled;

class OrthogonalScreenshotDlg : public PropPanel::DialogWindow
{
public:
  OrthogonalScreenshotDlg(const char caption[], GenericEditorAppWindow::OrtMultiScrData &scr_data,
    GenericEditorAppWindow::OrtScrCells &scr_cells, int max_rt_size, bool map2d_avail = false) :
    DialogWindow(IEditorCoreEngine::get()->getWndManager()->getMainWindow(), _pxScaled(DIALOG_WIDTH), _pxScaled(DIALOG_HEIGHT),
      caption),

    mScrData(&scr_data),
    mScrCells(&scr_cells),
    map2dAvail(map2d_avail),
    startTsInd(0)
  {
    multiScreenShot = map2dAvail ? mScrData->useIt : false;

    for (int i = 0; i < TEX_SIZES_CNT; ++i)
      if (tex_sizes[i] > max_rt_size)
        ++startTsInd;
    for (int i = startTsInd; i < TEX_SIZES_CNT; ++i)
      resLines.push_back().printf(0, "%dx%d", tex_sizes[i], tex_sizes[i]);
    shotSz = tex_sizes[startTsInd];
  }

  int showDialog() override
  {
    PropPanel::ContainerPropertyControl *_panel = getPanel();
    G_ASSERT(_panel && "No panel in GenericEditorAppWindow::OrthogonalScreenshotDlg");

    _panel->createEditFloat(MIN_X_ID, "Min. X", mScrData->mapPos.x);
    _panel->createEditFloat(MIN_Z_ID, "Min. Z", mScrData->mapPos.y);
    _panel->createEditFloat(SZ_X_ID, "Size X", mScrData->mapSize.x);
    _panel->createEditFloat(SZ_Z_ID, "Size Z", mScrData->mapSize.y);
    _panel->createCheckBox(MULTI_REND_OBJS, "Render objects", mScrData->renderObjects);

    if (map2dAvail)
    {
      _panel->createCheckBox(MULTI_SCREEN_ID, "Build for map2d", multiScreenShot);
      fillMultiPP(*_panel);
    }
    if (!multiScreenShot)
      _panel->createCombo(MULTI_TILE_SIZE_ID, "Resolution:", resLines, 0);

    updateScreenPrewiew(_panel);
    updateMipmapLevels(_panel);
    updateScreenPrewiew(_panel);
    return DialogWindow::showDialog();
  }

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override
  {
    switch (pcb_id)
    {
      case MULTI_SCREEN_ID:
        multiScreenShot = panel->getBool(MULTI_SCREEN_ID);
        fillMultiPP(*panel);
        break;

      case MULTI_RES_ID:
        if (panel->getFloat(MULTI_RES_ID) < 0.001)
          panel->setFloat(MULTI_RES_ID, 0.001);
        break;

      case SZ_X_ID:
      case SZ_Z_ID:
      {
        float val = panel->getFloat(pcb_id);
        if (val <= 0)
        {
          val = 0.1;
          panel->setFloat(pcb_id, val);
        }
      }
      break;

      case MULTI_REND_OBJS: mScrData->renderObjects = panel->getBool(MULTI_REND_OBJS); break;

      case MULTI_TILE_SIZE_ID:
        if (!multiScreenShot)
          shotSz = tex_sizes[startTsInd + panel->getInt(pcb_id)];
        break;
    }

    updateScreenPrewiew(panel);
    if (pcb_id != MULTI_TILE_MIPS)
      updateMipmapLevels(panel);
  }

  void fillMultiPP(PropPanel::ContainerPropertyControl &panel)
  {
    if (multiScreenShot && !panel.getById(MULTI_GRP_ID))
    {
      panel.removeById(MULTI_TILE_SIZE_ID);
      PropPanel::ContainerPropertyControl *grp = panel.createGroupBox(MULTI_GRP_ID, "Map2D screenshot params:");

      int sel = 0;
      for (int i = startTsInd; i < TEX_SIZES_CNT; ++i)
        if (mScrData->tileSize == tex_sizes[i])
          sel = i - startTsInd;
      grp->createCombo(MULTI_TILE_SIZE_ID, "Tile size:", resLines, sel);

      Tab<String> lines;
      for (int i = 0; i < TEX_QUALITY_TYPE_CNT; ++i)
        lines.push_back() = tex_qt[i];
      grp->createCombo(MULTI_TILE_QUALITY, "Type/quality:", lines, mScrData->q_type);

      grp->createEditFloat(MULTI_RES_ID, "Resolution (m/px)", mScrData->res);
      grp->setPrec(MULTI_RES_ID, 4);

      grp->createEditInt(MULTI_TILE_MIPS, "Mipmap levels", mScrData->mipLevels);
    }
    else if (!multiScreenShot && panel.getById(MULTI_GRP_ID))
    {
      panel.removeById(MULTI_GRP_ID);
      int sel = 0;
      for (int i = startTsInd; i < TEX_SIZES_CNT; ++i)
        if (shotSz == tex_sizes[i])
        {
          sel = i - startTsInd;
          break;
        }
      panel.createCombo(MULTI_TILE_SIZE_ID, "Resolution:", resLines, sel);
    }
  }

  bool onOk() override
  {
    PropPanel::ContainerPropertyControl *_panel = getPanel();
    G_ASSERT(_panel && "No panel in GenericEditorAppWindow::OrthogonalScreenshotDlg");
    mScrData->mapPos = Point2(_panel->getFloat(MIN_X_ID), _panel->getFloat(MIN_Z_ID));
    mScrData->mapSize = Point2(_panel->getFloat(SZ_X_ID), _panel->getFloat(SZ_Z_ID));
    mScrData->useIt = multiScreenShot;

    if (multiScreenShot)
    {
      mScrData->tileSize = getTexSize(_panel);
      mScrData->res = _panel->getFloat(MULTI_RES_ID);
      mScrData->q_type = _panel->getInt(MULTI_TILE_QUALITY);
      updateMipmapLevels(_panel);
      mScrData->mipLevels = _panel->getInt(MULTI_TILE_MIPS);
    }

    return true;
  }

  void updateMipmapLevels(PropPanel::ContainerPropertyControl *panel)
  {
    int max_size = max(mScrCells->sizeInTiles.x, mScrCells->sizeInTiles.y);
    int mipmax = (max_size == 0) ? 0 : ceil(log(1.0 * max_size) / log(2.0));
    if (mipmax == 0)
      panel->setInt(MULTI_TILE_MIPS, 0);
    else
      panel->setMinMaxStep(MULTI_TILE_MIPS, 0, mipmax, 1);
    panel->setEnabledById(MULTI_TILE_MIPS, mipmax != 0);
  }

  void updateScreenPrewiew(PropPanel::ContainerPropertyControl *panel)
  {
    IGenViewportWnd *viewport = EDITORCORE->getCurrentViewport();

    viewport->setProjection(true, 1.f, 1.f, OS_PREVIEW_H);

    float w_x0 = panel->getFloat(MIN_X_ID);
    float w_z0 = panel->getFloat(MIN_Z_ID);
    float w_x1 = w_x0 + panel->getFloat(SZ_X_ID);
    float w_z1 = w_z0 + panel->getFloat(SZ_Z_ID);

    mScrCells->mapPos = Point2(w_x0, w_z0);
    mScrCells->mapSize = Point2(w_x1 - w_x0, w_z1 - w_z0);
    if (multiScreenShot)
    {
      float tim = panel->getFloat(MULTI_RES_ID) * getTexSize(panel);
      mScrCells->tileInMeters = Point2(tim, tim);
      mScrCells->sizeInTiles = IPoint2(ceil((w_x1 - w_x0) / tim), ceil((w_z1 - w_z0) / tim));

      w_x1 = w_x0 + tim * mScrCells->sizeInTiles.x;
      w_z1 = w_z0 + tim * mScrCells->sizeInTiles.y;
    }
    else
    {
      mScrCells->tileInMeters = Point2(w_x1 - w_x0, w_z1 - w_z0);
      mScrCells->sizeInTiles = IPoint2(1, 1);
    }

    viewport->setCameraDirection(Point3(0.f, -1.f, 0.f), Point3(0.f, 0.f, 1.f));
    viewport->setCameraPos(Point3((w_x1 + w_x0) / 2.f, OS_CAMERA_H, (w_z1 + w_z0) / 2.f));

    int w, h;
    viewport->getViewportSize(w, h);
    viewport->setOrthogonalZoom(min(w / (w_x1 - w_x0) / 1.05, h / (w_z1 - w_z0) / 1.05));
    viewport->invalidateCache();
  }

  int getTexSize(PropPanel::ContainerPropertyControl *panel)
  {
    if (startTsInd < TEX_SIZES_CNT)
      return tex_sizes[startTsInd + panel->getInt(MULTI_TILE_SIZE_ID)];
    return 0;
  }

  int getResolution() const { return shotSz; }

private:
  enum
  {
    DIALOG_WIDTH = 250,
    DIALOG_HEIGHT = 245 + 50,
    DIALOG_FULL_HEIGHT = 435,
    MIN_X_ID = 1,
    MIN_Z_ID,
    SZ_X_ID,
    SZ_Z_ID,
    MULTI_SCREEN_ID,
    MULTI_GRP_ID,
    MULTI_RES_ID,
    MULTI_TILE_SIZE_ID,
    MULTI_TILE_QUALITY,
    MULTI_TILE_MIPS,
    MULTI_REND_OBJS,
  };

  bool multiScreenShot;
  GenericEditorAppWindow::OrtMultiScrData *mScrData;
  GenericEditorAppWindow::OrtScrCells *mScrCells;
  bool map2dAvail;
  int startTsInd;
  Tab<String> resLines;
  int shotSz;
};

struct BaseEditorCoreConsoleCmdHandler : public IConsoleCmd
{
  CoolConsole *console;
  BaseEditorCoreConsoleCmdHandler() : console(NULL) {}

  bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params) override
  {
    if (!console)
      return false;
    if (!stricmp(cmd, "time.speed"))
    {
      if (params.size() == 1)
      {
        float s = atof(params[0]);
        if (s >= 0)
        {
          dagor_game_time_scale = s;
          console->addMessage(ILogWriter::NOTE, "time scale changed to: %g", dagor_game_time_scale);
        }
        else
          console->addMessage(ILogWriter::ERROR, "bad time scale: %g (must be >=0)", s);
        return true;
      }
      else if (params.size() == 0)
      {
        dagor_game_time_scale = dagor_game_time_scale ? 0.0f : 1.0f;
        console->addMessage(ILogWriter::NOTE, "time scale changed to: %g", dagor_game_time_scale);
        return true;
      }
      console->addMessage(ILogWriter::ERROR, "%s requires 1 or 0 params, had %d", cmd, params.size());
      return true;
    }
    return false;
  }

  const char *onConsoleCommandHelp(const char *cmd) override
  {
    if (!console)
      return NULL;
    if (!stricmp(cmd, "time.speed"))
      return "time.speed <scale>";

    return NULL;
  }
  void registerSelf(CoolConsole *c)
  {
    console = c;
#define REGISTER_COMMAND(cmd_name)               \
  if (!console->registerCommand(cmd_name, this)) \
    console->addMessage(ILogWriter::ERROR, "[EditorCore] Couldn't register command '" cmd_name "'");
    REGISTER_COMMAND("time.speed");
#undef REGISTER_COMMAND
  }
};
static BaseEditorCoreConsoleCmdHandler ec_concmd;

namespace
{
bool saveDDSX(const char *fname, TexPixel32 *tex, int size, int line, bool save_alpha)
{
  ddsx::Header hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.label = _MAKE4C('DDSx');
  hdr.bitsPerPixel = 32;
  hdr.d3dFormat = 0x15; // D3DFMT_A8R8G8B8
  hdr.w = size;
  hdr.h = size;
  hdr.levels = 1;
  hdr.flags |= hdr.FLG_ZLIB | hdr.FLG_CUBTEX | (TEXADDR_CLAMP) | (TEXADDR_CLAMP << 4);
  hdr.memSz = size * size * 6 * sizeof(TexPixel32);

  FullFileSaveCB cwr(fname);
  if (cwr.fileHandle)
  {
    const auto write = [&cwr, &size, &line, &hdr](TexPixel32 *tex) {
      cwr.write(&hdr, sizeof(hdr));
      ZlibSaveCB z_cwr(cwr, ZlibSaveCB::CL_BestSpeed + 5); // ~25Mb/sec
      for (int face = 0; face < 6; ++face)
        for (int y = 0, ofs = size * face; y < size; y++, ofs += line)
          z_cwr.write(tex + ofs, size * sizeof(TexPixel32));
      z_cwr.finish();
    };

    if (!save_alpha)
    {
      eastl::unique_ptr<TexPixel32> noAlphaImg(new TexPixel32[size * line]);
      for (int i = 0; i < size * line; ++i)
      {
        auto *noAlphaPix = noAlphaImg.get() + i;
        noAlphaPix->r = (tex + i)->r;
        noAlphaPix->g = (tex + i)->g;
        noAlphaPix->b = (tex + i)->b;
        noAlphaPix->a = 1.f;
      }

      write(noAlphaImg.get());
    }
    else
    {
      write(tex);
    }

    hdr.packedSz = cwr.tell() - sizeof(hdr);
    cwr.seekto(0);
    cwr.write(&hdr, sizeof(hdr));
    cwr.close();
    return true;
  }
  else
  {
    return false;
  }
}


//==================================================================================================
bool saveScreenshot(ScreenshotFormat fmt, TexPixel32 *tex, int w, int h, int stride, const String &fname, int jpeg_q, bool save_alpha)
{
  switch (fmt)
  {
    case ScreenshotFormat::JPEG:
    {
      return ::save_jpeg32(tex, w, h, stride, jpeg_q, fname);
    }
    case ScreenshotFormat::TGA:
    {
      if (save_alpha)
      {
        return ::save_tga32(fname, tex, w, h, stride);
      }
      else
      {
        eastl::unique_ptr<std::byte> imageData(new std::byte[w * h * 3]);
        std::byte *image32 = reinterpret_cast<std::byte *>(tex);
        for (int i = 0; i < h; ++i)
          for (int j = 0; j < w; ++j)
            for (int h = 0; h < 3; ++h)
              imageData.get()[h + 3 * (j + w * i)] = image32[h + 4 * j + stride * i];
        return ::save_tga24(fname, reinterpret_cast<char *>(imageData.get()), w, h, w * 3);
      }
    }
    case ScreenshotFormat::PNG:
    {
      return ::save_png32(fname, tex, w, h, stride, nullptr, 0, save_alpha);
    }
    case ScreenshotFormat::TIFF:
    {
      return save_alpha ? ::save_tiff32(fname, tex, w, h, stride) : ::save_tiff24(fname, tex, w, h, stride);
    }
    case ScreenshotFormat::DDSX:
    {
      return saveDDSX(fname, tex, h, w, save_alpha);
    }
    default:
    {
      logerr("Unexpected screenshot format");
      return false;
    }
  }
}


//==================================================================================================
bool saveScreenshot(ScreenshotFormat fmt, Texture &tex, const String &fname, int jpeg_q, bool save_alpha)
{
  if (auto lockedTex = lock_texture<real>(&tex, 0, TEXLOCK_DEFAULT))
  {
    return saveScreenshot(fmt, reinterpret_cast<TexPixel32 *>(lockedTex.get()), lockedTex.getWidthInElems(),
      lockedTex.getHeightInElems(), lockedTex.getByteStride(), fname, jpeg_q, save_alpha);
  }

  return false;
}
} // namespace

GenericEditorAppWindow::GenericEditorAppWindow(const char *open_fname, IWndManager *manager) :
  ged(),
  quietMode(false),
  isRenderingOrtScreenshot_(false),
  mManager(manager),
  screenshotCfg(ScreenshotConfig::getDefaultCfg()),
  cubeScreenshotCfg(ScreenshotConfig::getDefaultCubeCfg())
{
  G_ASSERT(mManager && "GenericEditorAppWindow::ctor(): layout manager == NULL!");

  SimpleString exe_dir(dgs_argv[0]);
  ::dd_get_fname_location(exe_dir.str(), dgs_argv[0]);
  sgg::set_exe_path(exe_dir);

  themeName = defaultThemeName;

  memset(sceneFname, 0, sizeof(sceneFname));
  memset(&cursor, 0, sizeof(cursor));
  memset(&cursorLast, 0, sizeof(cursorLast));

  cursor.texId = BAD_TEXTUREID;
  cursorLast.texId = BAD_TEXTUREID;
  shouldUpdateViewports = false;

  undoSystem = create_undo_system("GenEditorUndo", 10 << 20, this);

  gizmoEH = new (uimem) GizmoEventFilter(ged, grid);
  brushEH = new (midmem) BrushEventFilter;
  appEH = NULL;

  // at the end of init we load or create new scene
  if (!open_fname)
  {
    sceneFname[0] = 0;
    shouldLoadFile = false;
  }
  else
  {
    strcpy(sceneFname, open_fname);
    shouldLoadFile = true;
  }

  mainMenu.reset(PropPanel::create_menu());
  getMainMenu()->setEventHandler(this);

  console = new CoolConsole();
}

GenericEditorAppWindow::~GenericEditorAppWindow()
{
  ec_concmd.console = NULL;
  delete console;

  del_it(undoSystem);
  del_it(gizmoEH);
  del_it(brushEH);

  closeScreenshotSettingsDialog();
}


PropPanel::IMenu *GenericEditorAppWindow::getMainMenu()
{
  G_ASSERT(mainMenu);
  return mainMenu.get();
}


void GenericEditorAppWindow::fillMenu(PropPanel::IMenu *menu) { addExitCommand(menu); }

void GenericEditorAppWindow::addExitCommand(PropPanel::IMenu *menu) { menu->addItem(PropPanel::ROOT_MENU_ITEM, CM_EXIT, "Exit"); }


void GenericEditorAppWindow::init(const char *select_workspace)
{
  // ged.setViewportCacheMode(ged.vcmode);

  console->initConsole();
  ec_concmd.registerSelf(console);

  fillMenu(getMainMenu());
  setDocTitle();

  mManager->show();

  startWith(select_workspace);

  updateMenu(getMainMenu());
}


void GenericEditorAppWindow::startWith(const char *select_workspace)
{
  startup_editor_core_select_startup_scene();

  // process start dialog

  for (bool handled = false; !handled;)
  {
    if (shouldLoadFile)
    {
      shouldLoadFile = false;
      String path_to_blk(sceneFname);
      ::dd_get_fname_location(path_to_blk, sceneFname);

      EditorWorkspace &wsp = getWorkspace();
      // wsp.initWorkspaceBlk(sceneFname);

      if (!wsp.loadIndirect(sceneFname))
      {
        debug("Errors while loading workspace \"%s\"", sceneFname);
        wingw::message_box(0, "Workspace error",
          "Errors while loading workspace \"%s\"\n"
          "Some workspace settings may be wrong.",
          sceneFname);
      }

      if (loadProject(path_to_blk))
        handled = true;
    }
    else
    {
      int variant = -1;

      EditorStartDialog esd("Select workspace", getWorkspace(), ::make_full_path(sgg::get_exe_path(), "../.local/workspace.blk"),
        select_workspace);

      variant = esd.showDialog();

      startup_editor_core_force_startup_scene_draw();

      // process dialog result

      switch (variant)
      {
        case PropPanel::DIALOG_ID_OK:
        {
          PropPanel::ContainerPropertyControl *_panel = esd.getPanel();
          if (_panel->getInt(ID_START_DIALOG_COMBO) > -1)
          {
            SimpleString buf(_panel->getText(ID_START_DIALOG_COMBO));
            G_ASSERT((buf.length() < 256) && "Line is too long");
            strncpy((char *)&sceneFname, buf.str(), 256);

            if (loadProject(getWorkspace().getAppDir()))
            {
              handled = true;
              GenericEditorAppWindow::setDocTitle();
            }
          }
          break;
        }

        case PropPanel::DIALOG_ID_CANCEL: mManager->close(); return;

        default: handled = true;
      }
    }
  }
}


void GenericEditorAppWindow::fillCommonToolbar(PropPanel::ContainerPropertyControl &tb)
{
  tb.createButton(CM_ZOOM_AND_CENTER, "Zoom and center (Z or Ctrl+Shift+Z)");
  tb.setButtonPictures(CM_ZOOM_AND_CENTER, "zoom_and_center");

  tb.createButton(CM_NAVIGATE, "Navigate");
  tb.setButtonPictures(CM_NAVIGATE, "navigate");

  tb.createSeparator();

  tb.createRadio(CM_CAMERAS_FREE, "Fly mode (Spacebar)");
  tb.setButtonPictures(CM_CAMERAS_FREE, "freecam");

  tb.createRadio(CM_CAMERAS_FPS, "FPS mode (Ctrl+Spacebar)");
  tb.setButtonPictures(CM_CAMERAS_FPS, "fpscam");

  tb.createRadio(CM_CAMERAS_TPS, "TPS mode (Ctrl+Shift+Spacebar)");
  tb.setButtonPictures(CM_CAMERAS_TPS, "tpscam");

  tb.createSeparator();

  tb.createButton(CM_STATS, "Show level statistics");
  tb.setButtonPictures(CM_STATS, "stats");

  tb.createSeparator();

  tb.createButton(CM_CHANGE_FOV, "Change FOV");
  tb.setButtonPictures(CM_CHANGE_FOV, "change_fov");

  tb.createSeparator();

  tb.createButton(CM_CREATE_SCREENSHOT, "Create screenshot (Ctrl+P)");
  tb.setButtonPictures(CM_CREATE_SCREENSHOT, "screenshot");

  tb.createButton(CM_CREATE_CUBE_SCREENSHOT, "Create cube screenshot (Ctrl+Shift+P)");
  tb.setButtonPictures(CM_CREATE_CUBE_SCREENSHOT, "screenshot_cube");

  tb.createSeparator();

  tb.createButton(CM_ENVIRONMENT_SETTINGS, "Environment settings");
  tb.setButtonPictures(CM_ENVIRONMENT_SETTINGS, "environment_settings");
}


/*
void GenericEditorAppWindow::handleNotifyEvent ( CtlWndObject *w, CtlEventData &ev ) {
  switch ( ev.id ) {
    case CTLWM_ACTIVATE:
    case CTLWM_INACTIVATE:
      if ( ged.isViewport ( w ))
        ged.setViewportCacheMode ( ged.vcmode );
      break;
    case CtlTrackBar::TBM_END_CHANGE_VAL:
      IEditorCoreEngine::get()->invalidateViewportCache();
      break;
    case CTLWM_MWHEEL_SCROLLUP:
    case CTLWM_MWHEEL_SCROLLDOWN:
    case CTLWM_CBPRESSED:
    case CTLWM_CBRELEASED:
    case CTLWM_EXT_CBRELEASED:
    case CTLWM_MOUSEMOVE:
    case CTLWM_EXT_MOUSEMOVE:
      if (ged.isViewport(w))
      {
        ViewportWindow *activeViewport = ged.getActiveViewport();
        ViewportWindow *viewport = activeViewport ?
          activeViewport : (ViewportWindow *)w;
        if (viewport)
          viewport->processCameraControl(ev.id, ev.p1, ev.p2, ev.p3);
      }
      break;

    case CM_SAVE_ACTIVE_VIEW:
    {
      unsigned id = *(unsigned *)&ev.p1;
      ViewportWindow *activeViewport = ged.getActiveViewport();
      if ( !activeViewport || ( id > 32 ))
        break;

      ViewportWindow::restoreFlags |= ( 1 << id );

      if ( viewportsParams.size() <= id )
        viewportsParams.resize( id + 1 );

      activeViewport->getCameraTransform( viewportsParams[id].view );
      viewportsParams[id].fov = activeViewport->getFov();
      break;
    }
    case CM_RESTORE_ACTIVE_VIEW:
    {
      unsigned id = *(unsigned *)&ev.p1;
      ViewportWindow *activeViewport = ged.getActiveViewport();
      if ( !activeViewport || ( id > 32 ) || !( ViewportWindow::restoreFlags & ( 1 << id ))
        || ( id >= viewportsParams.size()))
        break;

      activeViewport->setCameraTransform( viewportsParams[id].view );
      activeViewport->setFov( viewportsParams[id].fov );
      break;
    }
  }
}
*/

void GenericEditorAppWindow::saveViewportsParams(DataBlock &blk) const
{
  DataBlock *viewportsBlk = blk.addBlock("ViewportsParams");
  if (!viewportsBlk)
    return;

  for (int i = 0; i < ViewportWindow::viewportsParams.size(); ++i)
  {
    if (!(ViewportWindow::restoreFlags & (1 << i)))
      continue;

    DataBlock *viewportBlk = viewportsBlk->addBlock(String(32, "viewport%d", i));
    if (!viewportBlk)
      continue;

    viewportBlk->setInt("id", i);
    viewportBlk->setTm("view", ViewportWindow::viewportsParams[i].view);
    viewportBlk->setReal("fov", ViewportWindow::viewportsParams[i].fov);
  }
}


void GenericEditorAppWindow::loadViewportsParams(const DataBlock &blk)
{
  const DataBlock *viewports = blk.getBlockByName("ViewportsParams");
  if (!viewports)
    return;

  for (int i = 0; i < viewports->blockCount(); ++i)
  {
    const DataBlock *viewport = viewports->getBlock(i);
    if (!viewport)
      continue;

    const int id = viewport->getInt("id", -1);
    if ((id == -1) || (id > 32))
      continue;

    ViewportWindow::restoreFlags |= (1 << id);

    if (ViewportWindow::viewportsParams.size() <= id)
      ViewportWindow::viewportsParams.resize(id + 1);

    ViewportWindow::viewportsParams[id].view = viewport->getTm("view", TMatrix::IDENT);
    ViewportWindow::viewportsParams[id].fov = viewport->getReal("fov", 90.0f);
  }
}


void GenericEditorAppWindow::updateUndoRedoMenu() {}


int GenericEditorAppWindow::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case CM_EXIT: mManager->close(); return 1;

    default: return 0;
  }
}


/*
void GenericEditorAppWindow::handleCommand ( int cmd ) // use onMenuItemClick(unsigned id);
{
  if (cmd >= CM_FILE_RECENT_BASE && cmd < CM_FILE_RECENT_LAST &&
    cmd - CM_FILE_RECENT_BASE < recentFiles.size())
  {
    String fname = recentFiles[cmd - CM_FILE_RECENT_BASE];

    if ( stricmp ( fname, sceneFname ) == 0 )
      return;

    if ( canCloseScene("Open project"))
    {
      addToRecentList(sceneFname);
      strcpy ( sceneFname, fname );
      loadProject(sceneFname);
      GenericEditorAppWindow::setDocTitle();
    }
    return;
  }

  switch (cmd)
  {
    case CM_OPTIONS_CAMERAS:
      ::show_camera_objects_config_dialog(this);
      return;

    case CM_CAMERAS_FREE:
    case CM_CAMERAS_FPS:
    case CM_CAMERAS_TPS:
    {
      IGenViewportWnd* vp = IEditorCoreEngine::get()->getCurrentViewport();

      if (vp)
      {
        vp->activate();
        vp->getCtlWindow()->handleEvent(CTLWM_COMMAND, cmd);
      }

      switchAccelerators();
      return;
    }

    case CM_EXIT:
      close();
      return;

    case CM_UNDO:
      undoSystem->undo();
      updateUndoRedoMenu();
      IEditorCoreEngine::get()->updateViewports();
      IEditorCoreEngine::get()->invalidateViewportCache();
      return;

    case CM_REDO:
      undoSystem->redo();
      updateUndoRedoMenu();
      IEditorCoreEngine::get()->updateViewports();
      IEditorCoreEngine::get()->invalidateViewportCache();
      return;

    case CM_FILE_NEW:
      undoSystem->clear();
      handleNewProject();
      return;

    case CM_FILE_OPEN :
      handleOpenProject( false );
      return;

    case CM_FILE_SAVE:
      if (sceneFname[0])
      {
        saveProject(sceneFname);
        addToRecentList(sceneFname);
        return;
      }
      return;

    case CM_VIEWPORT_LAYOUT_4:
    case CM_VIEWPORT_LAYOUT_2HOR:
    case CM_VIEWPORT_LAYOUT_2VERT:
    case CM_VIEWPORT_LAYOUT_1:
      if ( ged.viewportLayout != cmd )
      {
        if (Menu)
          Menu->checkItem ( ged.viewportLayout, false, true );

        ged.viewportLayout = cmd;
        if (Menu)
          Menu->checkItem ( ged.viewportLayout, true, true );

        adjustNonClient ();
        ged.setViewportCacheMode ( ged.vcmode );

        IEditorCoreEngine::get()->updateViewports();
        IEditorCoreEngine::get()->invalidateViewportCache();
      }
      break;
    case CM_VIEW_GRID_SHOW:
    {
      ViewportWindow *vpw = ged.getActiveViewport();

      if (vpw)
      {
        int index = ged.findViewportIndex(vpw);
        grid.setVisible(!grid.isVisible(index), index);

        IEditorCoreEngine::get()->updateViewports();
        IEditorCoreEngine::get()->invalidateViewportCache();
      }
      break;
    }
    case CM_VIEW_GRID_INC_STEP:
      grid.incStep();
      IEditorCoreEngine::get()->updateViewports ();
      break;
    case CM_VIEW_GRID_DEC_STEP:
      grid.decStep();
      IEditorCoreEngine::get()->updateViewports ();
      break;
    case CM_VIEW_GRID_MOVE_SNAP:
      ged.tbManager->setMoveSnap();
      break;
    case CM_VIEW_GRID_ANGLE_SNAP:
      ged.tbManager->setRotateSnap();
      break;
    case CM_VIEW_GRID_SCALE_SNAP:
      ged.tbManager->setScaleSnap();
      break;
    case CM_CHANGE_FOV:
      onChangeFov();
      break;
    case CM_CONSOLE:
      onShowConsole();
      break;
    case CM_VIEW_WIREFRAME:
    case CM_VIEW_PERSPECTIVE:
    case CM_VIEW_FRONT:
    case CM_VIEW_BACK:
    case CM_VIEW_TOP:
    case CM_VIEW_BOTTOM:
    case CM_VIEW_LEFT:
    case CM_VIEW_RIGHT:
    {
      ViewportWindow *active = ged.getActiveViewport();
      if (active)
        active->handleEvent(CTLWM_COMMAND, cmd);
      break;
    }

    case CM_OPTIONS_SCREENSHOT:
      setScreenshotOptions();
      break;

    case CM_CREATE_SCREENSHOT:
      createScreenshot();
      break;

    case CM_CREATE_CUBE_SCREENSHOT:
      createCubeScreenshot();
      break;

    case CM_CREATE_ORTHOGONAL_SCREENSHOT:
      createOrthogonalScreenshot();
      break;
  }
}
*/

void GenericEditorAppWindow::getDocTitleText(String &text) { text.printf(300, "Simple Editor - %s", (const char *)sceneFname); }


void GenericEditorAppWindow::setDocTitle()
{
  String title;
  title.reserve(1024);

  getDocTitleText(title);

  mManager->setMainWindowCaption(title.str());
}


bool GenericEditorAppWindow::canCloseScene(const char *title)
{
  if (!sceneFname[0])
    return true;

  return true;
}


bool GenericEditorAppWindow::canClose() { return (canCloseScene("Exit")); }


class GenericEditorAppWindow::FovDlg : public PropPanel::DialogWindow
{
public:
  FovDlg(void *phandle, const char *caption, float curFov) : DialogWindow(phandle, _pxScaled(200), _pxScaled(100), caption)
  {
    PropPanel::ContainerPropertyControl *_panel = getPanel();
    G_ASSERT(_panel && "No panel in GenericEditorAppWindow::FovDlg");

    _panel->createEditFloat(ID_FOV_EDIT, "FoV (deg)", curFov);
    _panel->setMinMaxStep(ID_FOV_EDIT, 2.0, 178.0, 1);
  }

  float getFov()
  {
    PropPanel::ContainerPropertyControl *_panel = getPanel();
    G_ASSERT(_panel && "No panel in GenericEditorAppWindow::FovDlg:getFov()");

    return _panel->getFloat(ID_FOV_EDIT);
  }

private:
  enum
  {
    ID_FOV_EDIT,
  };
};


void GenericEditorAppWindow::onChangeFov()
{
  ViewportWindow *viewport = ged.getCurrentViewport();
  if (!viewport)
    return;

  FovDlg dlg(mManager->getMainWindow(), "Set Field-of-View", viewport->getFov() * RAD_TO_DEG);

  if (dlg.showDialog() == PropPanel::DIALOG_ID_OK)
    viewport->setFov(dlg.getFov() * DEG_TO_RAD);
  viewport->activate();
}


void GenericEditorAppWindow::onShowConsole()
{
  if (console->isVisible())
    console->hideConsole();
  else
    console->showConsole(true);
}


//==================================================================================================

void GenericEditorAppWindow::setScreenshotOptions(ScreenshotDlgMode mode)
{
  if (!screenshot_settings_dialog)
  {
    screenshot_settings_dialog.reset(
      new ScreenshotSettingsDialog(screenshotCfg, cubeScreenshotCfg, mode, [this](const ScreenshotSettingsDialog &dialog) {
        screenshotCfg = dialog.getScreenshotCfg();
        cubeScreenshotCfg = dialog.getCubeScreenshotCfg();
      }));

    screenshot_settings_dialog->fill();
  }

  if (screenshot_settings_dialog->isVisible())
    screenshot_settings_dialog->hide();
  else
    screenshot_settings_dialog->show();
}


//==================================================================================================

String GenericEditorAppWindow::getScreenshotName(bool cube) const
{
  const String fnameMask = getScreenshotNameMask(cube);
  String fname;

  for (int idx = 1; idx < 1000; ++idx)
  {
    fname.printf(512, fnameMask, idx);
    if (!::dd_file_exist(fname))
      return fname;
  }

  return fname;
}


//==================================================================================================

Texture *GenericEditorAppWindow::renderInTex(int w, int h, const TMatrix *tm, bool skip_debug_objects,
  bool should_make_orthogonal_screenshot, bool should_use_z_buffer, float world_x0, float world_x1, float world_z0, float world_z1)
{
  if (!ec_cached_viewports)
    return NULL;

  const int curVpIdx = ged.getCurrentViewportId();

  if (curVpIdx < 0)
    return NULL;

  ViewportWindow *viewport = (ViewportWindow *)ec_cached_viewports->getViewportUserData(curVpIdx);

  if (!viewport)
    return NULL;

  d3d::GpuAutoLock gpuLock;

  Driver3dRenderTarget old;
  d3d::get_render_target(old);

  Texture *tex = d3d::create_tex(NULL, w, h, TEXFMT_A8R8G8B8 | TEXCF_RTARGET, 1);


  if (!tex)
    return NULL;

  d3d::set_render_target(tex, 0);


  const bool cacheUsed = ::ec_cached_viewports->isViewportCacheUsed(curVpIdx);
  ::ec_cached_viewports->enableViewportCache(curVpIdx, false);
  ::ec_cached_viewports->invalidateViewportCache(curVpIdx);


  if (::ec_cached_viewports->startRender(curVpIdx))
  {
    int origW = w, origH = h;
    viewport->getViewportSize(origW, origH);

    viewport->setScreenshotMode(Point2(w, h));

    d3d::setview(0, 0, w, h, 0, 1);

    if (tm)
    {
      ::grs_cur_view.tm = inverse(*tm);
      ::grs_cur_view.itm = *tm;
      d3d::settm(TM_VIEW, ::grs_cur_view.tm);
    }

    Matrix44 projection;
    real zNear;
    real zFar;
    viewport->getZnearZfar(zNear, zFar);
    float fov = viewport->getFov();
    bool isOrthogonal = viewport->isOrthogonal();

    if (should_make_orthogonal_screenshot)
    {
      TMatrix tm;
      tm.setcol(2, 0, -1, 0);
      tm.setcol(1, 0, 0, 1);
      tm.setcol(0, tm.getcol(1) % tm.getcol(2));
      tm.setcol(3, 0, ORTHOGONAL_SCREENSHOT_HEIGHT, 0);

      ::grs_cur_view.tm = inverse(tm);
      ::grs_cur_view.itm = tm;
      d3d::settm(TM_VIEW, ::grs_cur_view.tm);

      viewport->setProjection(true, 1.f, 1.f, 10000.f);

      projection = matrix_ortho_off_center_lh(world_x0, world_x1, world_z0, world_z1, 10000.0f, 1.f);

      d3d::settm(TM_PROJ, &projection);
    }
    else if (viewport->isOrthogonal())
    {
      projection = matrix_ortho_lh_reverse(2 * w / viewport->getOrthogonalZoom(), 2 * h / viewport->getOrthogonalZoom(), 1, zFar);

      d3d::settm(TM_PROJ, &projection);
    }
    else if (tm)
    {
      d3d::setpersp(Driver3dPerspective(1, 1, zNear, zFar, 0, 0));
    }
    else
    {
      const float aspect = static_cast<float>(w) / h;
      const float origAspect = static_cast<float>(origW) / origH;
      float top = 0.f, bottom = 0.f, right = 0.f, left = 0.f;

      const float hFov = tm ? HALFPI : fov;
      if (origAspect < aspect)
      {
        top = tanf(0.5f * hFov) * (static_cast<float>(origH) / origW) * zNear;
        bottom = -top;
        right = top * aspect;
        left = -right;
      }
      else
      {
        right = tanf(hFov * 0.5f) * zNear;
        left = -right;
        top = right / aspect;
        bottom = -top;
      }

      const TMatrix4 perspective = matrix_perspective_off_center_reverse(left, right, bottom, top, zNear, zFar);
      d3d::settm(TM_PROJ, &perspective);
    }

    screenshotRender(skip_debug_objects);

    update_visibility_finder(EDITORCORE->queryEditorInterface<IVisibilityFinderProvider>()->getVisibilityFinder());
    ::ec_cached_viewports->endRender();

    viewport->resetScreenshotMode();
  }

  ::ec_cached_viewports->enableViewportCache(curVpIdx, cacheUsed);

  d3d::set_render_target(old);

  return tex;
}


//==================================================================================================

void GenericEditorAppWindow::createScreenshot()
{
  String fname = getScreenshotName(false);
  Texture *tex = renderInTex(screenshotCfg.width, screenshotCfg.height, NULL, !screenshotCfg.enableDebugGeometry);
  if (tex && saveScreenshot(screenshotCfg.format, *tex, fname, screenshotCfg.jpegQuality, screenshotCfg.enableTransparentBackground))
  {
    wingw::message_box(wingw::MBS_INFO, "Screenshot saved", "Screenshot saved to\n\"%s\"", fname);
  }
  else
  {
    wingw::message_box(wingw::MBS_EXCL, "Screenshot error", "Failed to save screenshot to\n\"%s\"", fname);
  }
}

//==================================================================================================

void GenericEditorAppWindow::createCubeScreenshot()
{
  String fname = getScreenshotName(true);
  const int lineLen = cubeScreenshotCfg.size * 6;
  const int strideLen = cubeScreenshotCfg.size * sizeof(TexPixel32);

  TexPixel32 *cubeImg = new (tmpmem) TexPixel32[cubeScreenshotCfg.size * lineLen];
  memset(cubeImg, 0, cubeScreenshotCfg.size * lineLen * sizeof(TexPixel32));

  bool canSave = true;

  Point3 pos = ::grs_cur_view.pos;
  for (int face = 0; face < 6; ++face)
  {
    TMatrix tm;
    tm.identity();
    tm.setcol(3, pos);
    tm = cube_matrix(tm, face);

    Texture *tex = renderInTex(cubeScreenshotCfg.size, cubeScreenshotCfg.size, &tm, !cubeScreenshotCfg.enableDebugGeometry);

    if (tex)
    {
      TexPixel32 *img = NULL;

      int stride = 0;
      tex->lockimg((void **)&img, stride, 0, TEXLOCK_DEFAULT);

      if (img)
        for (int line = 0; line < cubeScreenshotCfg.size; ++line)
          memcpy(&cubeImg[line * cubeScreenshotCfg.size * 6 + face * cubeScreenshotCfg.size], &img[line * cubeScreenshotCfg.size],
            strideLen);

      tex->unlockimg();
      del_d3dres(tex);
    }
    else
    {
      canSave = false;
      break;
    }
  }

  if (canSave && saveScreenshot(cubeScreenshotCfg.format, cubeImg, lineLen, cubeScreenshotCfg.size, lineLen * 4, fname,
                   cubeScreenshotCfg.jpegQuality, cubeScreenshotCfg.enableTransparentBackground))
  {
    wingw::message_box(wingw::MBS_INFO, "Cube Screenshot saved", "Screenshot saved to\n\"%s\"", fname);
  }
  else
  {
    wingw::message_box(wingw::MBS_EXCL, "Cube screenshot error", "Failed to save screenshot to\n\"%s\"", fname);
  }

  delete[] cubeImg;
}

void GenericEditorAppWindow::createOrthogonalScreenshot(const char *dest_folder, const float *x0z0x1z1)
{
  int sz = 8192;
  int max_sz = getMaxRTSize();
  while (sz > max_sz && sz > 32)
    sz >>= 1;

  ViewportWindow *sv = ged.getCurrentViewport();
  ViewportParams sv_params;
  if (sv)
  {
    sv->getCameraTransform(sv_params.view);
    sv_params.fov = sv->getFov();
  }

  OrthogonalScreenshotDlg dlg("Ortho screenshot", mScrData, mScrCells, getMaxRTSize(), (dest_folder != NULL));
  if (x0z0x1z1)
  {
    mScrData.useIt = false;
    mScrData.mapPos.x = x0z0x1z1[0];
    mScrData.mapPos.y = x0z0x1z1[1];
    mScrData.mapSize.x = x0z0x1z1[2] - x0z0x1z1[0];
    mScrData.mapSize.y = x0z0x1z1[3] - x0z0x1z1[1];
  }
  else
  {
    int res = dlg.showDialog();
    sz = dlg.getResolution();
    mScrCells.reset();

    if (res != PropPanel::DIALOG_ID_OK)
    {
      if (sv)
      {
        sv->setCameraTransform(sv_params.view);
        sv->setFov(sv_params.fov);
      }
      return;
    }
  }

  float minX = mScrData.mapPos.x, maxX = minX + mScrData.mapSize.x;
  float minZ = mScrData.mapPos.y, maxZ = minZ + mScrData.mapSize.y;
  isRenderingOrtScreenshot_ = true;

  if (!mScrData.useIt)
  {
    String fnameMask = getScreenshotNameMask(false);
    location_from_path(fnameMask);
    fnameMask.aprintf(260, "/de_ortho(%.0f,%.0f)-(%.0f,%.0f)-%%03i.tga", minX, minZ, maxX, maxZ);
    dd_simplify_fname_c(fnameMask);

    String fname;
    for (int idx = 1; idx < 1000; ++idx)
    {
      fname.printf(512, fnameMask, idx);
      if (!::dd_file_exist(fname))
        break;
    }

    Texture *tex = renderInTex(sz, sz, NULL, false, true, false, minX, maxX, minZ, maxZ);

    if (tex)
    {
      TexPixel32 *img = NULL;
      int stride = 0;
      tex->lockimg((void **)&img, stride, 0, TEXLOCK_DEFAULT);

      TextureInfo textureInfo;
      tex->getinfo(textureInfo);

      if (img && ::save_tga32(fname, img, textureInfo.w, textureInfo.h, stride))
      {
        if (!x0z0x1z1)
          wingw::message_box(wingw::MBS_INFO, "Screenshot saved", "Screenshot saved to\n\"%s\"", fname.str());
        else
          debug("Screenshot saved to\n\"%s\"", fname.str());
      }

      tex->unlockimg();
      del_d3dres(tex);
    }
  }
  else if (dest_folder)
  {
    SimpleString map2d_path(dest_folder);
    dd_mkdir(map2d_path);

    String fn(map2d_path);
    fn += "/screen";

    float tile_in_meters = mScrData.res * mScrData.tileSize;
    IPoint2 size_in_tiles = IPoint2(ceil(mScrData.mapSize.x / tile_in_meters), ceil(mScrData.mapSize.y / tile_in_meters));

    int _counter = 0;
    SimpleString file_ext(".tga");
    switch (mScrData.q_type)
    {
      case SCR_TYPE_QUALITY_TGA: file_ext = ".tga"; break;
      case SCR_TYPE_QUALITY_JPEG_100: file_ext = "_q100.jpeg"; break;
      case SCR_TYPE_QUALITY_JPEG_90: file_ext = "_q90.jpeg"; break;
      case SCR_TYPE_QUALITY_JPEG_40: file_ext = "_q40.jpeg"; break;
    };

    DataBlock blk;
    blk.addIPoint2("size_in_tiles", size_in_tiles);
    blk.addReal("tile_in_meters", tile_in_meters);
    blk.addInt("tile_in_pixels", mScrData.tileSize);
    blk.addPoint2("map_pos", mScrData.mapPos);
    blk.addStr("file_ext", file_ext);
    blk.addInt("mip_levels", mScrData.mipLevels);

    float _z = mScrData.mapPos.y;
    for (int i = 0; i < size_in_tiles.y; ++i)
    {
      float _x = mScrData.mapPos.x;
      for (int j = 0; j < size_in_tiles.x; ++j)
      {
        String cur_fn(512, "%s_%.6d%s", fn.str(), ++_counter, file_ext.str());
        saveOrthogonalScreenshot(cur_fn, 0, _x, _z, tile_in_meters);

        for (int k = 1; k < mScrData.mipLevels + 1; ++k)
        {
          int mip_mult = (1 << k);
          if ((i % mip_mult) == 0 && (j % mip_mult) == 0)
          {
            String mip_path(512, "%s/%s%d", map2d_path.str(), "mipmap_", k);
            dd_mkdir(mip_path);
            String mip_fn(512, "%s/%s", mip_path.str(), ::dd_get_fname(cur_fn.str()));
            saveOrthogonalScreenshot(mip_fn, k, _x, _z, tile_in_meters * mip_mult);
          }
        }
        _x += tile_in_meters;
      }
      _z += tile_in_meters;
    }

    wingw::message_box(wingw::MBS_INFO, "Screenshot saved", "Map2d screenshot saved to\n\"%s\"", map2d_path.str());

    blk.saveToTextFile(fn + ".blk");
  }
  isRenderingOrtScreenshot_ = false;

  if (sv)
  {
    sv->setCameraTransform(sv_params.view);
    sv->setFov(sv_params.fov);
  }
}


void GenericEditorAppWindow::saveOrthogonalScreenshot(const char *cur_fn, int mip_level, float _x, float _z, float tile_in_meters)
{
  int tile_size = mScrData.tileSize;
  Texture *tex = renderInTex(tile_size, tile_size, NULL, false, true, false, _x, _x + tile_in_meters, _z, _z + tile_in_meters);

  if (tex)
  {
    TexPixel32 *img = NULL;
    int stride = 0;
    tex->lockimg((void **)&img, stride, 0, TEXLOCK_DEFAULT);
    TextureInfo textureInfo;
    tex->getinfo(textureInfo);
    if (img)
      switch (mScrData.q_type)
      {
        case SCR_TYPE_QUALITY_TGA:
        {
          int wd = textureInfo.w, ht = textureInfo.h;
          for (int i = 0; i < wd * ht; ++i)
            img[i].a = 0xFF;

          ::save_tga32(cur_fn, img, wd, ht, stride);
        }

        break;
        case SCR_TYPE_QUALITY_JPEG_100: ::save_jpeg32(img, textureInfo.w, textureInfo.h, stride, 100, cur_fn); break;
        case SCR_TYPE_QUALITY_JPEG_90: ::save_jpeg32(img, textureInfo.w, textureInfo.h, stride, 90, cur_fn); break;
        case SCR_TYPE_QUALITY_JPEG_40: ::save_jpeg32(img, textureInfo.w, textureInfo.h, stride, 40, cur_fn); break;
      };

    tex->unlockimg();
    del_d3dres(tex);
  }
}


//==================================================================================================

void GenericEditorAppWindow::renderOrtogonalCells()
{
  if (!isOrthogonalPreview())
    return;

  ::begin_draw_cached_debug_lines();
  ::set_cached_debug_lines_wtm(TMatrix::IDENT);

  float st_x = mScrCells.mapPos.x;
  float st_z = mScrCells.mapPos.y;
  float end_x = st_x + mScrCells.tileInMeters.x * mScrCells.sizeInTiles.x;
  float end_z = st_z + mScrCells.tileInMeters.y * mScrCells.sizeInTiles.y;
  E3DCOLOR cl_real = E3DCOLOR(255, 255, 255);

  if (mScrCells.tileInMeters != mScrCells.mapSize)
  {
    float pos_x = st_x;
    for (int i = 0; i < mScrCells.sizeInTiles.x + 1; ++i)
    {
      ::draw_cached_debug_line(Point3(pos_x, OSGR_H, st_z), Point3(pos_x, OSGR_H, end_z), cl_real);
      pos_x += mScrCells.tileInMeters.x;
    }

    float pos_z = st_z;
    for (int i = 0; i < mScrCells.sizeInTiles.y + 1; ++i)
    {
      ::draw_cached_debug_line(Point3(st_x, OSGR_H, pos_z), Point3(end_x, OSGR_H, pos_z), cl_real);
      pos_z += mScrCells.tileInMeters.y;
    }
  }

  end_x = st_x + mScrCells.mapSize.x;
  end_z = st_z + mScrCells.mapSize.y;
  E3DCOLOR cl_need = E3DCOLOR(255, 255, 0);

  ::draw_cached_debug_line(Point3(st_x, OSSEL_H, st_z), Point3(end_x, OSSEL_H, st_z), cl_need);
  ::draw_cached_debug_line(Point3(st_x, OSSEL_H, end_z), Point3(end_x, OSSEL_H, end_z), cl_need);
  ::draw_cached_debug_line(Point3(st_x, OSSEL_H, st_z), Point3(st_x, OSSEL_H, end_z), cl_need);
  ::draw_cached_debug_line(Point3(end_x, OSSEL_H, st_z), Point3(end_x, OSSEL_H, end_z), cl_need);

  ::end_draw_cached_debug_lines();
}


//==================================================================================================


int GenericEditorAppWindow::getMaxRTSize()
{
  if (d3d::get_driver_code().is(d3d::dx11))
    return 8192;

  int max_ts = 0;
  int w, h;
  d3d::get_render_target_size(w, h, NULL, 0);

  String ed_set_fn(260, "%s/../commonData/startup_editors.blk", sgg::get_exe_path_full());
  DataBlock ed_set(ed_set_fn);
  const DataBlock *video_blk = ed_set.getBlockByName("video");
  if (video_blk)
    max_ts = video_blk->getInt("min_target_size", 0);

  if (max_ts != 0)
    return max(max_ts, min(w, h));

  video_blk = ::dgs_get_settings()->getBlockByName("video");
  if (video_blk)
    max_ts = video_blk->getInt("min_target_size", 0);

  return max(max_ts, min(w, h));
}


//==================================================================================================

String GenericEditorAppWindow::getThemeFileName() const { return String::mk_str_cat(themeName, ".blk"); }


//==================================================================================================

void GenericEditorAppWindow::saveThemeSettings(DataBlock &blk) const
{
  DataBlock *themeBlk = blk.addBlock("theme");
  if (themeBlk)
  {
    themeBlk->setStr("name", themeName);
  }
}

//==================================================================================================

void GenericEditorAppWindow::loadThemeSettings(const DataBlock &blk)
{
  const DataBlock *themeBlk = blk.getBlockByName("theme");
  if (themeBlk)
  {
    themeName = themeBlk->getStr("name", defaultThemeName);
  }
}


//==================================================================================================

void GenericEditorAppWindow::saveScreenshotSettings(DataBlock &blk) const
{
  DataBlock *screenshotBlk = blk.addBlock("screenshot");
  if (screenshotBlk)
  {
    saveScreenshotConfig(*screenshotBlk, screenshotCfg);
    saveCubeScreenshotConfig(*screenshotBlk, cubeScreenshotCfg);

    DataBlock *ortMultiScrBlk = screenshotBlk->addBlock("ort_multi_screenshot");
    if (ortMultiScrBlk)
    {
      ortMultiScrBlk->addBool("use_it", mScrData.useIt);
      ortMultiScrBlk->addBool("render_objects", mScrData.renderObjects);
      ortMultiScrBlk->addPoint2("map_pos", mScrData.mapPos);
      ortMultiScrBlk->addPoint2("map_size", mScrData.mapSize);
      ortMultiScrBlk->addInt("tile_size", mScrData.tileSize);
      ortMultiScrBlk->addReal("resolution", mScrData.res);
      ortMultiScrBlk->addInt("quality_type", mScrData.q_type);
      ortMultiScrBlk->addInt("mip_levels", mScrData.mipLevels);
    }
  }
}


//==================================================================================================

void GenericEditorAppWindow::loadScreenshotSettings(const DataBlock &blk)
{
  const DataBlock *screenshotBlk = blk.getBlockByName("screenshot");
  if (screenshotBlk)
  {
    loadScreenshotConfig(*screenshotBlk, screenshotCfg);
    loadCubeScreenshotConfig(*screenshotBlk, cubeScreenshotCfg);

    const DataBlock *ortMultiScrBlk = screenshotBlk->getBlockByName("ort_multi_screenshot");
    if (ortMultiScrBlk)
    {
      mScrData.useIt = ortMultiScrBlk->getBool("use_it");
      mScrData.renderObjects = ortMultiScrBlk->getBool("render_objects");
      mScrData.mapPos = ortMultiScrBlk->getPoint2("map_pos");
      mScrData.mapSize = ortMultiScrBlk->getPoint2("map_size");
      mScrData.tileSize = ortMultiScrBlk->getInt("tile_size");
      mScrData.res = ortMultiScrBlk->getReal("resolution");
      mScrData.q_type = ortMultiScrBlk->getInt("quality_type");
      mScrData.mipLevels = ortMultiScrBlk->getInt("mip_levels");
    }
  }
}


//==================================================================================================

void GenericEditorAppWindow::closeScreenshotSettingsDialog() { screenshot_settings_dialog.reset(); }
