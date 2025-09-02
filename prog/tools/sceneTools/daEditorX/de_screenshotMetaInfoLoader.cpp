// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_screenshotMetaInfoLoader.h"
#include <EditorCore/ec_interface.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <image/dag_jpeg.h>
#include <image/dag_texPixel.h>
#include <libTools/util/filePathname.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>

bool ScreenshotMetaInfoLoader::loadMetaInfo(const char *screenshot_path, DataBlock &meta_info, String &error_message)
{
  FullFileLoadCB crd(screenshot_path);
  if (!crd.fileHandle)
  {
    error_message = "Cannot read screenshot.";
    return false;
  }

  eastl::string comment;
  TexImage32 *img = load_jpeg32(crd, stdmem_ptr(), &comment);
  if (!img)
  {
    error_message = "Cannot parse screenshot.";
    return false;
  }
  delete img;

  if (comment.empty())
  {
    error_message = "No meta info in screenshot.";
    return false;
  }

  if (!meta_info.loadText(comment.c_str(), comment.size(), screenshot_path))
  {
    error_message = "Cannot load meta info from screenshot.";
    return false;
  }

  return true;
}

bool ScreenshotMetaInfoLoader::getExportPathFromProjectFile(const char *project_file_path, String &export_path)
{
  DataBlock projectBlk;
  if (!dblk::load(projectBlk, project_file_path, dblk::ReadFlag::ROBUST))
    return false;

  const char *exportPath = projectBlk.getStr("exportPath");
  if (!exportPath)
    return false;

  export_path = exportPath;
  return true;
}

bool ScreenshotMetaInfoLoader::getProjectFilePathFromDirectory(const char *directory_path, FilePathName &project_file_path)
{
  for (const alefind_t &ff : dd_find_iterator(FilePathName(directory_path, "*.level.blk"), DA_FILE))
  {
    // Ignore previous file versions made by CVS (".#file.revision").
    if (ff.name[0] == '.' && ff.name[1] == '#')
      continue;

    project_file_path = FilePathName(directory_path, ff.name);
    return true;
  }
  return false;
}

bool ScreenshotMetaInfoLoader::findProjectByExportPath(const char *directory_path, const char *searched_export_path,
  FilePathName &project_file_path)
{
  FilePathName currentProjectFilePath;
  if (getProjectFilePathFromDirectory(directory_path, currentProjectFilePath))
  {
    String exportPath;
    if (getExportPathFromProjectFile(currentProjectFilePath, exportPath) && stricmp(exportPath, searched_export_path) == 0)
    {
      project_file_path = currentProjectFilePath;
      return true;
    }

    return false;
  }

  for (const alefind_t &ff : dd_find_iterator(FilePathName(directory_path, "*"), DA_SUBDIR))
  {
    if ((ff.attr & DA_SUBDIR) == 0 || stricmp(ff.name, "cvs") == 0)
      continue;

    FilePathName childPath(directory_path, ff.name);
    if (findProjectByExportPath(childPath, searched_export_path, project_file_path))
    {
      return true;
    }
  }

  return false;
}

String ScreenshotMetaInfoLoader::expand_path(const char *p)
{
  String path(0, "%s/", p);
  dd_simplify_fname_c(path);
  path.updateSz();
  return path;
}

// See load_game_package in prog/daNetGame/main/gameLoad.cpp.
void ScreenshotMetaInfoLoader::getDaNetGameBasePaths(const char *settings_file_path, dag::Vector<String> &basePaths)
{
  DataBlock gameSettings;
  gameSettings.load(settings_file_path);

  const DataBlock *addons = gameSettings.getBlockByName("addonBasePath");
  if (!addons)
    return;

  const int addonNid = addons->getNameId("addon");
  for (int blockIndex = 0; blockIndex < addons->blockCount(); ++blockIndex)
  {
    const DataBlock *addonBlk = addons->getBlock(blockIndex);
    if (addonBlk->getBlockNameId() != addonNid)
      continue;

    const char *folder = addonBlk->getStr("folder");
    if (folder)
    {
      const String path = expand_path(folder);
      if (eastl::find(basePaths.begin(), basePaths.end(), path) == basePaths.end())
        basePaths.push_back(path);
    }

    const int srcNid = addonBlk->getNameId("src");
    const int paramCount = addonBlk->paramCount();
    for (int paramIndex = 0; paramIndex < paramCount; ++paramIndex)
    {
      if (addonBlk->getParamNameId(paramIndex) == srcNid || strncmp(addonBlk->getParamName(paramIndex), "src", 3) == 0)
      {
        const String path = expand_path(expand_path(addonBlk->getStr(paramIndex)));
        if (eastl::find(basePaths.begin(), basePaths.end(), path) == basePaths.end())
          basePaths.push_back(path);
      }
    }
  }
}

