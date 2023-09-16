#pragma once

#include <generic/dag_tabFwd.h>

class DagorAssetMgr;
class DagorAsset;
class DataBlock;


bool init_dabuild_cache(const char *start_dir);
void term_dabuild_cache();

int bind_dabuild_cache_with_mgr(DagorAssetMgr &mgr, DataBlock &appblk, const char *appdir);

void post_base_update_notify_dabuild();

const char *get_asset_pack_name(DagorAsset *asset);

bool check_assets_base_up_to_date(dag::ConstSpan<const char *> packs, bool tex, bool res);
void rebuild_assets_in_folders_single(unsigned trg_code, dag::ConstSpan<int> folders_idx, bool tex, bool res);
void rebuild_assets_in_folders(dag::ConstSpan<unsigned> tc, dag::ConstSpan<int> folders_idx, bool tex, bool res);
void rebuild_assets_in_root(dag::ConstSpan<unsigned> tc, bool build_tex, bool build_res);
void rebuild_assets_in_root_single(unsigned trg_code, bool build_tex, bool build_res);
void destroy_assets_cache(dag::ConstSpan<unsigned> tc);
void destroy_assets_cache_single(unsigned trg_code);
void build_assets(dag::ConstSpan<unsigned> tc, dag::ConstSpan<DagorAsset *> assets);
bool is_asset_exportable(DagorAsset *asset);
