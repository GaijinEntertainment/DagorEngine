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
#include <sepGui/wndGlobal.h>
#include <osApiWrappers/dag_direct.h>

#include <perfMon/dag_graphStat.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <debug/dag_debug.h>
#include <stdio.h>

InitOnDemand<DebugTexOverlay> de3_show_tex_helper;

//==================================================================================================
void DagorEdAppWindow::registerConsoleCommands()
{
#define REGISTER_COMMAND(cmd_name)               \
  if (!console->registerCommand(cmd_name, this)) \
    console->addMessage(ILogWriter::ERROR, "[DaEditorX] Couldn't register command '" cmd_name "'");

  REGISTER_COMMAND("exit");
  REGISTER_COMMAND("set_workspace");

  REGISTER_COMMAND("camera.pos");
  REGISTER_COMMAND("camera.dir");

  REGISTER_COMMAND("project.open");
  REGISTER_COMMAND("project.export.pc");
  REGISTER_COMMAND("project.export.xbox");
  REGISTER_COMMAND("project.export.ps3");
#if _TARGET_64BIT
  REGISTER_COMMAND("project.export.ps4");
#endif
  REGISTER_COMMAND("project.export.iOS");
  REGISTER_COMMAND("project.export.and");
  REGISTER_COMMAND("project.export.all");
  REGISTER_COMMAND("screenshot.ortho");

  REGISTER_COMMAND("entity.stat");
  REGISTER_COMMAND("shaders.list");
  REGISTER_COMMAND("shaders.set");
  REGISTER_COMMAND("shaderVar");
  REGISTER_COMMAND("shaders.reload");

  REGISTER_COMMAND("perf.on");
  REGISTER_COMMAND("perf.off");
  REGISTER_COMMAND("perf.dump");

  REGISTER_COMMAND("tex.info");
  REGISTER_COMMAND("tex.refs");
  REGISTER_COMMAND("tex.show");
  REGISTER_COMMAND("tex.hide");
  REGISTER_COMMAND("project.tex_metrics");

  REGISTER_COMMAND("driver.reset");

  REGISTER_COMMAND("render.puddles");

#undef REGISTER_COMMAND
}

static int __cdecl sort_texid_by_refc(const TEXTUREID *a, const TEXTUREID *b)
{
  if (int rc_diff = get_managed_texture_refcount(*a) - get_managed_texture_refcount(*b))
    return -rc_diff;
  return strcmp(get_managed_texture_name(*a), get_managed_texture_name(*b));
}

