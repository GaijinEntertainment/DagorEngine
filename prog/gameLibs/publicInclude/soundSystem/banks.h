//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <soundSystem/strHash.h>
#include <soundSystem/handle.h>
#include <EASTL/functional.h>
#include <EASTL/utility.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <generic/dag_span.h>

class framemem_allocator;
class DataBlock;

namespace sndsys
{
namespace banks
{
using PresetLoadedCallback = eastl::function<void(str_hash_t, bool)>;
using ErrorCallback = eastl::function<void(const char * /*sndsys_message*/, const char * /*fmod_error_message*/,
  const char * /*bank_path*/, bool /*is_mod*/)>;
using PathTags = dag::ConstSpan<eastl::pair<const char *, const char *>>;

void init(const DataBlock &blk);

void enable(const char *preset_name, bool enable = true, const PathTags &path_tags = {});
void enable_starting_with(const char *preset_name_starts_with, bool enable = true, const PathTags &path_tags = {});

const char *get_default_preset(const DataBlock &);

bool is_enabled(const char *preset_name);
bool is_loaded(const char *preset_name);
bool is_exist(const char *preset_name);
bool is_preset_has_failed_banks(const char *preset_name);
void get_failed_banks_names(eastl::vector<eastl::string, framemem_allocator> &failed_banks_names);
void get_loaded_banks_names(eastl::vector<eastl::string, framemem_allocator> &banks_names);
void prohibit_bank_events(const eastl::string &bank_name);
bool is_guid_prohibited(const FMODGUID &event_id);
void clear_prohibited_guids();

void set_preset_loaded_cb(PresetLoadedCallback cb);
bool any_banks_pending();
bool are_inited();

void set_err_cb(ErrorCallback cb);
void unload_banks_sample_data(void);
} // namespace banks
} // namespace sndsys
