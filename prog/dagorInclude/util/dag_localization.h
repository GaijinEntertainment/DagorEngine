//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>
#include <util/dag_oaHashNameMap.h>

class MemGeneralLoadCB;
class DataBlock;

typedef class LocalizedTextEntry *LocTextId;

//! returns currently set default language in dgs_get_settings()
const char *get_current_language();
//! returns override language for platforms
const char *get_force_language();
void set_language_to_settings(const char *lang);

LocTextId get_localized_text_id(const char *key, bool ci = false);
LocTextId get_localized_text_id_silent(const char *key, bool ci = false);
LocTextId get_optional_localized_text_id(const char *key, bool ci = false);


int get_plural_form_id_for_lang(const char *lang, int num);
int get_plural_form_id(int num);
const char *get_localized_text(LocTextId id);
const char *get_fake_loc_for_missing_key(const char *key);

__forceinline const char *get_localized_text(const char *key, bool ci = false)
{
  LocTextId locId = get_localized_text_id(key, ci);
  if (!locId)
#if _TARGET_PC | DAGOR_DBGLEVEL > 0
    return get_fake_loc_for_missing_key(key);
#else
    return "";
#endif
  return get_localized_text(locId);
}

__forceinline const char *get_localized_text_silent(const char *key, bool ci = false)
{
  LocTextId locId = get_localized_text_id_silent(key, ci);
  if (!locId)
#if _TARGET_PC | DAGOR_DBGLEVEL > 0
    return get_fake_loc_for_missing_key(key);
#else
    return "";
#endif
  return get_localized_text(locId);
}

__forceinline const char *get_localized_text(const char *key, const char *def, bool ci = false)
{
  LocTextId locId = get_optional_localized_text_id(key, ci);
  if (!locId)
    return def;
  return get_localized_text(locId);
}

bool does_localized_text_exist(const char *key, bool ci = false);

const char *get_localized_text_for_lang(const char *key, const char *lang);

bool load_localization_table_from_csv(const char *csv_filename, int lang_col = 0);
bool load_localization_table_from_csv(MemGeneralLoadCB *cb, int lang_col = 0);
bool load_localization_table_from_csv_V2(const char *csv_filename, const char *lang = NULL);
bool load_localization_table_from_csv_V2(MemGeneralLoadCB *cb, const char *lang = NULL);
bool load_col_from_csv(const char *file_name, int col_no, NameMap *ids, Tab<char *> *col);

bool startup_localization(const DataBlock &blk, int lang_col = 0);

// loc tables V2
bool startup_localization_V2(const DataBlock &blk, const char *lang = NULL);
const char *get_default_lang();
const char *get_default_lang(const DataBlock &blk);

void set_default_lang_override(const char *lang);
bool is_default_lang_override_set();

const char *language_to_locale_code(const char *language);

//! clears all localization data (keys and strings)
void shutdown_localization();

// returns list of all keys and strings
void get_all_localization(Tab<const char *> &loc_keys, Tab<const char *> &loc_vals);

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