//==============================================================================
bool DagorEdAppWindow::onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params)
{
  if (!stricmp(cmd, "exit"))
    return runExitCmd(params);

  if (!stricmp(cmd, "camera.pos"))
    return runCameraPosCmd(params);

  if (!stricmp(cmd, "camera.dir"))
    return runCameraDirCmd(params);

  if (!stricmp(cmd, "project.open"))
    return runProjectOpenCmd(params);

  if (!stricmp(cmd, "set_workspace"))
    return runSetWorkspaceCmd(params);

  if (!stricmp(cmd, "project.export.pc"))
    return runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_PC);

  if (!stricmp(cmd, "project.export.xbox"))
    return runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_XBOX360);

  if (!stricmp(cmd, "project.export.ps3"))
    return runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_PS3);

  if (!stricmp(cmd, "project.export.ps4"))
    return runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_PS4);

  if (!stricmp(cmd, "project.export.iOS"))
    return runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_iOS);

  if (!stricmp(cmd, "project.export.and"))
    return runProjectExportCmd(params, CM_FILE_EXPORT_TO_GAME_AND);

  if (!stricmp(cmd, "project.export.all"))
    return runProjectExportCmd(params);

  if (!stricmp(cmd, "screenshot.ortho"))
  {
    IHmapService *hmlService = EDITORCORE->queryEditorInterface<IHmapService>();
    if (GenHmapData *genHmap = hmlService ? hmlService->findGenHmap("hmap") : NULL)
    {
      float x0z0x1z1[4] = {-1, -1, 1, 1};
      x0z0x1z1[0] = genHmap->getHeightmapOffset().x;
      x0z0x1z1[1] = genHmap->getHeightmapOffset().z;
      x0z0x1z1[2] = x0z0x1z1[0] + genHmap->getHeightmapSizeX() * genHmap->getHeightmapCellSize();
      x0z0x1z1[3] = x0z0x1z1[1] + genHmap->getHeightmapSizeY() * genHmap->getHeightmapCellSize();
      createOrthogonalScreenshot(NULL, x0z0x1z1);
      return true;
    }
    return false;
  }

  if (!stricmp(cmd, "entity.stat"))
  {
    spawnEvent(HUID_DumpEntityStat, NULL);
    return true;
  }
  if (!stricmp(cmd, "project.tex_metrics"))
  {
    Tab<IGenEditorPlugin *> buildPlugin(tmpmem);
    Tab<bool *> doExport(tmpmem), doExternal(tmpmem);
    unsigned targetCode = _MAKE4C('PC');
    bool verbose = false;

    for (auto *p : params)
      if (strcmp(p, "verbose") == 0)
        verbose = true;
      else if (strcmp(p, "nodlg") == 0)
        mNeedSuppress = true;
      else
        DAEDITOR3.conError("unsupported arg '%s' for command '%s'", p, cmd);

    if (!showPluginDlg(buildPlugin, doExport, doExternal))
      return true;

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
    gatherUsedResStats(exporters, targetCode, levelResList, trh, texSize, texCount, verbose);
    for (int i = 0; i < buildPlugin.size(); ++i)
      if (*doExport[i])
        if (IOnExportNotify *iface = buildPlugin[i]->queryInterface<IOnExportNotify>())
          iface->onAfterExport(targetCode);
    mNeedSuppress = false;
    return true;
  }

  if (!stricmp(cmd, "shaders.list"))
    return runShadersListVars(params);
  if (!stricmp(cmd, "shaders.set"))
    return runShadersSetVar(params);
  if (!stricmp(cmd, "shaderVar"))
    return runShadersSetVar(params);
  if (!stricmp(cmd, "shaders.reload"))
    return runShadersReload(params);

  if (!stricmp(cmd, "perf.on"))
  {
    EDITORCORE->queryEditorInterface<IDynRenderService>()->enableFrameProfiler(true);
    return true;
  }
  if (!stricmp(cmd, "perf.off"))
  {
    EDITORCORE->queryEditorInterface<IDynRenderService>()->enableFrameProfiler(false);
    return true;
  }
  if (!stricmp(cmd, "perf.dump"))
  {
    EDITORCORE->queryEditorInterface<IDynRenderService>()->profilerDumpFrame();
    repaint();
    return true;
  }
  if (!stricmp(cmd, "tex.info"))
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
  if (!stricmp(cmd, "tex.refs"))
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
  if (!stricmp(cmd, "tex.show"))
  {
    de3_show_tex_helper.demandInit();

    Tab<const char *> argv;
    argv.push_back("app.tex");
    append_items(argv, params.size(), params.data());
    String str = de3_show_tex_helper->processConsoleCmd(argv.data(), argv.size());
    if (!str.empty())
      DAEDITOR3.conNote(str);
    return true;
  }
  if (!stricmp(cmd, "tex.hide"))
  {
    if (de3_show_tex_helper.get())
      de3_show_tex_helper->hideTex();
    return true;
  }
  if (!stricmp(cmd, "driver.reset"))
  {
    dagor_d3d_force_driver_reset = true;
    return true;
  }
  if (!stricmp(cmd, "render.puddles"))
  {
    IHmapService *hmlService = EDITORCORE->queryEditorInterface<IHmapService>();
    if (!hmlService)
      return true;

    static int puddles_powerVarId = ::get_shader_glob_var_id("puddles_power", true);
    static int puddles_seedVarId = ::get_shader_glob_var_id("puddles_seed", true);
    static int puddles_noise_influenceVarId = ::get_shader_glob_var_id("puddles_noise_influence", true);

    if (params.size() == 0)
    {
      DAEDITOR3.conNote("puddles power %f puddles_seed %f puddles_noise_influence %f", ShaderGlobal::get_real(puddles_powerVarId),
        ShaderGlobal::get_real(puddles_seedVarId), ShaderGlobal::get_real(puddles_noise_influenceVarId));
    }

    if (params.size() >= 1)
    {
      ShaderGlobal::set_real(puddles_powerVarId, atof(params[0]));
    }
    if (params.size() >= 2)
    {
      ShaderGlobal::set_real(puddles_seedVarId, atof(params[1]));
    }
    if (params.size() >= 3)
    {
      ShaderGlobal::set_real(puddles_noise_influenceVarId, atof(params[2]));
    }

    hmlService->invalidateClipmap(false, false);
    return true;
  }
  return false;
}


