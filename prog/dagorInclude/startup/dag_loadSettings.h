//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/functional.h>
#include <util/dag_oaHashNameMap.h>


class DataBlock;

using OverrideFilter = eastl::function<bool(const char *)>;
using SettingsHashMap = OAHashNameMap<false>;
OverrideFilter gen_default_override_filter(const SettingsHashMap *changed_settings);

// to be used as static instance in 'main' module: static DagorSettingsBlkHolder stgBlkHolder;
struct DagorSettingsBlkHolder
{
  DagorSettingsBlkHolder();
  ~DagorSettingsBlkHolder();
};

// generic loading of settings.blk (and optionally config.blk with partial merge; cmdline may be applied to config.blk before merge)
void dgs_load_settings_blk(bool apply_cmd = true, const char *settings_blk_fn = "settings.blk",
  const char *config_blk_fn = "config.blk", bool force_apply_all = false, bool store_cfg_copy = true,
  bool resolve_tex_streaming = true);

// the same as dgs_load_settings_blk() but accepts config,cmd BLK  objects instead of filename
void dgs_load_settings_blk_ex(const DataBlock &filtered_cmd_blk, const char *settings_blk_fn, const DataBlock &config_blk,
  bool force_apply_all, bool store_cfg_copy, bool resolve_tex_streaming);

// generic loading of gameParams.blk
void dgs_load_game_params_blk(const char *game_params_blk_fn = "config/gameParams.blk");

// command line parsing (-config:block1/block2/param:X=val)
void dgs_apply_command_line_to_config(DataBlock *target, const OverrideFilter *override_filter = nullptr);
void dgs_apply_command_line_arg_to_blk(DataBlock *target, const char *value, const char *target_desc = NULL);
// returns arg with sensitive values (login, password) masked; buf is used as storage when masking is needed
const char *dgs_cli_arg_value_for_log(const char *arg, char *buf, int buf_size);

// applies settings from config_blk to settings blk (using scheme in settings blk for Config=rel or direct merge for Config=dev)
void dgs_apply_config_blk(const DataBlock &config_blk, bool force_apply_all, bool store_cfg_copy, bool save_order = false);

// applies settings from config_blk to settings_blk (using scheme in settings_blk for Config=rel or direct merge for Config=dev)
void dgs_apply_config_blk_ex(DataBlock &settings_blk, const DataBlock &config_blk, bool force_apply_all, bool store_cfg_copy,
  bool save_order = false);

// clear saved config params based on __allowedSavedConfig
void dgs_filter_saved_config(DataBlock &config_blk);
