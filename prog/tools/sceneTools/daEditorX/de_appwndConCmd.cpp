// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_appwnd.h"
#include <de3_editorEvents.h>
#include <de3_hmapService.h>
#include <de3_genHmapData.h>
#include <de3_interface.h>
#include <de3_dynRenderService.h>
#include <render/debugTexOverlay.h>
#include <generic/dag_sort.h>

#include <oldEditor/de_cm.h>
#include <oldEditor/de_workspace.h>

#include <libTools/util/strUtil.h>

#include <winGuiWrapper/wgw_dialogs.h>
#include <EditorCore/ec_IEditorCore.h>
#include <EditorCore/ec_wndGlobal.h>
#include <osApiWrappers/dag_direct.h>

#include <perfMon/dag_graphStat.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <debug/dag_debug.h>
#include <math/dag_mathUtils.h>
#include <stdio.h>

InitOnDemand<DebugTexOverlay> de3_show_tex_helper;

static Point3 to_point3(dag::ConstSpan<const char *> params, int idx = 0)
{
  if ((idx + 3) <= params.size())
    return Point3(console::to_real(params[idx + 0]), console::to_real(params[idx + 1]), console::to_real(params[idx + 2]));

  return Point3(0, 0, 0);
}

static inline float deg_to_fov(float deg) { return 1.f / tanf(DEG_TO_RAD * 0.5f * deg); }

static inline float fov_to_deg(float fov) { return RAD_TO_DEG * 2.f * atan(1.f / fov); }

static int __cdecl sort_texid_by_refc(const TEXTUREID *a, const TEXTUREID *b)
{
  if (int rc_diff = get_managed_texture_refcount(*a) - get_managed_texture_refcount(*b))
    return -rc_diff;
  return strcmp(get_managed_texture_name(*a), get_managed_texture_name(*b));
}

class DagorEdConsoleCommandProcessor : public console::ICommandProcessor
{
public:
  using ICommandProcessor::ICommandProcessor;

  void destroy() override { delete this; }

  bool processCommand(const char *argv[], int argc) override;

private:
  static void runTexShow(const char *argv[], int argc);
};

