//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <assets/asset.h>
#include <3d/dag_buildOnDemandTexFactory.h>

class DagorAssetMgr;
class ILogWriter;

namespace texconvcache
{
//! init build-on-demand texture factory
bool init_build_on_demand_tex_factory(DagorAssetMgr &mgr, ILogWriter *con);
//! shutdown build-on-demand texture factory
void term_build_on_demand_tex_factory();
//! stops tex loading (and optionally disables async tex loading after that); returns previous allow_loading
bool build_on_demand_tex_factory_cease_loading(bool allow_further_loading);

//! fills current state of async textures loading; returns false when factory not inited
bool get_tex_factory_current_loading_state(AsyncTextureLoadingState &out_state);

//! limit texture quality to max specified level for textures in list
void build_on_demand_tex_factory_limit_tql(TexQL max_tql, const TextureIdSet &tid);
//! limit texture quality to max specified level for all registered textures)
void build_on_demand_tex_factory_limit_tql(TexQL max_tql);

//! schedule async tex prebuilding with specified quality
void schedule_prebuild_tex(DagorAsset *tex_a, TexQL ql);
} // namespace texconvcache