//==============================================================================
const char *DagorEdAppWindow::onConsoleCommandHelp(const char *cmd)
{
  if (!stricmp(cmd, "exit"))
    return "Type:\n"
           "'exit [save_project]' to exit from daEditor.\n"
           "if save_project is set no message box will be shown\n";

  if (!stricmp(cmd, "camera.pos"))
    return "Type:\n"
           "'camera.pos' to know camera position in last active viewport.\n"
           "'camera.pos x y z' to set camera position in last active viewport.\n";

  if (!stricmp(cmd, "camera.dir"))
    return "Type:\n"
           "'camera.dir x y z [x_up = 0 y_up = 1 z_up = 0]' to set camera "
           "direction in last active viewport.\n";

  if (!stricmp(cmd, "project.open"))
    return "Type:\n"
           "'project.open file_path [lock = false] [save_project]'\n"
           "where\n"
           "file_path - full path to \"level.blk\" or path relative to project's \"develop\" folder\n"
           "lock - boolean value to determine to lock project or not\n"
           "save_project - boolean value to supress save question message box\n";

  if (!stricmp(cmd, "project.export.pc"))
    return "Type:\n"
           "'project.export.pc level_name' to export project in PC format\n"
           "where\n"
           "level_name - path to exported level relative application levels folder\n";

  if (!stricmp(cmd, "project.export.xbox"))
    return "Type:\n"
           "'project.export.xbox level_name' to export project in XBOX 360 format\n"
           "where\n"
           "level_name - path to exported level relative application levels folder\n";

  if (!stricmp(cmd, "project.export.ps3"))
    return "Type:\n"
           "'project.export.ps3 level_name' to export project in PS3 format\n"
           "where\n"
           "level_name - path to exported level relative application levels folder\n";

  if (!stricmp(cmd, "project.export.ps4"))
    return "Type:\n"
           "'project.export.ps4 level_name' to export project in PS4 format\n"
           "where\n"
           "level_name - path to exported level relative application levels folder\n";

  if (!stricmp(cmd, "project.export.iOS"))
    return "Type:\n"
           "'project.export.iOS level_name' to export project in iOS format\n"
           "where\n"
           "level_name - path to exported level relative application levels folder\n";

  if (!stricmp(cmd, "project.export.and"))
    return "Type:\n"
           "'project.export.and level_name' to export project in Android (Tegra GPU family) format\n"
           "where\n"
           "level_name - path to exported level relative application levels folder\n";

  if (!stricmp(cmd, "project.export.all"))
    return "Type:\n"
           "'project.export.all level_name_pc level_name_xbox level_name_ps3'"
           " to export project in all formats\n"
           "where\n"
           "level_name_* - path to exported level relative application levels folder\n";

  if (!stricmp(cmd, "set_workspace"))
    return "Type:\n"
           "'set_workspace application_blk_path [save_project]' to load workspace from mentioned "
           "\"application.blk\" file\n"
           "where\n"
           "application_blk_path - path to desired \"application.blk\" file. Must be full or "
           "relative to daEditor folder\n"
           "save_project - boolean value to supress save question message box\n";

  if (!stricmp(cmd, "screenshot.ortho"))
    return "Outputs orthogonal screenshot using extents from landscape.";

  if (!stricmp(cmd, "entity.stat"))
    return "Outputs statistics for entity pools to console.";
  if (!stricmp(cmd, "project.tex_metrics"))
    return "Computes and outputs tex metrics to console.\n  project.tex_metrics [verbose] [nodlg]";

  if (!stricmp(cmd, "shaders.list"))
    return "shaders.list [name_substr]";
  if (!stricmp(cmd, "shaders.set"))
    return "shaders.set <shader-var-name> <value>";
  if (!stricmp(cmd, "shaders.reload"))
    return "shaders.reload [shaders-binary-dump-fname]";
  if (!stricmp(cmd, "tex.info"))
    return "tex.info [full]";
  if (!stricmp(cmd, "tex.refs"))
    return "tex.refs";
  if (!stricmp(cmd, "driver.reset"))
    return "driver.reset";
  if (!stricmp(cmd, "tex.show"))
  {
    static String str;
    de3_show_tex_helper.demandInit();
    str = de3_show_tex_helper->processConsoleCmd(NULL, 0);
    if (!str.empty())
      return str;
  }
  if (!stricmp(cmd, "tex.hide"))
    return "tex.hide";

  return NULL;
}


//==============================================================================
bool DagorEdAppWindow::runExitCmd(dag::ConstSpan<const char *> params)
{
  mMsgBoxResult = -1;
  if (params.size())
  {
    mNeedSuppress = true;
    mMsgBoxResult = strToBool(params[0]) ? wingw::MB_ID_YES : wingw::MB_ID_NO;
  }

  if (on_batch_exit)
    on_batch_exit();

  onMenuItemClick(CM_EXIT);

  mNeedSuppress = false;
  mMsgBoxResult = -1;

  return true;
}


