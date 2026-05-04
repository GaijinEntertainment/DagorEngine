// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_consoleCommandProcessor.h"

#include "av_appwnd.h"

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <EditorCore/ec_wndGlobal.h>
#include <math/dag_mathUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <render/debugTexOverlay.h>
#include <de3_dynRenderService.h>
#include <de3_interface.h>

InitOnDemand<DebugTexOverlay> av_show_tex_helper;

static Point3 to_point3(dag::ConstSpan<const char *> params, int idx = 0)
{
  if ((idx + 3) <= params.size())
    return Point3(console::to_real(params[idx + 0]), console::to_real(params[idx + 1]), console::to_real(params[idx + 2]));

  return Point3(0, 0, 0);
}

static inline float deg_to_fov(float deg) { return 1.f / tanf(DEG_TO_RAD * 0.5f * deg); }

static inline float fov_to_deg(float fov) { return RAD_TO_DEG * 2.f * atan(1.f / fov); }

static int sort_texid_by_refc(const TEXTUREID *a, const TEXTUREID *b)
{
  if (int rc_diff = get_managed_texture_refcount(*a) - get_managed_texture_refcount(*b))
    return -rc_diff;
  return strcmp(get_managed_texture_name(*a), get_managed_texture_name(*b));
}

void AssetViewerConsoleCommandProcessor::runCameraAt(dag::ConstSpan<const char *> params)
{
  IGenViewportWnd *vpw = get_app().getCurrentViewport();
  if (!vpw)
    return;

  if (params.size() == 3 || params.size() == 6 || params.size() == 7)
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
  }
  else
  {
    TMatrix tm;
    vpw->getCameraTransform(tm);

    const float fovRad = vpw->getFov();
    const float fovDeg = fovRad > 0.0f ? fov_to_deg(fovRad) : -1.0f;

    get_app().getConsole().addMessage(ILogWriter::NOTE, "\tcamera.at %g %g %g %g %g %g %g", P3D(tm.getcol(3)), P3D(tm.getcol(2)),
      fovDeg);
  }
}

void AssetViewerConsoleCommandProcessor::runCameraDir(dag::ConstSpan<const char *> params)
{
  IGenViewportWnd *vpw = get_app().getCurrentViewport();
  if (!vpw)
    return;

  if (params.size() >= 3)
  {
    Point3 p3 = to_point3(params);
    Point3 up = params.size() >= 6 ? to_point3(params, 3) : Point3(0, 1, 0);
    vpw->setCameraDirection(::normalize(p3), ::normalize(up));
  }
}

void AssetViewerConsoleCommandProcessor::runCameraPos(dag::ConstSpan<const char *> params)
{
  IGenViewportWnd *vpw = get_app().getCurrentViewport();
  if (!vpw)
    return;

  if (params.size() == 3)
  {
    vpw->setCameraPos(to_point3(params));
  }
  else
  {
    TMatrix tm;
    vpw->getCameraTransform(tm);

    get_app().getConsole().addMessage(ILogWriter::NOTE, "Current camera position: %g, %g, %g", P3D(tm.getcol(3)));
  }
}

void AssetViewerConsoleCommandProcessor::runPerfDump()
{
  get_app().queryEditorInterface<IRenderHelperService>()->profilerDumpFrame();
  get_app().repaint();
}

void AssetViewerConsoleCommandProcessor::runShadersListVars(dag::ConstSpan<const char *> params)
{
  CoolConsole *console = &get_app().getConsole();

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
    switch (type)
    {
      case SHVT_INT: console->addMessage(ILogWriter::NOTE, "[int]   %-40s %d", name, ShaderGlobal::get_int_fast(varId)); break;
      case SHVT_REAL: console->addMessage(ILogWriter::NOTE, "[real]  %-40s %.3f", name, ShaderGlobal::get_float(varId)); break;
      case SHVT_COLOR4:
        c = ShaderGlobal::get_float4(varId);
        console->addMessage(ILogWriter::NOTE, "[color] %-40s %.3f,%.3f,%.3f,%.3f", name, c.r, c.g, c.b, c.a);
        break;
      case SHVT_TEXTURE:
      {
        TEXTUREID tid = ShaderGlobal::get_tex_fast(varId);
        BaseTexture *tptr = ShaderGlobal::get_tex_ptr_fast(varId);
        if (tid == BAD_TEXTUREID && tptr)
          console->addMessage(ILogWriter::NOTE, "[tex]   %-40s %p", name, tptr);
        else
          console->addMessage(ILogWriter::NOTE, "[tex]   %-40s %s", name,
            tid == BAD_TEXTUREID ? "---" : get_managed_texture_name(tid));
      }
      break;
    }
  }
}

