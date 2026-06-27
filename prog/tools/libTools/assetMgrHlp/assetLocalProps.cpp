// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetHlp.h>
#include <assets/asset.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_string.h>
#include <libTools/util/appDirRelativePath.h>

static String basedir, tmp;

//! init asset local props management for specified application folders
void assetlocalprops::init(const char *local_dir) { make_eff_app_relative_path(basedir, local_dir, true); }

bool assetlocalprops::mkDir(const char *rel_path) { return dd_mkdir(makePath(rel_path)); }

const char *assetlocalprops::makePath(const char *rel_path)
{
  tmp.printf(260, "%s%s", basedir.str(), rel_path);
  return tmp;
}

bool assetlocalprops::load(const DagorAsset &a, DataBlock &dest)
{
  tmp.printf(260, "%s%s~%s.blk", basedir.str(), a.getName(), a.getTypeStr());
  if (dest.load(tmp))
    return true;
  if (!dd_file_exist(tmp))
  {
    dest.clearData();
    return true;
  }
  return false;
}

bool assetlocalprops::save(const DagorAsset &a, const DataBlock &src)
{
  tmp.printf(260, "%s%s~%s.blk", basedir.str(), a.getName(), a.getTypeStr());
  return src.saveToTextFile(tmp);
}

bool assetlocalprops::loadBlk(const char *rel_path, DataBlock &dest)
{
  tmp.printf(260, "%s%s", basedir.str(), rel_path);
  if (dest.load(tmp))
    return true;
  if (!dd_file_exist(tmp))
  {
    dest.clearData();
    return true;
  }
  return false;
}

bool assetlocalprops::saveBlk(const char *rel_path, const DataBlock &src)
{
  tmp.printf(260, "%s%s", basedir.str(), rel_path);
  dd_mkpath(tmp);
  return src.saveToTextFile(tmp);
}
