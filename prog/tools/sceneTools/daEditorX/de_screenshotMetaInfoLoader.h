// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <util/dag_string.h>

class DataBlock;
class FilePathName;
class IGenViewportWnd;

class ScreenshotMetaInfoLoader
{
public:
  static bool loadMetaInfo(const char *screenshot_path, DataBlock &meta_info, String &error_message);
  static bool getProjectPath(const DataBlock &meta_info, const char *app_dir, const char *levels_dir, String &project_path,
    String &error_message);
  static void applyCameraSettings(const DataBlock &meta_info, IGenViewportWnd &viewport);

private:
  static bool getExportPathFromProjectFile(const char *project_file_path, String &export_path);
  static bool getProjectFilePathFromDirectory(const char *directory_path, FilePathName &project_file_path);
  static bool findProjectByExportPath(const char *directory_path, const char *searched_export_path, FilePathName &project_file_path);

  static String expand_path(const char *p);
  static void getDaNetGameBasePaths(const char *settings_file_path, dag::Vector<String> &basePaths);
  static void processDaNetGameSceneBlkImports(DataBlock &scene_blk);
  static bool getLevelBlkFromDaNetGameScene(const char *scene_file_path, const DataBlock &scene_blk, String &level_blk_path);
  static bool getProjectPathForDaNetGameInternal(const char *levels_dir, const DataBlock &meta_info, String &project_path,
    String &error_message);
  static bool getProjectPathForDaNetGame(const DataBlock &meta_info, const char *app_dir, const char *levels_dir, String &project_path,
    String &error_message);

  static bool getProjectPathForWarThunder(const DataBlock &meta_info, const char *levels_dir, String &project_path,
    String &error_message);
};