void AssetViewerConsoleCommandProcessor::runShadersSetVar(dag::ConstSpan<const char *> params)
{
  CoolConsole *console = &get_app().getConsole();

  if (params.size() < 1)
  {
    console->addMessage(ILogWriter::ERROR, "shaders.set requires <name> and <value>");
    return;
  }
  int varId = VariableMap::getVariableId(params[0]);
  int type = ShaderGlobal::get_var_type(varId);
  if (type < 0)
  {
    console->addMessage(ILogWriter::ERROR, "<%s> is not global shader var", params[0]);
    return;
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
          return;
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
}

void AssetViewerConsoleCommandProcessor::runTexInfo(dag::ConstSpan<const char *> params)
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

void AssetViewerConsoleCommandProcessor::runTexRefs()
{
  Tab<TEXTUREID> ids;
  ids.reserve(32 << 10);
  for (TEXTUREID i = first_managed_texture(1); i != BAD_TEXTUREID; i = next_managed_texture(i, 1))
    ids.push_back(i);
  sort(ids, &sort_texid_by_refc);

  for (TEXTUREID texId : ids)
    DAEDITOR3.conNote("  [%5d] refc=%-3d %s", texId, get_managed_texture_refcount(texId), get_managed_texture_name(texId));
  DAEDITOR3.conNote("total %d referenced textures", ids.size());
}

void AssetViewerConsoleCommandProcessor::runTexShow(const char *argv[], int argc)
{
  av_show_tex_helper.demandInit();

  String str = av_show_tex_helper->processConsoleCmd(argv, argc);
  if (!str.empty())
    DAEDITOR3.conNote(str);
}

void AssetViewerConsoleCommandProcessor::runTexHide()
{
  if (av_show_tex_helper.get())
    av_show_tex_helper->hideTex();
}

void AssetViewerConsoleCommandProcessor::runShadersReload(dag::ConstSpan<const char *> params)
{
  CoolConsole *console = &get_app().getConsole();

  G_ASSERT(d3d::is_inited());
  const DriverDesc &dsc = d3d::get_driver_desc();

  DataBlock appblk(::get_app().getWorkspace().getAppBlkPath());
  String sh_file;
  if (appblk.getStr("shaders", NULL))
    sh_file.printf(260, "%s/%s", ::get_app().getWorkspace().getAppDir(), appblk.getStr("shaders", NULL));
  else
    sh_file.printf(260, "%s/compiledShaders/classic/tools", sgg::get_common_data_dir());
  simplify_fname(sh_file);

  const char *shname = params.size() > 0 ? params[0] : sh_file.c_str();

  String fileName;
  for (auto version : d3d::smAll)
  {
    if (dsc.shaderModel < version)
      continue;

    fileName.printf(260, "%s.%s.shdump.bin", shname, version.psName);

    if (!dd_file_exist(fileName))
      continue;

    if (load_shaders_bindump(shname, version))
    {
      console->addMessage(ILogWriter::NOTE, "reloaded %s, ver=%s", shname, version.psName);
      return;
    }
    console->addMessage(ILogWriter::FATAL, "failed to reload %s, ver=%s", shname, version.psName);
    return;
  }
  console->addMessage(ILogWriter::FATAL, "failed to reload %s, dsc.shaderModel=%s", shname, d3d::as_string(dsc.shaderModel));
}

void AssetViewerConsoleCommandProcessor::runSelectAsset(dag::ConstSpan<const char *> params)
{
  const char *name = params[0];
  CoolConsole *console = &get_app().getConsole();
  DagorAsset *asset = get_app().getAssetMgr().findAsset(name);
  if (asset)
  {
    get_app().selectAsset(*asset);
    console->addMessage(ILogWriter::NOTE, "Selected asset: \"%s\"", name);
  }
  else
    console->addMessage(ILogWriter::ERROR, "Asset with name \"%s\" not found!", name);
}

bool AssetViewerConsoleCommandProcessor::processCommand(const char *argv[], int argc)
{
  if (argc < 1)
    return false;

  const dag::Span<const char *> params = make_span(&argv[1], argc - 1);
  int found = 0;

  CONSOLE_CHECK_NAME("camera", "at", 1, 8) { runCameraAt(params); }
  CONSOLE_CHECK_NAME("camera", "dir", 1, 7) { runCameraDir(params); }
  CONSOLE_CHECK_NAME("camera", "pos", 1, 4) { runCameraPos(params); }

  CONSOLE_CHECK_NAME("driver", "reset", 1, 1) { dagor_d3d_force_driver_reset = true; }

  CONSOLE_CHECK_NAME("perf", "dump", 1, 1) { runPerfDump(); }
  CONSOLE_CHECK_NAME("perf", "off", 1, 1) { get_app().queryEditorInterface<IRenderHelperService>()->enableFrameProfiler(false); }
  CONSOLE_CHECK_NAME("perf", "on", 1, 1) { get_app().queryEditorInterface<IRenderHelperService>()->enableFrameProfiler(true); }

  // This is the same as the "tex.show" command but the "Console commands and variables" debug dialog executes this command.
  CONSOLE_CHECK_NAME("render", "show_tex", 1, DebugTexOverlay::MAX_CONSOLE_ARGS_CNT) { runTexShow(argv, argc); }

  CONSOLE_CHECK_NAME_EX("shaders", "list", 1, 2, "", "[filter-string]") { runShadersListVars(params); }
  CONSOLE_CHECK_NAME_EX("shaders", "reload", 1, 2, "", "[shaders-binary-dump-filename]") { runShadersReload(params); }
  CONSOLE_CHECK_NAME_EX("shaders", "set", 1, 6, "", "<shader-variable-name> <value>") { runShadersSetVar(params); }

  CONSOLE_CHECK_NAME("tex", "hide", 1, 1) { runTexHide(); }
  CONSOLE_CHECK_NAME("tex", "info", 1, 2) { runTexInfo(params); }
  CONSOLE_CHECK_NAME("tex", "refs", 1, 1) { runTexRefs(); }
  CONSOLE_CHECK_NAME("tex", "show", 1, DebugTexOverlay::MAX_CONSOLE_ARGS_CNT) { runTexShow(argv, argc); }

  CONSOLE_CHECK_NAME_EX("app", "select_asset", 2, 2, "", "[asset-name]") { runSelectAsset(params); }

  return found != 0;
}
