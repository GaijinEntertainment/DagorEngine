// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <osApiWrappers/dag_vromfs.h>
#include <contentUpdater/version.h>
#include <compressionUtils/vromfsCompressionImpl.h>
#include <EASTL/string.h>

class DataBlock;
class IMemAlloc;
class String;

struct RAIIVrom
{
  IMemAlloc *mem;
  VirtualRomFsData *vrom;
  RAIIVrom(const char *vname,
    bool ignore_missing = false,
    IMemAlloc *alloc = NULL,
    bool insert_first = false,
    char *mnt = NULL,
    bool req_sign = false);
  ~RAIIVrom();
  bool isLoaded() const { return vrom != NULL; }
};

enum class ReqVromSign
{
  No,
  Yes
};

struct VromLoadInfo // -V730
{
  const char *path;
  const char *mount;
  bool isPack, optional;
  ReqVromSign reqSign = ReqVromSign::Yes;
};

String get_eff_vrom_fpath(const char *name);
VirtualRomFsData *mount_vrom(const char *name, const char *mnt_path, bool is_pack, bool is_optional, ReqVromSign req_sign);
void apply_vrom_list_difference(const DataBlock &blk, dag::ConstSpan<VromLoadInfo> extra_list = {}); // unload extra, load new
void unmount_all_vroms();

const char *get_vrom_path_by_name(const char *name);
VromfsCompression create_vromfs_compression(const char *path);

void reset_vroms_changed_state();
void mark_vrom_as_changed_by_name(const char *name);
int remount_changed_vroms();
int remount_all_vroms();

bool register_data_as_single_file_in_virtual_vrom(dag::ConstSpan<char> data, const char *mount_dir, const char *rel_fname);
bool unregister_data_as_single_file_in_virtual_vrom(const char *mount_dir, const char *rel_fname);
void unregister_all_virtual_vrom();

// Returns effective game's version.
// The version is affected by the embedded updater.
// The version is updated on remount_all_vroms() call
// New vroms might be downloaded but not mounted yet
// So, game's version can't be updated until vroms are remounted
updater::Version get_updated_game_version();

// Returns game's version in main game folder.
// The version is downloaded by the launcher.
updater::Version get_game_build_version();

bool check_vromfs_version_newer(updater::Version base_ver, updater::Version upd_ver);

bool init_expected_vromfs_version();
void install_vromfs_signature_check(signature_checker_factory_cb sig);

namespace updater
{
namespace binarycache
{
const eastl::string &get_cache_folder();
const eastl::string &get_remote_folder();

Version get_cache_version();
} // namespace binarycache
} // namespace updater