// This is a very-very basic import resolver. It is used in place of
// load_templates_blk_file because the ECS is not yet initialized when
// the level selector (StartupDlg) is shown.
void ScreenshotMetaInfoLoader::processDaNetGameSceneBlkImports(DataBlock &scene_blk)
{
  const int importNid = scene_blk.getNameId("import");
  const int sceneBlkBlockCount = scene_blk.blockCount();
  for (int blockIndex = 0; blockIndex < sceneBlkBlockCount; ++blockIndex)
  {
    DataBlock *importStatementBlk = scene_blk.getBlock(blockIndex);
    if (importStatementBlk->getBlockNameId() != importNid)
      continue;

    const char *importScenePath = importStatementBlk->getStr("scene");
    if (!importScenePath)
      continue;

    DataBlock importFileBlk;
    if (!dblk::load(importFileBlk, importScenePath, dblk::ReadFlag::ROBUST))
      continue;

    // These are added to the end, they will not be processed by this loop.
    // We only resolve one level of imports.
    const int importFileBlkBlockCount = importFileBlk.blockCount();
    for (int importFileBlkBlockIndex = 0; importFileBlkBlockIndex < importFileBlkBlockCount; ++importFileBlkBlockIndex)
      scene_blk.addNewBlock(importFileBlk.getBlock(importFileBlkBlockIndex));
  }
}

bool ScreenshotMetaInfoLoader::getLevelBlkFromDaNetGameScene(const char *scene_file_path, const DataBlock &scene_blk,
  String &level_blk_path)
{
  const int entityNid = scene_blk.getNameId("entity");
  for (int blockIndex = 0; blockIndex < scene_blk.blockCount(); ++blockIndex)
  {
    const DataBlock *entityBlk = scene_blk.getBlock(blockIndex);
    if (entityBlk->getBlockNameId() != entityNid)
      continue;

    const char *levelBlk = entityBlk->getStr("level__blk");
    if (levelBlk)
    {
      level_blk_path = levelBlk;
      return !level_blk_path.empty();
    }
  }

  return false;
}

bool ScreenshotMetaInfoLoader::getProjectPathForDaNetGameInternal(const char *levels_dir, const DataBlock &meta_info,
  String &project_path, String &error_message)
{
  const DataBlock *settingsBlk = meta_info.getBlockByName("settings");
  const char *gameName = settingsBlk ? settingsBlk->getStr("contactsGameId") : nullptr;
  if (!gameName)
  {
    error_message = "\"contactsGameId\" is missing from the meta info.";
    return false;
  }

  const char *relativeScenePath = meta_info.getStr("scene");
  if (!relativeScenePath)
  {
    error_message = "\"scene\" path is missing from the meta info.";
    return false;
  }

  const String settingsFilePath(0, "content/%s/gamedata/%s.settings.blk", gameName, gameName);
  dag::Vector<String> basePaths;
  getDaNetGameBasePaths(settingsFilePath, basePaths);

  String levelBlkPath;
  for (const String &basePath : basePaths)
    dd_add_base_path(basePath, false);

  DataBlock sceneBlk;
  if (!dblk::load(sceneBlk, relativeScenePath, dblk::ReadFlag::ROBUST))
  {
    error_message.printf(0, "Failed to load the scene file \"%s\".", relativeScenePath);
    return false;
  }

  processDaNetGameSceneBlkImports(sceneBlk);
  if (!getLevelBlkFromDaNetGameScene(relativeScenePath, sceneBlk, levelBlkPath))
  {
    error_message.printf(0, "\"level__blk\" is missing from the scene file \"%s\".", levelBlkPath.c_str());
    return false;
  }

  DataBlock levelBlk;
  if (!dblk::load(levelBlk, levelBlkPath, dblk::ReadFlag::ROBUST))
  {
    error_message.printf(0, "Failed to load level blk file \"%s\".", levelBlkPath.c_str());
    return false;
  }

  const char *levelBinPath = levelBlk.getStr("levelBin");
  if (!levelBinPath)
  {
    error_message.printf(0, "\"levelBin\" path is missing from the level blk file \"%s\".", levelBlkPath.c_str());
    return false;
  }

  FilePathName foundProjectPath;
  if (!findProjectByExportPath(levels_dir, levelBinPath, foundProjectPath))
  {
    error_message.printf(0, "Could not find project file where \"exportPath\" is \"%s\".", levelBinPath);
    return false;
  }

  project_path = foundProjectPath;
  return true;
}

