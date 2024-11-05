// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>
#include <util/dag_fastIntList.h>
#include <assets/assetMgr.h>

ECS_DECLARE_BOXED_TYPE(DagorAssetMgr);

class ILogWriter;

void install_asset_manager_hooks();
void init_asset_import_texture_factory();
void reregister_texture_asset_manager_factory(const DagorAsset &a);

ILogWriter *get_asset_manager_log_writer();
const DagorAssetMgr *get_asset_manager();
dag::ConstSpan<int> get_exportable_types();