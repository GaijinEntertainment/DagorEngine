// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_workspace.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_wndGlobal.h>

#include <generic/dag_sort.h>
#include <libTools/util/fileUtils.h>
#include <libTools/util/strUtil.h>
#include <libTools/containers/tab_sort.h>
#include <coolConsole/coolConsole.h>

#include <osApiWrappers/dag_direct.h>

#include <util/dag_globDef.h>
#include <debug/dag_debug.h>

#include <winGuiWrapper/wgw_dialogs.h>

//==================================================================================================
EditorWorkspace::EditorWorkspace() : deDisabled(midmem), reDisabled(midmem), wspData(NULL), platforms(midmem), maxTraceDistance(1000.0)
{}


//==================================================================================================
EditorWorkspace::~EditorWorkspace() { del_it(wspData); }


//==================================================================================================
bool EditorWorkspace::initWorkspaceBlk(const char *path)
{
  if (!path || !*path)
    return false;

  del_it(wspData);

  wspData = new (tmpmem) Workspaces;
  G_ASSERT(wspData);

  if (!dd_file_exist(path))
  {
    wspData->blk.saveToTextFile(path);
  }

  if (!wspData->blk.load(path))
  {
    del_it(wspData);
    return false;
  }

  blkPath = path;

  const int wspNid = wspData->blk.getNameId("workspace");
  bool needs_save = false;

  for (int i = 0; i < wspData->blk.blockCount(); ++i)
  {
    DataBlock *wspBlk = wspData->blk.getBlock(i);

    if (wspBlk && wspBlk->getBlockNameId() == wspNid)
    {
      String wspName(wspBlk->getStr("name", ""));
      if (wspName.length())
        if (!wspData->names.add(wspName, wspBlk))
        {
          IEditorCoreEngine::get()->getConsole().addMessage(ILogWriter::WARNING, "removing duplicate workspace %s", wspName.str());
          wspData->blk.removeBlock(i);
          needs_save = true;
          i--;
        }
    }
  }

  if (needs_save)
    wspData->blk.saveToTextFile(path);
  return true;
}


//==================================================================================================
void EditorWorkspace::freeWorkspaceBlk() { del_it(wspData); }


//==================================================================================================
void EditorWorkspace::getWspNames(Tab<String> &list) const
{
  G_ASSERT(wspData);

  list.resize(wspData->names.size());

  String key;
  DataBlock *val;
  int i = 0;

  for (bool ok = wspData->names.getFirst(key, val); ok; ok = wspData->names.getNext(key, val))
    list[i++] = key;

  sort(list, &tab_sort_stringsi);
}


//==================================================================================================
bool EditorWorkspace::load(const char *workspace_name, bool *app_path_set)
{
  if (!workspace_name || !*workspace_name)
    return false;

  G_ASSERT(wspData);

  if (app_path_set)
    *app_path_set = false;

  DataBlock *blk;
  if (!wspData->names.get(String(workspace_name), blk))
  {
    ::debug("Unknown workspace \"%s\"", workspace_name);
    wingw::message_box(0, "Workspace loading error", "Workspace named \"%s\" not found", workspace_name);

    return false;
  }

  G_ASSERT(blk);

  name = workspace_name;

  return loadFromBlk(*blk, app_path_set);
}


//==================================================================================================
bool EditorWorkspace::loadIndirect(const char *app_blk_path)
{
  DataBlock appBlk;

  if (appBlk.load(app_blk_path))
  {
    String applicationDir(app_blk_path);
    ::location_from_path(applicationDir);

    applicationDir = make_path_absolute(applicationDir);
    G_ASSERT(!applicationDir.empty());
    append_slash(applicationDir);

    appBlk.setStr("application_path", app_blk_path);
    appBlk.setStr("application_dir", applicationDir);

    return loadFromBlk(appBlk);
  }

  return false;
}