//==============================================================================
bool DagorEdConsoleCommandProcessor::processCommand(const char *argv[], int argc)
{
  if (argc < 1)
    return false;

  const dag::Span<const char *> params = make_span(&argv[1], argc - 1);
  int found = 0;

  CONSOLE_CHECK_NAME_EX("", "exit", 1, 2,
    "'exit [save_project]' to exit from daEditor.\nif save_project is set no message box will be shown", "[save_project]")
  {
    CONSOLE_COMMAND_BATCH_MODE_RESULT(get_app().runExitCmd(params));
  }

  CONSOLE_CHECK_NAME_EX("camera", "at", 1, 8,
    "Type:\n"
    "'camera.at' prints the camera's position, direction and FOV in last active viewport.\n"
    "'camera.at pos_x pos_y pos_z [dir_x dir_y dir_z] [fov]' to set camera position, direction and FOV in last active viewport.",
    "")
  {
    get_app().runCameraAtCmd(params);
  }

  CONSOLE_CHECK_NAME_EX("camera", "pos", 1, 4,
    "Type:\n"
    "'camera.pos' to know camera position in last active viewport.\n"
    "'camera.pos x y z' to set camera position in last active viewport.",
    "")
  {
    get_app().runCameraPosCmd(params);
  }

  CONSOLE_CHECK_NAME_EX("camera", "dir", 1, 7,
    "Type:\n"
    "'camera.dir x y z [x_up = 0 y_up = 1 z_up = 0]' to set camera direction in last active viewport.",
    "")
  {
    get_app().runCameraDirCmd(params);
  }

  CONSOLE_CHECK_NAME_EX("camera", "save", 1, 1,
    "Saves the current camera position and direction to the .level.local.blk file.\n"
    "It can be useful if you do not want to save the entire project but want to use the current camera at the next start.",
    "")
  {
    get_app().runCameraSaveCmd(params);
  }

  CONSOLE_CHECK_NAME_EX("project", "open", 1, 4,
    "Type:\n"
    "'project.open file_path [lock = false] [save_project]'\n"
    "where\n"
    "file_path - full path to \"level.blk\" or path relative to project's \"develop\" folder\n"
    "lock - boolean value to determine to lock project or not\n"
    "save_project - boolean value to supress save question message box",
    "")
  {
    CONSOLE_COMMAND_BATCH_MODE_RESULT(get_app().runProjectOpenCmd(params));
  }

  CONSOLE_CHECK_NAME_EX("", "set_workspace", 1, 3,
    "Type:\n"
    "'set_workspace application_blk_path [save_project]' to load workspace from mentioned "
    "\"application.blk\" file\n"
    "where\n"
    "application_blk_path - path to desired \"application.blk\" file. Must be full or "
    "relative to daEditor folder\n"
    "save_project - boolean value to supress save question message box",
    "")
  {
    CONSOLE_COMMAND_BATCH_MODE_RESULT(get_app().runSetWorkspaceCmd(params));
  }

  CONSOLE_CHECK_NAME_EX("project", "export.pc", 1, 2,
    "Type:\n"
    "'project.export.pc level_name' to export project in PC format\n"
    "where\n"
    "level_name - path to exported level relative application levels folder",
    "")
  {
    CONSOLE_COMMAND_BATCH_MODE_RESULT(get_app().runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_PC));
  }

  CONSOLE_CHECK_NAME_EX("project", "export.xbox", 1, 2,
    "Type:\n"
    "'project.export.xbox level_name' to export project in XBOX 360 format\n"
    "where\n"
    "level_name - path to exported level relative application levels folder",
    "")
  {
    CONSOLE_COMMAND_BATCH_MODE_RESULT(get_app().runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_XBOX360));
  }

  CONSOLE_CHECK_NAME_EX("project", "export.ps3", 1, 2,
    "Type:\n"
    "'project.export.ps3 level_name' to export project in PS3 format\n"
    "where\n"
    "level_name - path to exported level relative application levels folder",
    "")
  {
    CONSOLE_COMMAND_BATCH_MODE_RESULT(get_app().runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_PS3));
  }

#if _TARGET_64BIT
  CONSOLE_CHECK_NAME_EX("project", "export.ps4", 1, 2,
    "Type:\n"
    "'project.export.ps4 level_name' to export project in PS4 format\n"
    "where\n"
    "level_name - path to exported level relative application levels folder",
    "")
  {
    CONSOLE_COMMAND_BATCH_MODE_RESULT(get_app().runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_PS4));
  }
