// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetHlp.h>
#include <assets/asset.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_string.h>

static String basedir, tmp;

//! init asset local props management for specified application folders
void assetlocalprops::init(const char *app_dir, const char *local_dir)
{
  if (!app_dir || !app_dir[0])
    app_dir = "./";
  if (!local_dir || !local_dir[0])
    local_dir = ".";
  tmp.printf(260, "%s/%s", app_dir, local_dir);
  dd_simplify_fname_c(tmp);

  if (!tmp[0])
    tmp = ".";
  else
    dd_mkdir(tmp);
  basedir.printf(260, "%s/", tmp.str());
}

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
