#pragma once

#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <util/dag_oaHashNameMap.h>

class String;
class DataBlock;
class ILogWriter;
class IGenericProgressIndicator;
namespace mkbindump
{
class BinDumpSaveCB;
}

class DagorAssetMgr;
class DagorAsset;
class AssetExportCache;

struct GrpAndTexPackId
{
  short respackId, texPackId;
};

struct AssetPack
{
  Tab<DagorAsset *> assets;
  SimpleString packName;
  int packageId;
  bool exported;
  int buildRes;

  AssetPack(const char *nm, int pi) : assets(tmpmem), packName(nm), exported(true), packageId(pi), buildRes(-1) {}
};

#define IMPORTANT_NOTE ((ILogWriter::MessageType)(ILogWriter::NOTE | 0x100))

bool loadAssetBase(DagorAssetMgr &mgr, const char *app_dir, const DataBlock &appblk, IGenericProgressIndicator &pbar, ILogWriter &log);

bool exportAssets(DagorAssetMgr &mgr, const char *app_dir, unsigned targetCode, const char *profile,
  dag::ConstSpan<const char *> pack_to_build, dag::ConstSpan<const char *> folder_restrict, dag::ConstSpan<bool> exp_types_mask,
  const DataBlock &appblk, IGenericProgressIndicator &pbar, ILogWriter &log, bool export_tex = true, bool export_res = true);


bool loadExporterPlugins(const DataBlock &appblk, DagorAssetMgr &mgr, const char *dirpath, dag::ConstSpan<bool> exp_types_mask,
  ILogWriter &log);
bool loadSingleExporterPlugin(const DataBlock &appblk, DagorAssetMgr &mgr, const char *dll_path, dag::ConstSpan<bool> exp_types_mask,
  ILogWriter &log);

void unloadExporterPlugins(DagorAssetMgr &mgr);

bool checkUpToDate(DagorAssetMgr &mgr, const char *app_dir, unsigned targetCode, const char *profile, int tc_flags,
  dag::ConstSpan<const char *> packs_to_check, dag::ConstSpan<bool> exp_types_mask, const DataBlock &appblk,
  IGenericProgressIndicator &pbar, ILogWriter &log);


bool buildGameResPack(mkbindump::BinDumpSaveCB &cwr, dag::ConstSpan<DagorAsset *> assets, AssetExportCache &c4, const char *pack_fname,
  ILogWriter &log, IGenericProgressIndicator &pbar, bool &up_to_date, const DataBlock &blk_export_props);

bool buildDdsxTexPack(mkbindump::BinDumpSaveCB &cwr, dag::ConstSpan<DagorAsset *> assets, AssetExportCache &c4, const char *pack_fname,
  ILogWriter &log, IGenericProgressIndicator &pbar, bool &up_to_date);

bool checkDdsxTexPackUpToDate(unsigned tc, const char *profile, bool be, dag::ConstSpan<DagorAsset *> assets, AssetExportCache &c4,
  const char *pack_fname, int ch_bit);

bool checkGameResPackUpToDate(dag::ConstSpan<DagorAsset *> assets, AssetExportCache &c4, const char *pack_fname, int ch_bit);

bool isAssetExportable(DagorAssetMgr &mgr, DagorAsset *asset, dag::ConstSpan<bool> exp_types_mask);

void preparePacks(DagorAssetMgr &mgr, dag::ConstSpan<bool> exp_types_mask, const DataBlock &expblk, Tab<AssetPack *> &tex_pack,
  Tab<AssetPack *> &grp_pack, FastNameMapEx &addPackages, ILogWriter &log, bool tex, bool res, const char *target_str,
  const char *profile);
bool get_exported_assets(Tab<DagorAsset *> &dest, dag::ConstSpan<DagorAsset *> src, const char *ts, const char *profile);
void dabuild_list_extra_packs(const char *app_dir, const DataBlock &appblk, DagorAssetMgr &mgr, const FastNameMap &pkg_to_list,
  unsigned targetCode, const char *profile, ILogWriter &log);

void dabuild_list_packs(const char *app_dir, const DataBlock &appblk, DagorAssetMgr &mgr, const FastNameMap &pkg_to_list,
  unsigned targetCode, const char *profile, bool export_tex, bool export_res, dag::ConstSpan<const char *> packs_re, ILogWriter &log);

bool cmp_data_eq(mkbindump::BinDumpSaveCB &cwr, const char *pack_fname);
bool cmp_data_eq(const void *data, int len, const char *pack_fname);

int64_t get_file_sz(const char *fn);

extern int stat_grp_total, stat_grp_built, stat_grp_failed, stat_grp_unchanged;
extern int stat_tex_total, stat_tex_built, stat_tex_failed, stat_tex_unchanged;
extern int stat_pack_removed;
extern int64_t stat_grp_sz_diff, stat_tex_sz_diff;
extern int64_t stat_changed_grp_total_sz, stat_changed_tex_total_sz;

extern bool dabuild_dry_run;

extern bool dabuild_strip_d3d_res;
extern bool dabuild_collapse_packs;

extern bool dabuild_skip_any_build;
extern bool dabuild_force_dxp_rebuild;
extern bool dabuild_build_tex_separate;

extern int dabuild_dxp_write_ver;
extern int dabuild_grp_write_ver;
bool setup_dxp_grp_write_ver(const DataBlock &build_blk, ILogWriter &log);

extern String dabuild_progress_prefix_text;

void make_cache_fname(String &cache_fname, const char *cache_base, const char *pkg_name, const char *pack_name, const char *target_str,
  const char *profile);

int make_exp_types_mask(Tab<bool> &exp_types_mask, DagorAssetMgr &mgr, const DataBlock &expblk, ILogWriter &log);

void dabuild_prepare_out_blk(DataBlock &dest, DagorAssetMgr &mgr, const DataBlock &build_blk);
void dabuild_finish_out_blk(DataBlock &dest, DagorAssetMgr &mgr, const DataBlock &build_blk, const DataBlock &export_blk,
  const char *app_dir, unsigned tc, const char *profile);