#endif

  CONSOLE_CHECK_NAME_EX("project", "export.iOS", 1, 2,
    "Type:\n"
    "'project.export.iOS level_name' to export project in iOS format\n"
    "where\n"
    "level_name - path to exported level relative application levels folder",
    "")
  {
    CONSOLE_COMMAND_BATCH_MODE_RESULT(get_app().runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_iOS));
  }

  CONSOLE_CHECK_NAME_EX("project", "export.and", 1, 2,
    "Type:\n"
    "'project.export.and level_name' to export project in Android (Tegra GPU family) format\n"
    "where\n"
    "level_name - path to exported level relative application levels folder",
    "")
  {
    CONSOLE_COMMAND_BATCH_MODE_RESULT(get_app().runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_AND));
  }

  CONSOLE_CHECK_NAME_EX("project", "export.all", 1, 4,
    "Type:\n"
    "'project.export.all level_name_pc level_name_xbox level_name_ps3'"
    " to export project in all formats\n"
    "where\n"
    "level_name_* - path to exported level relative application levels folder",
    "")
  {
    CONSOLE_COMMAND_BATCH_MODE_RESULT(get_app().runProjectExportCmd(params));
  }

  CONSOLE_CHECK_NAME_EX("screenshot", "ortho", 1, 1, "Outputs orthogonal screenshot using extents from landscape.", "")
  {
    IHmapService *hmlService = EDITORCORE->queryEditorInterface<IHmapService>();
    if (GenHmapData *genHmap = hmlService ? hmlService->findGenHmap("hmap") : NULL)
    {
      float x0z0x1z1[4] = {-1, -1, 1, 1};
      x0z0x1z1[0] = genHmap->getHeightmapOffset().x;
      x0z0x1z1[1] = genHmap->getHeightmapOffset().z;
      x0z0x1z1[2] = x0z0x1z1[0] + genHmap->getHeightmapSizeX() * genHmap->getHeightmapCellSize();
      x0z0x1z1[3] = x0z0x1z1[1] + genHmap->getHeightmapSizeY() * genHmap->getHeightmapCellSize();
      get_app().createOrthogonalScreenshot(NULL, x0z0x1z1);
    }
  }

  CONSOLE_CHECK_NAME_EX("entity", "stat", 1, 1, "Outputs statistics for entity pools to console.", "")
  {
    get_app().spawnEvent(HUID_DumpEntityStat, NULL);
  }

  CONSOLE_CHECK_NAME_EX("entity", "ri_stat", 1, 2,
    "entity.ri_stat [min_vb_size_Kb_to_report]\n"
    "Outputs statistics for RI entities (per-cell), reports all non-empty cells if min VB size is not specified",
    "")
  {
    get_app().spawnEvent(HUID_DumpRiEntityStat, (void *)(uintptr_t)(params.size() == 1 ? atoi(params[0]) << 10 : 0));
  }

  CONSOLE_CHECK_NAME_EX("project", "tex_metrics", 1, 3,
    "Computes and outputs tex metrics to console.\n  project.tex_metrics [verbose] [nodlg]", "")
  {
    Tab<IGenEditorPlugin *> buildPlugin(tmpmem);
    Tab<bool *> doExport(tmpmem), doExternal(tmpmem);
    unsigned targetCode = _MAKE4C('PC');
    bool verbose = false;

    for (auto *p : params)
      if (strcmp(p, "verbose") == 0)
        verbose = true;
      else if (strcmp(p, "nodlg") == 0)
        get_app().mNeedSuppress = true;
      else
        DAEDITOR3.conError("unsupported arg '%s' for command '%s'", p, argv[0]);

    if (!get_app().showPluginDlg(buildPlugin, doExport, doExternal))
      return found != 0;

    Tab<IBinaryDataBuilder *> exporters(tmpmem);

    for (int i = 0; i < buildPlugin.size(); ++i)
      if (*doExport[i])
      {
        if (IOnExportNotify *iface = buildPlugin[i]->queryInterface<IOnExportNotify>())
          iface->onBeforeExport(targetCode);
        if (IBinaryDataBuilder *iface = buildPlugin[i]->queryInterface<IBinaryDataBuilder>())
          exporters.push_back(iface);
      }

    OAHashNameMap<true> levelResList;
    for (int i = 0; i < buildPlugin.size(); i++)
      if (*doExport[i])
        if (ILevelResListBuilder *iface = buildPlugin[i]->queryInterface<ILevelResListBuilder>())
          iface->addUsedResNames(levelResList);
    TextureRemapHelper trh(targetCode);

    int64_t texSize = 0;
    int texCount = 0;
    get_app().gatherUsedResStats(exporters, targetCode, levelResList, trh, texSize, texCount, verbose);
    for (int i = 0; i < buildPlugin.size(); ++i)
      if (*doExport[i])
        if (IOnExportNotify *iface = buildPlugin[i]->queryInterface<IOnExportNotify>())
          iface->onAfterExport(targetCode);
    get_app().mNeedSuppress = false;

    DataBlock metrixBlk;
    if (get_app().wsp->getMetricsBlk(metrixBlk))
      if (const DataBlock *levelMetricsBlk = metrixBlk.getBlockByName("whole_level"))
      {
        const int custom_tex_cnt = trh.getCustomMetricsTexCount();
        const float custom_tex_sz = trh.getCustomMetricsTexSizeMb();
        const int maxTexCount = custom_tex_cnt < 0 ? levelMetricsBlk->getInt("textures_count", 0) : custom_tex_cnt;
        const int64_t maxTexSize = (custom_tex_sz < 0 ? levelMetricsBlk->getReal("textures_size", 0.f) : custom_tex_sz) * (1 << 20);
        DAEDITOR3.conNote("Actual metrics: max %d textures, max %dM texture size%s", maxTexCount, maxTexSize >> 20,
          (custom_tex_cnt >= 0 || custom_tex_sz >= 0) ? " [using custom metrics from level-BLK]" : "");
      }
  }

  CONSOLE_CHECK_NAME_EX("shaders", "list", 1, 2, "", "[filter-string]") { get_app().runShadersListVars(params); }

  CONSOLE_CHECK_NAME_EX("shaders", "set", 1, 6, "", "<shader-variable-name> <value>") { get_app().runShadersSetVar(params); }

  CONSOLE_CHECK_NAME_EX("", "shaderVar", 1, 6, "", "<shader-variable-name> <value>") { get_app().runShadersSetVar(params); }

  CONSOLE_CHECK_NAME_EX("shaders", "reload", 1, 2, "", "[shaders-binary-dump-filename]") { get_app().runShadersReload(params); }

  CONSOLE_CHECK_NAME("perf", "on", 1, 1) { EDITORCORE->queryEditorInterface<IRenderHelperService>()->enableFrameProfiler(true); }

  CONSOLE_CHECK_NAME("perf", "off", 1, 1) { EDITORCORE->queryEditorInterface<IRenderHelperService>()->enableFrameProfiler(false); }

  CONSOLE_CHECK_NAME("perf", "dump", 1, 1)
  {
    EDITORCORE->queryEditorInterface<IRenderHelperService>()->profilerDumpFrame();
    get_app().repaint();
  }

  CONSOLE_CHECK_NAME_EX("tex", "info", 1, 2, "", "[full]")
  {
    if (params.size() == 0 || (params.size() == 1 && strcmp(params[0], "full") == 0))
    {
      uint32_t num_textures = 0;
      uint64_t total_mem = 0;
      String texStat;
      d3d::get_texture_statistics(&num_textures, &total_mem, &texStat);
      char *str = texStat.str();
      if (params.size() == 1)
      {
        while (char *p = strchr(str, '\n'))
        {
          *p = 0;
          DAEDITOR3.conNote(str);
          str = p + 1;
        }
        DAEDITOR3.conNote(str);
      }
      else
        DAEDITOR3.conNote("%d tex use %dM of GPU memory", num_textures, total_mem >> 20);
    }
  }

  CONSOLE_CHECK_NAME("tex", "refs", 1, 1)
  {
    Tab<TEXTUREID> ids;
    ids.reserve(32 < 10);
    for (TEXTUREID i = first_managed_texture(1); i != BAD_TEXTUREID; i = next_managed_texture(i, 1))
      ids.push_back(i);
    sort(ids, &sort_texid_by_refc);

    for (TEXTUREID texId : ids)
      DAEDITOR3.conNote("  [%5d] refc=%-3d %s", texId, get_managed_texture_refcount(texId), get_managed_texture_name(texId));
    DAEDITOR3.conNote("total %d referenced textures", ids.size());
  }

  CONSOLE_CHECK_NAME("tex", "show", 1, DebugTexOverlay::MAX_CONSOLE_ARGS_CNT) { runTexShow(argv, argc); }

  // This is the same as the "tex.show" command but the "Console commands and variables" debug dialog executes this command.
  CONSOLE_CHECK_NAME("render", "show_tex", 1, DebugTexOverlay::MAX_CONSOLE_ARGS_CNT) { runTexShow(argv, argc); }

  CONSOLE_CHECK_NAME("tex", "hide", 1, 1)
  {
    if (de3_show_tex_helper.get())
      de3_show_tex_helper->hideTex();
  }

  CONSOLE_CHECK_NAME("driver", "reset", 1, 1) { dagor_d3d_force_driver_reset = true; }

  CONSOLE_CHECK_NAME("render", "puddles", 1, 4)
  {
    IHmapService *hmlService = EDITORCORE->queryEditorInterface<IHmapService>();
    if (!hmlService)
      return found != 0;

    static int puddles_powerVarId = ::get_shader_glob_var_id("puddles_power", true);
    static int puddles_seedVarId = ::get_shader_glob_var_id("puddles_seed", true);
    static int puddles_noise_influenceVarId = ::get_shader_glob_var_id("puddles_noise_influence", true);

    if (params.size() == 0)
    {
      DAEDITOR3.conNote("puddles power %f puddles_seed %f puddles_noise_influence %f", ShaderGlobal::get_float(puddles_powerVarId),
        ShaderGlobal::get_float(puddles_seedVarId), ShaderGlobal::get_float(puddles_noise_influenceVarId));
    }

    if (params.size() >= 1)
    {
      ShaderGlobal::set_float(puddles_powerVarId, atof(params[0]));
    }
    if (params.size() >= 2)
    {
      ShaderGlobal::set_float(puddles_seedVarId, atof(params[1]));
    }
    if (params.size() >= 3)
    {
      ShaderGlobal::set_float(puddles_noise_influenceVarId, atof(params[2]));
    }

    hmlService->invalidateClipmap(false, false);
  }

  return found != 0;
}

