//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>
#include <util/dag_oaHashNameMap.h>
#include <stdint.h>

class MemGeneralLoadCB;
class DataBlock;

// Opaque handle: in current implementation a pointer to the localized string
// itself (which can also be the same character data shared with another key).
// Treat as opaque -- only pass back to get_localized_text() or test against
// nullptr. IDs returned by get_localized_text_id may become invalid across
// clear_rta_localization() and shutdown_localization(); do not cache.
typedef class LocalizedTextEntry *LocTextId;

// Opaque caller-owned key dump. Pass to startup_localization*/load_localization*
// to collect the (hash -> key string) mapping needed by get_all_localization()
// and by dev-build collision verification. nullptr is fine -- dev builds will
// allocate a temporary local dump for collision checks; release builds skip.
struct LocKeyDump;
LocKeyDump *make_loc_key_dump();
void destroy_loc_key_dump(LocKeyDump *);

//! returns currently set default language in dgs_get_settings()
const char *get_current_language();
//! returns override language for platforms
const char *get_force_language();
void set_language_to_settings(const char *lang);

int getLangId(const char *lnag);
const char *getLangById(int id);

LocTextId get_localized_text_id(const char *key);
LocTextId get_localized_text_id_silent(const char *key);
LocTextId get_optional_localized_text_id(const char *key);


int get_plural_form_id_for_lang(const char *lang, int num);
int get_plural_form_id(int num);
const char *get_localized_text(LocTextId id);
const char *get_fake_loc_for_missing_key(const char *key);

__forceinline const char *get_localized_text(const char *key)
{
  LocTextId locId = get_localized_text_id(key);
  if (!locId)
#if _TARGET_PC | DAGOR_DBGLEVEL > 0
    return get_fake_loc_for_missing_key(key);
#else
    return "";
#endif
  return get_localized_text(locId);
}

__forceinline const char *get_localized_text_silent(const char *key)
{
  LocTextId locId = get_localized_text_id_silent(key);
  if (!locId)
#if _TARGET_PC | DAGOR_DBGLEVEL > 0
    return get_fake_loc_for_missing_key(key);
#else
    return "";
#endif
  return get_localized_text(locId);
}

__forceinline const char *get_localized_text(const char *key, const char *def)
{
  LocTextId locId = get_optional_localized_text_id(key);
  if (!locId)
    return def;
  return get_localized_text(locId);
}

bool does_localized_text_exist(const char *key);

const char *get_localized_text_for_lang(const char *key, const char *lang);
const char *get_localized_text_for_lang_id(const char *key, int lang_id);

//! Loads a CSV into the localization table. Keys already present in the table
//! are overwritten with the new value; the old value bytes are not reclaimed
//! until shutdown_localization. For repeated reloads, prefer shutdown_localization
//! followed by a fresh startup_localization_V2.
bool load_localization_table_from_csv(const char *csv_filename, int lang_col = 0, LocKeyDump *out_keys = nullptr);
bool load_localization_table_from_csv(MemGeneralLoadCB *cb, int lang_col = 0, LocKeyDump *out_keys = nullptr);
bool load_localization_table_from_csv_V2(const char *csv_filename, const char *lang = NULL, LocKeyDump *out_keys = nullptr);
bool load_localization_table_from_csv_V2(MemGeneralLoadCB *cb, const char *lang = NULL, LocKeyDump *out_keys = nullptr);
bool load_col_from_csv(const char *file_name, int col_no, NameMap *ids, Tab<char *> *col);

bool startup_localization(const DataBlock &blk, int lang_col = 0, LocKeyDump *out_keys = nullptr);

// loc tables V2
bool startup_localization_V2(const DataBlock &blk, const char *lang = NULL, LocKeyDump *out_keys = nullptr);
const char *get_default_lang();
const char *get_default_lang(const DataBlock &blk);

void set_default_lang_override(const char *lang);
bool is_default_lang_override_set();

const char *language_to_locale_code(const char *language);

//! clears all localization data (keys and strings)
void shutdown_localization();

// Returns list of all keys (from the dump) and their current main-table values.
// Multi-language ("full") keys loaded via the editor path are not returned --
// they live in a separate table not indexed by main.
// The dump must have been populated by startup_localization*/load_localization*
// against the same locTable; an empty or null dump returns no entries
// regardless of build config.
// Not safe to call concurrently with a load that is mutating the same dump.
void get_all_localization(const LocKeyDump *key_dump, Tab<const char *> &loc_keys, Tab<const char *> &loc_vals);

//! dagor callback to be called when duplicate loc key detected in localization files
//! if installed, must return false to disable loc value update, or true to overwrite loc value with new one (default)
extern bool (*dag_on_duplicate_loc_key)(const char *key);


// additional runtime localization support
// (these entries are searched through after main localization tables)

//! adds (replaces) additinal runtime localized text for key
void add_rta_localized_key_text(const char *key, const char *text);
//! adds (replaces) additinal runtime localized text/key pairs from BLK: key:t=text
void add_rta_localized_key_text(const DataBlock &key_text);
//! clears all additional runtime localization entries
void clear_rta_localization();