//==================================================================================================
bool EditorWorkspace::loadFromBlk(DataBlock &blk, bool *app_path_set)
{
  platforms.clear();
  deDisabled.clear();
  reDisabled.clear();

  appDir = blk.getStr("application_dir", "");
  appPath = blk.getStr("application_path", "");
  if (appDir.length() && !appPath.length())
    appPath = ::make_full_path(appDir, "application.blk");

  if (!appPath.length() || !::dd_file_exist(appPath))
  {
    ::debug("Application settings not exist or not specified for workspace \"%s\"", (const char *)name);
  }

  String appBlkPath = appPath;

  if (!::dd_file_exist(appBlkPath))
  {
    ::debug("Couldn't open file \"%s\"", (const char *)appBlkPath);
    return false;
  }

  DataBlock appBlk;

  if (!appBlk.load(appBlkPath))
  {
    ::debug("Errors while loading \"%s\"", (const char *)appBlkPath);
    wingw::message_box(0, "Workspace initialization failed", "Errors while loading \"%s\". Workspace initialization failed",
      (const char *)appBlkPath);

    return false;
  }

  const DataBlock *sdkBlk = appBlk.getBlockByName("SDK");

  if (!sdkBlk)
  {
    ::debug("\"SDK\" section not found in \"application.blk\" file");
    wingw::message_box(0, "Workspace initialization failed", "\"SDK\" section not found in \"application.blk\" file.");
    return false;
  }

  sdkDir = ::make_full_path(appDir, sdkBlk->getStr("sdk_folder", ""));
  libDir = ::make_full_path(appDir, sdkBlk->getStr("lib_folder", ""));
  levelsDir = ::make_full_path(appDir, sdkBlk->getStr("levels_folder", ""));
  resDir = ::make_full_path(appDir, sdkBlk->getStr("resource_database_folder", ""));
  scriptDir = ::make_full_path(appDir, sdkBlk->getStr("script_scheme_folder", ""));

  const char *asd = sdkBlk->getStr("asset_script_scheme_folder", NULL);
  assetScriptsDir = (asd) ? ::make_full_path(appDir, asd) : "";

  const DataBlock *gameBlk = appBlk.getBlockByName("game");

  if (!gameBlk)
  {
    ::debug("\"game\" section not found in \"application.blk\" file");
    wingw::message_box(0, "Workspace initialization failed", "\"game\" section not found in \"application.blk\" file.");

    return false;
  }

  gameDir = ::make_full_path(appDir, gameBlk->getStr("game_folder", ""));

  levelsBinDir = ::make_full_path(appDir, gameBlk->getStr("levels_bin_folder", ""));
  const char *tmp = gameBlk->getStr("physmat", "/game/config/physmat.blk");
  physmatPath = (tmp && tmp[0]) ? ::make_full_path(appDir, tmp) : "";
  scriptLibrary = ::make_full_path(appDir, gameBlk->getStr("script_objects", ""));

  const DataBlock *deDisabledBlk = appBlk.getBlockByName("dagored_disabled_plugins");

  if (deDisabledBlk)
  {
    for (int i = 0; i < deDisabledBlk->paramCount(); ++i)
    {
      const char *str = deDisabledBlk->getStr(i);
      if (str && *str)
        deDisabled.push_back() = str;
    }
  }

  const DataBlock *reDisabledBlk = appBlk.getBlockByName("reseditor_disabled_plugins");

  if (reDisabledBlk)
  {
    for (int i = 0; i < reDisabledBlk->paramCount(); ++i)
    {
      const char *str = reDisabledBlk->getStr(i);
      if (str && *str)
        reDisabled.push_back() = str;
    }
  }

  const DataBlock *platformsBlk = appBlk.getBlockByName("additional_platforms");

  if (platformsBlk)
    for (int i = 0; i < platformsBlk->paramCount(); ++i)
    {
      const char *platf = platformsBlk->getStr(i);
      if (platf)
      {
        unsigned plt = getPlatformFromStr(platf);
        if (plt)
          platforms.push_back(plt);
        else
          debug("Unknown platform \"%s\"", platf);
      }
    }

  const DataBlock *paramsBlk = appBlk.getBlockByName("parameters");
  if (paramsBlk)
    maxTraceDistance = paramsBlk->getReal("maxTraceDistance", 1000.0);

  collisionName = appBlk.getStr("collision", "Bullet");
  if (stricmp(collisionName, "unigine") == 0)
    collisionName = "Bullet";

  if (!loadAppSpecific(appBlk))
    return false;

  return loadSpecific(blk);
}
unsigned EditorWorkspace::getPlatformFromStr(const char *platf)
{
  unsigned plt = 0;
  if (!stricmp(platf, "PC"))
    plt = _MAKE4C('PC');
  else if (!stricmp(platf, "PS4"))
    plt = _MAKE4C('PS4');
  else if (!stricmp(platf, "iOS"))
    plt = _MAKE4C('iOS');
  else if (!stricmp(platf, "and"))
    plt = _MAKE4C('and');

  return plt;
}