void DagorEdConsoleCommandProcessor::runTexShow(const char *argv[], int argc)
{
  de3_show_tex_helper.demandInit();

  String str = de3_show_tex_helper->processConsoleCmd(argv, argc);
  if (!str.empty())
    DAEDITOR3.conNote(str);
}

//==================================================================================================
void DagorEdAppWindow::registerConsoleCommands()
{
  console::init();
  add_con_proc(new DagorEdConsoleCommandProcessor(10000)); // Let daEditorX override all other commands.
}

//==============================================================================
bool DagorEdAppWindow::runExitCmd(dag::ConstSpan<const char *> params)
{
  mMsgBoxResult = -1;
  if (params.size())
  {
    mNeedSuppress = true;
    mMsgBoxResult = console::to_bool(params[0]) ? wingw::MB_ID_YES : wingw::MB_ID_NO;
  }

  if (on_batch_exit)
    on_batch_exit();

  onMenuItemClick(CM_EXIT);

  mNeedSuppress = false;
  mMsgBoxResult = -1;

  return true;
}


//==============================================================================
void DagorEdAppWindow::runCameraAtCmd(dag::ConstSpan<const char *> params)
{
  IGenViewportWnd *vpw = getCurrentViewport();
  if (!vpw)
    return;

  if (params.size() == 0)
  {
    TMatrix tm;
    vpw->getCameraTransform(tm);

    const float fovRad = vpw->getFov();
    const float fovDeg = fovRad > 0.0f ? fov_to_deg(fovRad) : -1.0f;

    console->addMessage(ILogWriter::NOTE, "\tcamera.at %g %g %g %g %g %g %g", P3D(tm.getcol(3)), P3D(tm.getcol(2)), fovDeg);
    return;
  }
  else if (params.size() == 3 || params.size() == 6 || params.size() == 7)
  {
    TMatrix tm;
    vpw->getCameraTransform(tm);

    tm.setcol(3, to_point3(params));
    if (params.size() >= 6)
      lookAt(tm.getcol(3), tm.getcol(3) + to_point3(params, 3), Point3(0, 1, 0), tm);

    vpw->setCameraTransform(tm);

    if (params.size() == 7)
    {
      const float fovDeg = console::to_real(params[6]);
      if (fovDeg > 0.0f)
        vpw->setFov(deg_to_fov(fovDeg));
    }

    return;
  }

  console->runHelp("camera.at");
}


