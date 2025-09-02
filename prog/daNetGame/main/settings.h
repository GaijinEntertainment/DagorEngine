// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/functional.h>
#include <startup/dag_loadSettings.h>
#include <util/dag_oaHashNameMap.h>

class DataBlock;

String get_config_filename();

void initial_load_settings();
void load_gameparams();
void save_settings(const SettingsHashMap *changed_settings, bool apply_settings = true);
void clear_settings();
void commit_settings(const eastl::function<void(DataBlock &)> &mutate_cb);
bool do_settings_changes_need_videomode_change(const FastNameMap &changed_fields);
void apply_settings_changes(const FastNameMap &changed_fields);

void start_settings_async_saver_jobmgr();
void stop_settings_async_saver_jobmgr();
void get_settings_async_change_state(unsigned &out_change_gen, unsigned &out_saved_gen);
inline bool are_settings_changes_committed()
{
  unsigned change_gen = 0, saved_gen = 0;
  get_settings_async_change_state(change_gen, saved_gen);
  return change_gen == saved_gen;
}

int get_platformed_value(const DataBlock &blk, const char *key, int def);
const char *get_platformed_value(const DataBlock &blk, const char *key, const char *def);
const char *get_platform_string();

DataBlock *get_settings_override_blk();

// apply changes to config, from commandline, graphics preset
void dgs_apply_changes_to_config(DataBlock &config_blk, bool need_merge_cmd = false, const OverrideFilter *override_filter = nullptr);

// apply changes by graphics preset name from .patch file
void dgs_apply_console_preset_params(DataBlock &config_blk, const char *preset_name);

// apply changes by graphics preset name from settings.blk, to avoid waiting for UI (only "essential" params, ie. texture quality)
void dgs_apply_essential_pc_preset_params(DataBlock &config_blk, const char *preset_name);

// apply preset params (from pcGraphicalPresets) to config, from commandline
void dgs_apply_essential_pc_preset_params_to_config_from_cmd();

// apply auto graphical settings based on hardware
void try_apply_auto_graphical_settings();