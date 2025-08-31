//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
struct ProhibitedBankDesc
{
  const char *filename;
  intptr_t filesize;
  typedef uint64_t file_hash_t;
  file_hash_t filehash;
  const char *label;
};
using ProhibitedBankDescs = dag::ConstSpan<ProhibitedBankDesc>;

void init(const DataBlock &blk, const ProhibitedBankDescs &prohibited_bank_descs = {});

using PresetLoadedCallback = eastl::function<void(str_hash_t, bool)>;
using ErrorCallback = eastl::function<void(const char * /*sndsys_message*/, const char * /*fmod_error_message*/,
  const char * /*bank_path*/, bool /*is_mod*/)>;
using PathTags = dag::ConstSpan<eastl::pair<const char *, const char *>>;

void enable(const char *preset_name, bool enable = true, const PathTags &path_tags = {});
void enable_starting_with(const char *preset_name_starts_with, bool enable = true, const PathTags &path_tags = {});

const char *get_master_preset();

bool is_enabled(const char *preset_name);
bool is_loaded(const char *preset_name);
bool is_master_loaded();
bool is_exist(const char *preset_name);
bool is_preset_has_failed_banks(const char *preset_name);
void get_failed_banks_names(eastl::vector<eastl::string, framemem_allocator> &failed_banks_names);
void get_loaded_banks_names(eastl::vector<eastl::string, framemem_allocator> &banks_names);
void get_mod_banks_names(eastl::vector<eastl::string, framemem_allocator> &banks_names);
bool are_banks_from_preset_exist(const char *preset_name, const char *lang = nullptr, const char *type = nullptr);

bool is_valid_event(const FMODGUID &event_id);
bool is_valid_event(const char *full_path);

void set_preset_loaded_cb(PresetLoadedCallback cb);
bool any_banks_pending();

void set_err_cb(ErrorCallback cb);
void unload_banks_sample_data(void);
} // namespace banks
} // namespace sndsys