//==============================================================================
void DagorEdAppWindow::runCameraPosCmd(dag::ConstSpan<const char *> params)
{
  IGenViewportWnd *vpw = getCurrentViewport();
  if (!vpw)
    return;

  if (!params.size())
  {
    TMatrix tm;
    vpw->getCameraTransform(tm);

    console->addMessage(ILogWriter::NOTE, "Current camera position: %g, %g, %g", P3D(tm.getcol(3)));

    return;
  }
  else if (params.size() >= 3)
  {
    vpw->setCameraPos(to_point3(params, 0));
    return;
  }

  console->runHelp("camera.pos");
}


//==============================================================================
void DagorEdAppWindow::runCameraDirCmd(dag::ConstSpan<const char *> params)
{
  IGenViewportWnd *vpw = getCurrentViewport();
  if (!vpw)
    return;

  if (params.size() >= 3)
  {
    Point3 p3 = to_point3(params, 0);
    Point3 up = params.size() >= 6 ? to_point3(params, 3) : Point3(0, 1, 0);

    vpw->setCameraDirection(::normalize(p3), ::normalize(up));
    return;
  }

  console->runHelp("camera.dir");
}


//==============================================================================
void DagorEdAppWindow::runCameraSaveCmd(dag::ConstSpan<const char *> /*params*/)
{
  ViewportWindow *viewport = ged.getViewport(0);
  if (!viewport)
  {
    console->addMessage(ILogWriter::ERROR, "No viewport. Cannot save camera.");
    return;
  }

  if (sceneFname[0] == 0)
  {
    console->addMessage(ILogWriter::ERROR, "The project has never been saved. Cannot save camera.");
    return;
  }

  String localSetPath;
  DataBlock localBlk;
  prepareLocalLevelBlk(sceneFname, localSetPath, localBlk);

  const TMatrix viewTm = viewport->getViewTm();
  DataBlock *viewportBlk = localBlk.addBlock("viewport0");
  viewportBlk->setPoint3("viewMatrix0", viewTm.getcol(0));
  viewportBlk->setPoint3("viewMatrix1", viewTm.getcol(1));
  viewportBlk->setPoint3("viewMatrix2", viewTm.getcol(2));
  viewportBlk->setPoint3("viewMatrix3", viewTm.getcol(3));

  if (!localBlk.saveToTextFile(localSetPath))
  {
    console->addMessage(ILogWriter::ERROR, "Error saving \"%s\".", localSetPath.c_str());
    return;
  }

  console->addMessage(ILogWriter::NOTE, "The camera has been updated in \"%s\".", localSetPath.c_str());
}