bool ScreenshotMetaInfoLoader::getProjectPathForDaNetGame(const DataBlock &meta_info, const char *app_dir, const char *levels_dir,
  String &project_path, String &error_message)
{
  struct BasePathRestorer
  {
    BasePathRestorer()
    {
      int index = 0;
      while (const char *basePath = df_next_base_path(&index))
        basePaths.emplace_back(String(basePath));
    }

    ~BasePathRestorer()
    {
      dd_clear_base_paths();
      for (const String &basePath : basePaths)
        dd_add_base_path(basePath);
    }

    dag::Vector<String> basePaths;
  };

  BasePathRestorer basePathRestorer;

  // These are needed for the scene file loading.
  DataBlock::setRootIncludeResolver(nullptr);   // This is set in DagorWinMain in prog/daNetGame/main/main.cpp in the game.
  dd_add_base_path("../prog/gameBase/", false); // See get_allowed_addon_src_files_prefix in prog/daNetGame/main/settings.cpp.

  const bool result = getProjectPathForDaNetGameInternal(levels_dir, meta_info, project_path, error_message);

  DataBlock::setRootIncludeResolver(app_dir);

  return result;
}

bool ScreenshotMetaInfoLoader::getProjectPathForWarThunder(const DataBlock &meta_info, const char *levels_dir, String &project_path,
  String &error_message)
{
  const DataBlock *locationBlk = meta_info.getBlockByName("location");
  if (!locationBlk)
  {
    error_message = "\"location\" block is missing from the meta info.";
    return false;
  }

  const char *levelBinPath = locationBlk->getStr("scene");
  if (!levelBinPath)
  {
    error_message = "\"scene\" path is missing from the meta info.";
    return false;
  }

  FilePathName foundProjectPath;
  if (!findProjectByExportPath(levels_dir, levelBinPath, foundProjectPath))
  {
    error_message.printf(0, "Could not find project file where \"exportPath\" is \"%s\".", levelBinPath);
    return false;
  }

  project_path = foundProjectPath;
  return true;
}

bool ScreenshotMetaInfoLoader::getProjectPath(const DataBlock &meta_info, const char *app_dir, const char *levels_dir,
  String &project_path, String &error_message)
{
  if (meta_info.getStr("game"))
    return getProjectPathForWarThunder(meta_info, levels_dir, project_path, error_message);
  else
    return getProjectPathForDaNetGame(meta_info, app_dir, levels_dir, project_path, error_message);
}

void ScreenshotMetaInfoLoader::applyCameraSettings(const DataBlock &meta_info, IGenViewportWnd &viewport)
{
  const DataBlock *cameraBlk = meta_info.getBlockByName("camera");
  if (!cameraBlk)
    return;

  const int paramIndex = cameraBlk->findParam("itm");
  if (paramIndex >= 0 && cameraBlk->getParamType(paramIndex) == DataBlock::TYPE_MATRIX)
  {
    const TMatrix tm = cameraBlk->getTm(paramIndex);
    viewport.setCameraTransform(tm);
  }
}
