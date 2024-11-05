// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>

class IGameResPatchProgressBar
{
public:
  virtual void setStage(int stage) = 0;
  virtual void setPromilleDone(int promille) = 0;
  virtual bool isCancelled() = 0;

  virtual bool ignoreResFile(const char *path)
  {
    G_UNUSED(path);
    return false;
  }
};

//! returns total data reusage in percents, or -1 on error
int patch_update_game_resources(const char *root_dir, const char *vromfs_fn, const char *rel_mount_dir,
  IGameResPatchProgressBar *pbar = NULL, bool dryRun = false, const char *vrom_name = "grp_hdr.vromfs.bin",
  const char *res_blk_name = "resPacks.blk");

int patch_update_game_resources_mem(const char *game_dir, const char *cache_dir, const void *vromfs_data, int vromfs_data_sz,
  const char *rel_mount_dir, IGameResPatchProgressBar *pbar = NULL, bool dryRun = false, const char *vrom_name = "grp_hdr.vromfs.bin",
  const char *res_blk_name = "resPacks.blk");

//! global settings for path verbosity level (default=0)
extern int patch_update_game_resources_verbose;