//==============================================================================
bool DagorEdAppWindow::runProjectOpenCmd(dag::ConstSpan<const char *> params)
{

  if (params.size())
  {
    mNeedSuppress = true;
    bool lock = false;
    String projectPath = ::make_full_path(getSdkDir(), params[0]);
    mSuppressDlgResult = projectPath;

    if (params.size() >= 2)
      lock = console::to_bool(params[1]);

    mMsgBoxResult = -1;
    if (params.size() >= 3)
      mMsgBoxResult = console::to_bool(params[2]) ? wingw::MB_ID_YES : wingw::MB_ID_NO;

    bool result = handleOpenProject(lock);
    setDocTitle();

    mNeedSuppress = false;
    mSuppressDlgResult = "";
    mMsgBoxResult = -1;

    return result;
  }


  console->runHelp("project.open");
  return false;
}


//==============================================================================
bool DagorEdAppWindow::runProjectExportCmd(dag::ConstSpan<const char *> params, int export_fmt)
{
  if (params.size())
  {
    mNeedSuppress = true;

    String outPath = ::make_full_path(wsp->getLevelsBinDir(), params[0]);
    mSuppressDlgResult = outPath;
    mMsgBoxResult = wingw::MB_ID_OK;

    onMenuItemClick(export_fmt);

    mNeedSuppress = false;
    mSuppressDlgResult = "";
    mMsgBoxResult = -1;

    return true;
  }

  return false;
}

//==============================================================================

bool check_path_valid(const char *path, const char *pc_path)
{
  if (!strcmp(path, pc_path))
    return false;

  char buf[260];
  dd_get_fname_location(buf, path);
  if (!dd_dir_exist(buf))
    return false;

  return true;
}

