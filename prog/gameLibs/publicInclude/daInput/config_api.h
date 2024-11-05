//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;

namespace dainput
{
//! sets default preset to be used when loading config that doesn't specify preset (usually, initial empty ones)
void set_default_preset_prefix(const char *preset_name_prefix);
//! returns default preset name
const char *get_default_preset_prefix();
//! returns column count for default preset
int get_default_preset_column_count();

//! sets complementary bindings config to be used for any preset (this config doesn't override preset data but only complements it)
void set_invariant_preset_complementary_config(const DataBlock &preset_cfg);

//! loads config (contains preset name, customized binding (if any) and some related properties)
bool load_user_config(const DataBlock &cfg);
//! saves config (when save_all_bindings_if_any_changed==false only changed bindings will be written)
bool save_user_config(DataBlock &cfg, bool save_all_bindings_if_any_changed = true);

//! returns current name of preset (which user config is based on)
const char *get_user_config_base_preset();
//! returns true if user config is somehow customizes preset
bool is_user_config_customized();

//! resets config to specified preset (may cherry-pick changes between config and prev preset to new config)
void reset_user_config_to_preset(const char *preset_name_prefix, bool move_changes = false);
//! resets config to current preset
void reset_user_config_to_currest_preset();

//! returns block with custom user props
DataBlock &get_user_props();
//! returns true if user props is somehow customized
bool is_user_props_customized();

//! terminate user config and free data structures
void term_user_config();
} // namespace dainput
