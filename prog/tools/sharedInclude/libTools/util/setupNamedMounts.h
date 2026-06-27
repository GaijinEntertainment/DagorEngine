//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h> //==
#if _TARGET_PC_WIN
#include <direct.h>
#else
#include <unistd.h>
#endif

// build and simplify absolute path to app_dir and set it as %appDir
static inline void set_canonical_app_dir_mount(const char *app_dir)
{
  String path;
  if (strchr("\\/%", app_dir[0]) || strstr(app_dir, ":/") || strstr(app_dir, ":\\") || strncmp(app_dir, "\\\\", 2) == 0)
    path = app_dir;
  else
  {
    char curdir[512] = ".";
    path.setStrCat3(::getcwd(curdir, 512), "/", app_dir);
  }
  simplify_fname(path);
  debug("set_canonical_app_dir_mount(%s) -> %s", app_dir, path); //==
  dd_set_named_mount_path("%appDir", path);
}
static inline void set_canonical_app_dir_mount_from_fpath(const char *app_fpath)
{
  char app_dir[DAGOR_MAX_PATH];
  dd_get_fname_location(app_dir, app_fpath);
  set_canonical_app_dir_mount(app_dir);
}

// setup named mounts from blocks: "%lang"{ forSource:t="%appDir/prog/gameBase/lang_src/lang"; forVromfs:t="lang"; }
static inline void setup_named_mount_points(const DataBlock &mnt, const char *sdk_dir = nullptr)
{
  if (!mnt.blockCount())
    return;

  const bool useAddonVromSrc = true;
  char curdir[512];
  if (!sdk_dir)
    sdk_dir = ::getcwd(curdir, 512);

  dblk::iterate_child_blocks(mnt, [p = useAddonVromSrc ? "forSource" : "forVromfs", dir = sdk_dir](const DataBlock &b) {
    const char *path = b.getStr(p);
    String mntPath;
    if (dd_resolve_named_mount(mntPath, path))
    {
      simplify_fname(mntPath);
      dd_set_named_mount_path(b.getBlockName(), mntPath);
    }
    else if (strchr("\\/%", path[0]) || strstr(path, ":/") || strstr(path, ":\\") || strncmp(path, "\\\\", 2) == 0)
      dd_set_named_mount_path(b.getBlockName(), path);
    else
    {
      mntPath.setStrCat3(dir, "/", path);
      simplify_fname(mntPath);
      dd_set_named_mount_path(b.getBlockName(), mntPath);
    }
  });
  dd_dump_named_mounts();
}