bool DagorEdAppWindow::runProjectExportCmd(dag::ConstSpan<const char *> params)
{
  SimpleString _pc_path(params.size() ? ::make_full_path(wsp->getLevelsBinDir(), params[0]).str() : getExportPath(_MAKE4C('PC')));

  SimpleString _ps4_path(
    params.size() > 2 ? ::make_full_path(wsp->getLevelsBinDir(), params[2]).str() : getExportPath(_MAKE4C('PS4')));

  SimpleString _iOS_path(
    params.size() > 2 ? ::make_full_path(wsp->getLevelsBinDir(), params[2]).str() : getExportPath(_MAKE4C('iOS')));

  SimpleString _and_path(
    params.size() > 2 ? ::make_full_path(wsp->getLevelsBinDir(), params[2]).str() : getExportPath(_MAKE4C('and')));


  mMsgBoxResult = wingw::MB_ID_OK;

  mNeedSuppress = check_path_valid(_pc_path, "");
  mSuppressDlgResult = _pc_path;
  exportLevelToGame(_MAKE4C('PC'));

  mNeedSuppress = check_path_valid(_ps4_path, _pc_path);
  mSuppressDlgResult = _ps4_path;
  exportLevelToGame(_MAKE4C('PS4'));

  mNeedSuppress = check_path_valid(_iOS_path, _pc_path);
  mSuppressDlgResult = _iOS_path;
  exportLevelToGame(_MAKE4C('iOS'));

  mNeedSuppress = check_path_valid(_and_path, _pc_path);
  mSuppressDlgResult = _and_path;
  exportLevelToGame(_MAKE4C('and'));

  mNeedSuppress = false;
  mSuppressDlgResult = "";
  mMsgBoxResult = -1;

  return true;
}


//==============================================================================
bool DagorEdAppWindow::runSetWorkspaceCmd(dag::ConstSpan<const char *> params)
{
  if (params.size())
  {
    mNeedSuppress = true;
    if (params.size() >= 2)
      mMsgBoxResult = console::to_bool(params[1]) ? wingw::MB_ID_YES : wingw::MB_ID_NO;

    String wspPath = ::make_full_path(sgg::get_exe_path_full(), params[0]);

    debug("loading wsp from %s", (const char *)wspPath);

    bool result = selectWorkspace(wspPath);
    mNeedSuppress = false;
    mMsgBoxResult = -1;

    if (result)
      return true;

    console->addMessage(ILogWriter::FATAL, "\"set_workspace\" command run failed. Couldn't load workspace from \"%s\"", wspPath);
  }

  return false;
}

