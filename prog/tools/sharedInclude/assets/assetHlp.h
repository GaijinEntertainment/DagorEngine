//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>
class DagorAssetMgr;
class DagorAsset;
class IDaBuildInterface;
class ILogWriter;
class DataBlock;
class IGenSave;
class String;
namespace ddsx
{
struct Buffer;
struct ConvertParams;
} // namespace ddsx


//! daBuild cache uses gameres callbacks to build and create resources on-fly
namespace dabuildcache
{
//! load dabuild DLL
bool init(const char *start_dir, ILogWriter *l);
bool init(const char *start_dir, ILogWriter *l, IDaBuildInterface *_dabuild);
//! shutdown build cache
void term();

//! return pointer to dabuild interface (may be NULL, when init() failed)
IDaBuildInterface *get_dabuild();

//! binds dabuild to asset base, load export plugins and returns their count
int bind_with_mgr(DagorAssetMgr &mgr, DataBlock &appblk, const char *appdir, const char *ddsx_plugins_path = nullptr);

//! must be called after asset base update done to schedule changed resources for rebuild
void post_base_update_notify();

//! resolves and returns res_id for asset; returns -1 if asset is not related to gameres
int get_asset_res_id(const DagorAsset &a);

//! invalidates built gameres, specified by original asset
bool invalidate_asset(const DagorAsset &a, bool free_unused_resources = true);
} // namespace dabuildcache


//! load dabuild plugins only for IDagorAssetRefProvider interfaces
namespace assetrefs
{
//! load ref provider plugins for asset specified in application.blk
bool load_plugins(DagorAssetMgr &mgr, DataBlock &appblk, const char *start_dir, bool dabuild_exist = false);
//! unregister plugins from asset manager and unload DLLs
void unload_plugins(DagorAssetMgr &mgr);

//! returns list of asset types (excluding tex) subjected to export
dag::ConstSpan<int> get_exported_types();
//! returns list of all asset types (including tex) subjected to export
dag::ConstSpan<int> get_all_exported_types();
//! returns tex asset type
int get_tex_type();
} // namespace assetrefs


//! load/save asset local properties
namespace assetlocalprops
{
//! init asset local props management for specified application folders
void init(const char *app_dir, const char *local_dir = "develop/.asset-local");

//! creates folder specified by relative path
bool mkDir(const char *rel_path);

//! builds and returns full path for given relative path using highly temporary buffer
const char *makePath(const char *rel_path);

//! loads local props for asset; returns false on read error (missing file is NOT error)
bool load(const DagorAsset &a, DataBlock &dest);
//! saves local props for asset; returns false on write error
bool save(const DagorAsset &a, const DataBlock &src);

//! loads abitrary BLK file in local folder; returns false on read error (missing file is NOT error)
bool loadBlk(const char *rel_path, DataBlock &dest);
//! saves abitrary BLK file in local folder; returns false on write error
bool saveBlk(const char *rel_path, const DataBlock &src);
} // namespace assetlocalprops


//! cache for texture assets requiring conversion (can cooperate with daBuild when present)
namespace texconvcache
{
//! init cache - use tex exporter registered in mgr if present, or load new one and register otherwise
bool init(DagorAssetMgr &mgr, const DataBlock &appblk, const char *start_dir, bool dryrun = false, bool fastcvt = false);
//! shutdown cache - unload tex exporter if needed
void term();

//! change reqFastConv setting at runtime; return previous value
bool set_fast_conv(bool fast);

//! returns when texture asset is convertible
bool is_tex_asset_convertible(DagorAsset &a);

//! checks cache for specified asset and converted when cache is outdated;
//! returns: -1=error, 0=cache invalid, 1=cache valid, 2=cache updated
int validate_tex_asset_cache(DagorAsset &a, unsigned target, const char *profile, ILogWriter *l, bool upd = true);

//! returns DDSx for specified asset (converted or reused from cache) or returns false on error
bool get_tex_asset_built_ddsx(DagorAsset &a, ddsx::Buffer &dest, unsigned target, const char *profile, ILogWriter *l);

//! writes source DDS or converted DDSx texture asset contents to stream; returns size of data or -1 on error
int write_tex_contents(IGenSave &cwr, DagorAsset &a, unsigned target, const char *profile, ILogWriter *l);
//! return size of source DDS or converted DDSx texture asset
int get_tex_size(DagorAsset &a, unsigned target, const char *profile);

//! checks whether texture loaded from FAST cache
bool is_tex_built_fast(DagorAsset &a, unsigned target, const char *profile);

//! writes DDS for specified asset performing necessary processing for FLG_NEED_PAIRED_BASETEX; returns size of data or -1 on error
int __stdcall write_built_dds_final(IGenSave &cwr, DagorAsset &a, unsigned target, const char *profile, ILogWriter *l);


//! writes convertible texture asset as raw DDS to stream
bool get_tex_asset_built_raw_dds(DagorAsset &a, IGenSave &cwr, unsigned target, const char *profile, ILogWriter *l);

//! finds convertible texture asset by name and writes texture as raw DDS to stream
bool get_tex_asset_built_raw_dds_by_name(const char *texname, IGenSave &cwr, unsigned target, const char *profile, ILogWriter *l);

//! uses ddsx::convert_dds or previously cached result
bool convert_dds(ddsx::Buffer &b, const char *tex_path, const DagorAsset *a, unsigned target, const ddsx::ConvertParams &cp);
} // namespace texconvcache


namespace assethlp
{
//! validates exp blk settings; returns true when settings are OK, and outputs errors log and return false otherwise
bool validate_exp_blk(bool has_pkgs, const DataBlock &expBlk, const char *target_str, const char *profile, ILogWriter &log);

//! builds and returns both dest_base and pack_fname_prefix for given package
void build_package_dest_strings(String &out_dest_base, String &out_pack_fname_prefix, const DataBlock &expBlk, const char *pkg_name,
  const char *app_dir, const char *target_str, const char *profile);

//! builds and returns dest_base for given package
void build_package_dest(String &out_dest_base, const DataBlock &expBlk, const char *pkg_name, const char *app_dir,
  const char *target_str, const char *profile);

//! builds and returns pack_fname_prefix for given package
void build_package_pack_fname_prefix(String &out_pack_fname_prefix, const DataBlock &expBlk, const char *pkg_name, const char *app_dir,
  const char *target_str, const char *profile);
} // namespace assethlp


//! reload texture data from disk for given asset
bool reload_changed_texture_asset(const DagorAsset &a);