const char *EditorWorkspace::getPlatformNameFromId(unsigned plt)
{
  if (plt == _MAKE4C('PC'))
    return "";
  else if (plt == _MAKE4C('PS4'))
    return "PS4";
  else if (plt == _MAKE4C('iOS'))
    return "iOS";
  else if (plt == _MAKE4C('and'))
    return "and";

  return NULL;
}

//==================================================================================================
bool EditorWorkspace::save()
{
  if (blkPath.empty())
    return false;
  DataBlock blk;

  if (!blk.load(blkPath))
    return false;

  if (name && *name)
  {
    DataBlock *wspSaveBlk = findWspBlk(blk, name, true);

    wspSaveBlk->setStr("name", name);
    wspSaveBlk->setStr("application_dir", appDir);
    wspSaveBlk->setStr("application_path", appPath);

    if (!saveSpecific(*wspSaveBlk))
      return false;
  }
  return blk.saveToTextFile(blkPath);
}


//==================================================================================================
bool EditorWorkspace::remove()
{
  DataBlock blk;

  if (!blk.load(blkPath))
    return false;

  DataBlock *remBlk = findWspBlk(blk, name, false);

  if (!remBlk)
    return true;

  for (int i = 0; i < blk.blockCount(); ++i)
    if (blk.getBlock(i) == remBlk)
    {
      blk.removeBlock(i);
      break;
    }

  return blk.saveToTextFile(blkPath);
}


//==================================================================================================
void EditorWorkspace::setAppPath(const char *new_path)
{
  if (::dd_file_exist(new_path))
  {
    appPath = new_path;
    appDir = new_path;
    ::location_from_path(appDir);
  }
}


//==================================================================================================
bool EditorWorkspace::createApplicationBlk(const char *path) const
{
  const String src = ::make_full_path(sgg::get_exe_path(), "../common/samples/application.blk");
  return ::dag_copy_file(src, path);
}


//==================================================================================================
DataBlock *EditorWorkspace::findWspBlk(DataBlock &blk, const char *wsp_name, bool create_new)
{
  const int wspNid = blk.getNameId("workspace");
  DataBlock *retBlk = NULL;

  for (int i = 0; i < blk.blockCount(); ++i)
  {
    DataBlock *wspBlk = blk.getBlock(i);

    if (wspBlk && wspBlk->getBlockNameId() == wspNid)
    {
      const char *wspName = wspBlk->getStr("name", NULL);
      if (wspName && *wspName && !::stricmp(wsp_name, wspName))
      {
        retBlk = wspBlk;
        break;
      }
    }
  }

  if (create_new && !retBlk)
  {
    retBlk = blk.addNewBlock("workspace");
    G_ASSERT(retBlk);
  }

  return retBlk;
}