bool DagorEdAppWindow::runShadersListVars(dag::ConstSpan<const char *> params)
{
  unsigned found = 0;
  console->addMessage(ILogWriter::NOTE, "listing shader vars (%d total):", VariableMap::getGlobalVariablesCount());
  for (int i = 0, ie = VariableMap::getGlobalVariablesCount(); i < ie; i++)
  {
    const char *name = VariableMap::getGlobalVariableName(i);
    if (!name)
      continue;
    if (params.size() && !strstr(String(name).toLower(), String(params[0]).toLower()))
      continue;

    int varId = VariableMap::getVariableId(name);
    int type = ShaderGlobal::get_var_type(varId);
    if (type < 0)
      continue;

    Color4 c;
    found++;
    switch (type)
    {
      case SHVT_INT: console->addMessage(ILogWriter::NOTE, "[int]   %-40s %d", name, ShaderGlobal::get_int_fast(varId)); break;
      case SHVT_REAL: console->addMessage(ILogWriter::NOTE, "[real]  %-40s %.3f", name, ShaderGlobal::get_float(varId)); break;
      case SHVT_INT4: console->addMessage(ILogWriter::NOTE, "[int4]  %-40s %@", name, ShaderGlobal::get_int4(varId)); break;
      case SHVT_COLOR4:
        c = ShaderGlobal::get_float4(varId);
        console->addMessage(ILogWriter::NOTE, "[color] %-40s %.3f,%.3f,%.3f,%.3f", name, c.r, c.g, c.b, c.a);
        break;
      case SHVT_TEXTURE:
      {
        TEXTUREID tid = ShaderGlobal::get_tex_fast(varId);
        BaseTexture *tptr = ShaderGlobal::get_tex_ptr_fast(varId);
        if (tid == BAD_TEXTUREID && tptr)
          console->addMessage(ILogWriter::NOTE, "[tex]   %-40s %p", tptr);
        else
          console->addMessage(ILogWriter::NOTE, "[tex]   %-40s %s", name,
            tid == BAD_TEXTUREID ? "---" : get_managed_texture_name(tid));
      }
      break;
    }
  }
  if (!found)
    console->addMessage(ILogWriter::NOTE, "  no vars (containing \"%s\") found", params.empty() ? "" : params[0]);
  return true;
}
bool DagorEdAppWindow::runShadersSetVar(dag::ConstSpan<const char *> params)
{
  if (params.size() < 1)
  {
    console->addMessage(ILogWriter::ERROR, "shaders.set requires <name> and <value>");
    return false;
  }
  int varId = VariableMap::getVariableId(params[0]);
  int type = ShaderGlobal::get_var_type(varId);
  if (type < 0)
  {
    console->addMessage(ILogWriter::ERROR, "<%s> is not global shader var", params[0]);
    return false;
  }
  if (params.size() > 1)
  {
    switch (type)
    {
      case SHVT_INT: ShaderGlobal::set_int(varId, atoi(params[1])); break;
      case SHVT_REAL: ShaderGlobal::set_float(varId, atof(params[1])); break;
      case SHVT_COLOR4:
        if (params.size() < 5)
        {
          console->addMessage(ILogWriter::ERROR, "shaders.set requires <name> <R> <G> <B> <A> for color var");
          return false;
        }
        ShaderGlobal::set_float4(varId, atof(params[1]), atof(params[2]), atof(params[3]), atof(params[4]));
        break;
      case SHVT_TEXTURE:
        TEXTUREID tex_id = get_managed_texture_id(params[1]);
        if (tex_id == BAD_TEXTUREID && !strchr(params[1], '*') && dd_file_exist(params[1]))
          tex_id = add_managed_texture(params[1]);
        if (tex_id != BAD_TEXTUREID)
          ShaderGlobal::set_texture(varId, tex_id);
        else
          console->addMessage(ILogWriter::ERROR, "<%s> is not valid texture", params[1]);
        break;
    }
  }
  switch (type)
  {
    case SHVT_INT:
      console->addMessage(ILogWriter::NOTE, "globalvar <%s> [int] set to %d", params[0], ShaderGlobal::get_int_fast(varId));
      break;
    case SHVT_REAL:
      console->addMessage(ILogWriter::NOTE, "globalvar <%s> [real] set to %.3f", params[0], ShaderGlobal::get_float(varId));
      break;
    case SHVT_COLOR4:
    {
      Color4 val = ShaderGlobal::get_float4(varId);
      console->addMessage(ILogWriter::NOTE, "globalvar <%s> [color] set to %.3f,%.3f,%.3f,%.3f", params[0], val.r, val.g, val.b,
        val.a);
    }
    break;
    case SHVT_TEXTURE:
    {
      TEXTUREID tex_id = ShaderGlobal::get_tex_fast(varId);
      console->addMessage(ILogWriter::NOTE, "globalvar <%s> [tex] set to %s [0x%x]", params[0], get_managed_texture_name(tex_id),
        tex_id);
    }
    break;
  }
  return true;
}

bool DagorEdAppWindow::runShadersReload(dag::ConstSpan<const char *> params)
{
  G_ASSERT(d3d::is_inited());
  auto shaderModel = d3d::get_driver_desc().shaderModel;

  DataBlock appblk(DAGORED2->getWorkspace().getAppBlkPath());
  String sh_file;
  if (appblk.getStr("shaders", NULL))
    sh_file.printf(260, "%s/%s", DAGORED2->getWorkspace().getAppDir(), appblk.getStr("shaders", NULL));
  else
    sh_file.printf(260, "%s/compiledShaders/classic/tools", sgg::get_common_data_dir());
  simplify_fname(sh_file);

  const char *shname = params.size() > 0 ? params[0] : sh_file;

  String fileName;
  for (auto version : d3d::smAll)
  {
    if (shaderModel < version)
      continue;
    fileName.printf(260, "%s.%s.shdump.bin", shname, version.psName);
    if (!dd_file_exist(fileName))
      continue;
    if (!load_shaders_bindump(shname, version))
    {
      console->addMessage(ILogWriter::FATAL, "failed to reload %s, ver=%s", shname, version.psName);
      return false;
    }
    console->addMessage(ILogWriter::NOTE, "reloaded %s, ver=%s", shname, version.psName);
    return true;
  }
  console->addMessage(ILogWriter::FATAL, "failed to reload %s, dsc.shaderModel=%s", shname, d3d::as_string(shaderModel));
  return false;
}