//==============================================================================
bool DagorEdAppWindow::runCameraPosCmd(dag::ConstSpan<const char *> params)
{
  IGenViewportWnd *vpw = getCurrentViewport();
  if (!vpw)
    return false;

  if (!params.size())
  {
    TMatrix tm;
    vpw->getCameraTransform(tm);

    console->addMessage(ILogWriter::NOTE, "Current camera position: %g, %g, %g", P3D(tm.getcol(3)));

    return true;
  }
  else if (params.size() >= 3)
  {
    vpw->setCameraPos(strToPoint3(params, 0));
    return true;
  }

  console->addMessage(ILogWriter::NOTE, onConsoleCommandHelp("camera.pos"));
  return false;
}


//==============================================================================
bool DagorEdAppWindow::runCameraDirCmd(dag::ConstSpan<const char *> params)
{
  IGenViewportWnd *vpw = getCurrentViewport();
  if (!vpw)
    return false;

  if (params.size() >= 3)
  {
    Point3 p3 = strToPoint3(params, 0);
    Point3 up = params.size() >= 6 ? strToPoint3(params, 3) : Point3(0, 1, 0);

    vpw->setCameraDirection(::normalize(p3), ::normalize(up));
    return true;
  }

  console->addMessage(ILogWriter::NOTE, onConsoleCommandHelp("camera.dir"));
  return false;
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
      lock = strToBool(params[1]);

    mMsgBoxResult = -1;
    if (params.size() >= 3)
      mMsgBoxResult = strToBool(params[2]) ? wingw::MB_ID_YES : wingw::MB_ID_NO;

    bool result = handleOpenProject(lock);

    mNeedSuppress = false;
    mSuppressDlgResult = "";
    mMsgBoxResult = -1;

    return result;
  }


  console->addMessage(ILogWriter::NOTE, onConsoleCommandHelp("project.open"));
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
      mMsgBoxResult = strToBool(params[1]) ? wingw::MB_ID_YES : wingw::MB_ID_NO;

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
    TEXTUREID tid;
    found++;
    switch (type)
    {
      case SHVT_INT: console->addMessage(ILogWriter::NOTE, "[int]   %-40s %d", name, ShaderGlobal::get_int_fast(varId)); break;
      case SHVT_REAL: console->addMessage(ILogWriter::NOTE, "[real]  %-40s %.3f", name, ShaderGlobal::get_real_fast(varId)); break;
      case SHVT_COLOR4:
        c = ShaderGlobal::get_color4_fast(varId);
        console->addMessage(ILogWriter::NOTE, "[color] %-40s %.3f,%.3f,%.3f,%.3f", name, c.r, c.g, c.b, c.a);
        break;
      case SHVT_TEXTURE:
        tid = ShaderGlobal::get_tex_fast(varId);
        console->addMessage(ILogWriter::NOTE, "[tex]   %-40s %s", name, tid == BAD_TEXTUREID ? "---" : get_managed_texture_name(tid));
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
      case SHVT_REAL: ShaderGlobal::set_real(varId, atof(params[1])); break;
      case SHVT_COLOR4:
        if (params.size() < 5)
        {
          console->addMessage(ILogWriter::ERROR, "shaders.set requires <name> <R> <G> <B> <A> for color var");
          return false;
        }
        ShaderGlobal::set_color4(varId, atof(params[1]), atof(params[2]), atof(params[3]), atof(params[4]));
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
      console->addMessage(ILogWriter::NOTE, "globalvar <%s> [real] set to %.3f", params[0], ShaderGlobal::get_real_fast(varId));
      break;
    case SHVT_COLOR4:
    {
      Color4 val = ShaderGlobal::get_color4_fast(varId);
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
  const Driver3dDesc &dsc = d3d::get_driver_desc();

  DataBlock appblk(DAGORED2->getWorkspace().getAppPath());
  String sh_file;
  if (appblk.getStr("shaders", NULL))
    sh_file.printf(260, "%s/%s", DAGORED2->getWorkspace().getAppDir(), appblk.getStr("shaders", NULL));
  else
    sh_file.printf(260, "%s/../commonData/compiledShaders/classic/tools", sgg::get_exe_path_full());
  simplify_fname(sh_file);

  const char *shname = params.size() > 0 ? params[0] : sh_file;

  String fileName;
  for (auto version : d3d::smAll)
  {
    if (dsc.shaderModel < version)
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
  console->addMessage(ILogWriter::FATAL, "failed to reload %s, dsc.shaderModel=%s", shname, d3d::as_string(dsc.shaderModel));
  return false;
}
