// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/menu.h>
#include <generic/dag_tabFwd.h>
#include <util/dag_string.h>

class DagorAssetMgr;
class DagorAsset;
class DataBlock;


bool init_dabuild_cache(const char *start_dir);
void term_dabuild_cache();

struct DaBuildProgress
{
  int phase = 0; // 0 = none, 1 = tex packs, 2 = grp packs
  int packDone = 0;
  int packTotal = 0;
  int assetDone = 0;
  int assetTotal = 0;
};

bool is_dabuild_running();
void stop_dabuild_background();
void update_dabuild_background(PropPanel::IMenu *mm);
DaBuildProgress get_dabuild_progress();
int get_dabuild_jobs();
void set_dabuild_jobs(int jobs);
int get_dabuild_max_jobs();
bool get_dabuild_current_build_uses_jobs();
void render_dabuild_imgui();

int bind_dabuild_cache_with_mgr(DagorAssetMgr &mgr, DataBlock &appblk, const char *appdir);

void post_base_update_notify_dabuild();

String get_asset_pack_name(DagorAsset *asset);
String get_asset_pkg_name(DagorAsset *asset);

bool check_assets_base_up_to_date(dag::ConstSpan<const char *> packs, bool tex, bool res);
void rebuild_assets_in_folders_single(unsigned trg_code, dag::ConstSpan<int> folders_idx, bool tex, bool res);
void rebuild_assets_in_folders(dag::ConstSpan<unsigned> tc, dag::ConstSpan<int> folders_idx, bool tex, bool res);
void rebuild_assets_in_root(dag::ConstSpan<unsigned> tc, bool build_tex, bool build_res);
void rebuild_assets_in_root_single(unsigned trg_code, bool build_tex, bool build_res);
void destroy_assets_cache(dag::ConstSpan<unsigned> tc);
void destroy_assets_cache_single(unsigned trg_code);
void build_assets(dag::ConstSpan<unsigned> tc, dag::ConstSpan<DagorAsset *> assets);
bool is_asset_exportable(DagorAsset *asset);
