// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>

class DataBlock;
class IGenSave;
class IGenLoad;

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

enum class PatchGameResourcesStrategy
{
  Full, //!< Expecting that upcoming update will download all of the files from package. Allow to create non-existing files accouring
        //!< to grp_hdr.vromfs.bin (Default strategy).

  PatchOnlyExisting, //!< Patch only existing files on the disk. Deny creating new files. Suitable in case when the package has been
                     //!< downladed partially (per file) and upcoming update won't add new files and only update existing ones.
};

PatchGameResourcesStrategy set_patch_update_game_resources_strategy(PatchGameResourcesStrategy strategy);

int patch_update_vromfs_pack(const char *old_vromfs_pack, const DataBlock &new_vromfs_index);

bool write_index_for_sound_bank_to_stream(const char *bank_path, IGenSave &index_cwr);

bool patch_update_sound_bank(const char *index_path, const char *sound_dir);
bool patch_update_sound_bank_mem(IGenLoad &index_crd, const char *sound_dir);

//! global settings for path verbosity level (default=0)
extern int patch_update_game_resources_verbose;
