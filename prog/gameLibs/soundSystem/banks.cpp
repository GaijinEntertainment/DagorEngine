// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/fixed_string.h>
#include <EASTL/bitset.h>
#include <EASTL/sort.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_localization.h>
#include <memory/dag_framemem.h>
#include <fmod_studio.hpp>
#include <soundSystem/soundSystem.h>
#include <soundSystem/fmodApi.h>
#include <soundSystem/banks.h>
#include <soundSystem/debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_globalSettings.h>
#include "internal/framememString.h"
#include "internal/attributes.h"
#include "internal/banks.h"
#include "internal/banks.verifyMods.h"
#include "internal/pathHash.h"
#include "internal/visualLabels.h"
#include "internal/attributes.h"
#include "internal/debug.h"
#if _TARGET_ANDROID || _TARGET_IOS
#include <crashlytics/firebase_crashlytics.h>
#endif
#include <atomic>

namespace sndsys::banks
{
static WinCritSec g_banks_cs;
#define SNDSYS_BANKS_BLOCK WinAutoLock banksLock(g_banks_cs);

static const size_t max_banks = 512;
using Bitset = eastl::bitset<max_banks>;

static bool g_report_bank_loading_time = false;

struct Bank
{
  using id_t = uint32_t;
  using label_t = uint32_t;

  FMOD::Studio::Bank *fmodBank = nullptr;
  const eastl::string path;
  const label_t label; // filename trimmed after dot (e.g. hash("master_bank"), same for "/master_bank.bank",
                       // "/master_bank.strings.bank", "/master_bank.assets.bank")
  const id_t id = 0;

  const bool isAsync = false;
  const bool isPreload = false;
  const bool isLoadToMemory = false;
  const bool isOptional = false;
  const bool isMod = false;

  Bank() = delete;
  Bank(const char *path, label_t label, id_t id, bool is_async, bool is_preload, bool is_load_to_memory, bool is_optional,
    bool is_mod) :
    path(path),
    label(label),
    id(id),
    isAsync(is_async),
    isPreload(is_preload),
    isLoadToMemory(is_load_to_memory),
    isOptional(is_optional),
    isMod(is_mod)
  {}
};

struct Preset
{
  Bitset banks;
  const eastl::string name;
  const str_hash_t hash = {};
  bool isEnabled = false;
  bool isLoaded = false;

  Preset() = delete;
  Preset(const char *name_) : name(name_), hash(SND_HASH_SLOW(name_)) {}
};

struct Plugin
{
  uint32_t handle = 0;
  const eastl::string path;

  Plugin() = delete;
  Plugin(const char *path_) : path(path_) {}
};

static eastl::vector<Bank> all_banks;
static eastl::vector<Preset> all_presets;
static eastl::vector<Plugin> all_plugins;

static Bitset pending_banks;
static Bitset loaded_banks;
static Bitset failed_banks;

static eastl::fixed_string<char, 8> locale;
static eastl::fixed_string<char, 8> master_preset_name;
static eastl::string g_banks_folder;
static const char *mod_path = "mod/";

static bool g_was_inited = false;

static void def_err_cb(const char *sndsys_message, const char *fmod_error_message, const char *bank_path, bool is_mod)
{
#if DAGOR_DBGLEVEL == 0
  if (!is_mod)
#endif
  {
    logerr("[SNDSYS] %s \"%s\" \"%s\"", sndsys_message, bank_path, fmod_error_message);
  }
  G_UNREFERENCED(is_mod);
}

static ErrorCallback err_cb = def_err_cb;
static PresetLoadedCallback preset_loaded_cb = nullptr;

static inline bool is_async(Bank::id_t bank_id) { return all_banks[bank_id].isAsync; }
static inline bool is_preload(Bank::id_t bank_id) { return all_banks[bank_id].isPreload; }

static inline bool is_loaded(Bank::id_t bank_id) { return loaded_banks[bank_id]; }
static inline bool is_pending(Bank::id_t bank_id) { return pending_banks[bank_id]; }
static inline bool is_failed(Bank::id_t bank_id) { return failed_banks[bank_id]; }

static inline bool is_loaded_or_pending(Bank::id_t bank_id) { return is_loaded(bank_id) || is_pending(bank_id); }
static inline bool is_valid_events_in_bank(Bank::id_t id) { return is_loaded(id) && !is_failed(id); }

// ............................................................

static HashedKeySet<guid_hash_t> g_all_valid_event_guids;
static std::atomic_int g_debug_all_valid_event_guids_count = ATOMIC_VAR_INIT(0);

static WinCritSec g_valid_event_guids_cs;
#define SNDSYS_VALID_EVENT_GUIDS_BLOCK WinAutoLock validEventGuidsLock(g_valid_event_guids_cs);

static bool is_valid_events_in_bank_with_label(Bank::label_t bank_label)
{
  for (const Bank &bank : all_banks)
    if (bank.label == bank_label && !is_valid_events_in_bank(bank.id))
      return false;
  return true;
};

static void gather_all_valid_event_guids()
{
  SNDSYS_BANKS_BLOCK;
  SNDSYS_VALID_EVENT_GUIDS_BLOCK;

  eastl::vector<FMOD::Studio::EventDescription *, framemem_allocator> eventList;
  g_all_valid_event_guids.clear();

  for (const Bank &bank : all_banks)
  {
    if (!is_valid_events_in_bank_with_label(bank.label))
      continue;

    int eventCount = 0;
    if (FMOD_OK != bank.fmodBank->getEventCount(&eventCount))
      continue;

    if (eventCount == 0)
      continue;

    eventList.resize(eventCount);
    if (FMOD_OK != bank.fmodBank->getEventList(eventList.begin(), eventCount, &eventCount))
      continue;

    G_ASSERT(eventCount == eventList.size());
    for (const FMOD::Studio::EventDescription *eventDesc : eventList)
    {
      FMODGUID guid;
      if (FMOD_OK == eventDesc->getID(reinterpret_cast<FMOD_GUID *>(&guid)))
        g_all_valid_event_guids.insert(hash_fun(guid));
    }
  }
  g_debug_all_valid_event_guids_count = g_all_valid_event_guids.size();
}

static void validate_cache()
{
  invalidate_attributes_cache();
  gather_all_valid_event_guids();
}

bool is_valid_event(const FMODGUID &event_id)
{
  SNDSYS_VALID_EVENT_GUIDS_BLOCK;
  return g_all_valid_event_guids.has_key(hash_fun(event_id));
}

bool is_valid_event(const char *full_path)
{
  G_ASSERT_RETURN(sndsys::is_inited(), false);

  FMODGUID guid;
  if (FMOD_OK == fmodapi::get_studio_system()->lookupID(full_path, reinterpret_cast<FMOD_GUID *>(&guid)))
    return is_valid_event(guid);

  return false;
}

// ............................................................

static inline void replace(FrameStr &path, const char *what, const char *with)
{
  eastl_size_t idx = path.find(what);
  if (idx != FrameStr::npos)
    path.replace(idx, strlen(what), with);
}
static inline void make_localized(FrameStr &str) { replace(str, "<loc>", locale.c_str()); }

// ............................................................

static inline FMOD_RESULT load_bank_memory(const char *filename, FMOD_STUDIO_LOAD_BANK_FLAGS flags, FMOD::Studio::Bank **fmod_bank)
{
  eastl::vector<char> data;

  if (file_ptr_t fp = df_open(filename, DF_READ))
  {
    FINALLY([fp]() { df_close(fp); });

    const intptr_t filesize = df_length(fp);
    G_ASSERT_AND_DO(filesize >= 0, { return FMOD_ERR_FILE_BAD; });

    data.resize(filesize);
    G_ASSERT_AND_DO(data.size() == filesize, { return FMOD_ERR_MEMORY; });

    const intptr_t sizeRead = df_read(fp, data.begin(), data.size());

    if (sizeRead != data.size())
      return FMOD_ERR_FILE_BAD;
  }
  else
  {
    return FMOD_ERR_FILE_NOTFOUND;
  }

  // When mode is FMOD_STUDIO_LOAD_MEMORY FMOD will allocate an internal buffer and copy the data from the passed in buffer before
  // using it. When used in this mode there are no alignment restrictions on buffer and the memory pointed to by buffer may be cleaned
  // up at any time after this function returns.

  return fmodapi::get_studio_system()->loadBankMemory(data.begin(), data.size(), FMOD_STUDIO_LOAD_MEMORY, flags, fmod_bank);
}

static inline void patch_path_with_mod(FrameStr &path)
{
  FrameStr moddedPath;
  moddedPath.sprintf("%s/%s", g_banks_folder.c_str(), mod_path);
  FrameStr fullModdedPath = path;
  fullModdedPath.replace(0, g_banks_folder.size() + 1, moddedPath);
  if (df_get_real_name(fullModdedPath.c_str()) != nullptr)
    path = fullModdedPath;
}

static inline void load_bank(Bank &bank, const PathTags &path_tags)
{
  G_ASSERT_RETURN(!is_loaded_or_pending(bank.id), );
  G_ASSERT_RETURN(!is_failed(bank.id), );
  G_ASSERT_RETURN(bank.fmodBank == nullptr, );

  FrameStr taggedPath = bank.path.c_str();
  for (auto &tag : path_tags)
    replace(taggedPath, tag.first, tag.second);

  if (bank.isMod && taggedPath.find(mod_path) == FrameStr::npos)
    patch_path_with_mod(taggedPath);

  const char *realPath = df_get_real_name(taggedPath.c_str());
  if (!realPath)
  {
    failed_banks.set(bank.id);
    if (bank.isOptional)
    {
      debug_trace_warn("Optional sound bank load was skipped because file \"%s\" is missing", taggedPath.c_str());
    }
    else
      err_cb("bank could not be loaded because there is no such file", "", taggedPath.c_str(), bank.isMod);
    return;
  }

  FMOD_STUDIO_LOAD_BANK_FLAGS flags = is_async(bank.id) ? FMOD_STUDIO_LOAD_BANK_NONBLOCKING : FMOD_STUDIO_LOAD_BANK_NORMAL;
  const int32_t start = get_time_msec();

  const FMOD_RESULT result = bank.isLoadToMemory ? load_bank_memory(realPath, flags, &bank.fmodBank)
                                                 : fmodapi::get_studio_system()->loadBankFile(realPath, flags, &bank.fmodBank);

  if (FMOD_OK != result)
  {
    failed_banks.set(bank.id);
    err_cb("bank could not be loaded", FMOD_ErrorString(result), realPath, bank.isMod);
    if (bank.fmodBank)
    {
      SOUND_VERIFY(bank.fmodBank->unload());
      bank.fmodBank = nullptr;
    }
    return;
  }

  if (is_async(bank.id))
    pending_banks.set(bank.id, true);
  else
  {
    loaded_banks.set(bank.id, true);
    if (is_preload(bank.id))
      SOUND_VERIFY(bank.fmodBank->loadSampleData());
    if (g_report_bank_loading_time)
    {
      const int32_t now = get_time_msec();
      debug("[SNDSYS] load bank \"%s\" in %d ms", realPath, now - start);
    }
  }
}

static inline void unload_bank(Bank &bank, eastl::vector<FMOD::Studio::Bank *, framemem_allocator> &fmod_banks)
{
  if (is_loaded_or_pending(bank.id))
  {
    debug("[SNDSYS] unload bank \"%s\"", bank.path.c_str());
    if (bank.fmodBank != nullptr)
      fmod_banks.push_back(bank.fmodBank);
    bank.fmodBank = nullptr;
    pending_banks.set(bank.id, false);
    loaded_banks.set(bank.id, false);
  }
  failed_banks.set(bank.id, false);
}

static inline void append_bank(const char *path, Bank::label_t label, bool is_async, bool is_preload, bool is_load_to_memory,
  bool is_optional, bool is_mod, Preset &preset)
{
  auto pred = [path](const Bank &bank) { return bank.path == path; };
  auto it = eastl::find_if(all_banks.begin(), all_banks.end(), pred);
  Bank::id_t bankId = {};
  if (it != all_banks.end())
  {
    bankId = it - all_banks.begin();
    G_ASSERTF(it->isMod == is_mod && it->isOptional == is_optional && it->isOptional == is_optional && it->isAsync == is_async &&
                it->isPreload == is_preload && it->isLoadToMemory == is_load_to_memory,
      "Append new to existing bank parameters mismatch: there is already bank with same path but different options");
  }
  else
  {
    bankId = all_banks.size();
    all_banks.emplace_back(path, label, bankId, is_async, is_preload, is_load_to_memory, is_optional, is_mod);
    G_ASSERT(all_banks.size() <= max_banks);
  }
  preset.banks.set(bankId);
}

static inline bool is_preset_loaded(const Preset &preset)
{
  return preset.banks.any() && preset.banks == (preset.banks & (loaded_banks | failed_banks));
}

using loaded_presets_t = eastl::vector<eastl::pair<str_hash_t /*preset_hash*/, bool /*is_loaded*/>, framemem_allocator>;

static void invoke_presets_callback(const loaded_presets_t &lp)
{
  if (preset_loaded_cb)
    for (auto &it : lp)
      preset_loaded_cb(it.first, it.second);
}

static inline void update_presets(bool update_cache, loaded_presets_t &lp)
{
  invalidate_visual_labels();

  bool cacheWasUpdated = update_cache;
  if (update_cache)
    validate_cache();

  for (auto &preset : all_presets)
  {
    const bool isLoaded = is_preset_loaded(preset);
    if (preset.isLoaded != isLoaded)
    {
      if (!cacheWasUpdated)
      {
        cacheWasUpdated = true;
        validate_cache();
      }
      preset.isLoaded = isLoaded;
      debug_trace_info("%s preset \"%s\", %d valid events total", isLoaded ? "loaded" : "unloaded", preset.name.c_str(),
        int(g_debug_all_valid_event_guids_count));
      lp.emplace_back(preset.hash, isLoaded);
    }
  }
}

static inline void init_locale_deprecated(const DataBlock &blk, const DataBlock &banks_blk)
{
  locale = "en";
  const DataBlock *localesBlk = banks_blk.getBlockByNameEx("locales", nullptr);
  G_ASSERT(localesBlk && localesBlk->blockCount());
  if (!localesBlk || !localesBlk->blockCount())
    return;
  const char *language = ::dgs_get_settings()->getStr("language", "English");
  language = blk.getStr("language", language);
  locale = language_to_locale_code(language);
  size_t s = locale.find('-', 0);
  if (s != eastl::string::npos)
    locale.erase(locale.begin() + s, locale.end());

  for (int i = 0; i < localesBlk->blockCount(); ++i)
  {
    if (locale == localesBlk->getBlock(i)->getBlockName())
      return;
  }
  locale = localesBlk->getBlock(0)->getBlockName();
}

static inline void init_locale(const DataBlock &blk, const DataBlock &banks_blk)
{
  locale = ::dgs_get_settings()->getBlockByNameEx("sound")->getStr("locale", "");
  if (!locale.empty())
    return;
  if (banks_blk.blockExists("locales"))
    init_locale_deprecated(blk, banks_blk);
  else
  {
    const char *language = get_current_language();
    language = blk.getStr("language", language);
    const DataBlock &languageToLocaleBlk = *banks_blk.getBlockByNameEx("languageToLocale");
    const char *def = languageToLocaleBlk.getStr("default", "en");
    locale = languageToLocaleBlk.getStr(language, def);
    if (locale.empty())
      locale = "en";
  }
}

static inline void add_plugin(const char *path)
{
  auto pred = [path](const Plugin &plugin) { return plugin.path == path; };
  auto it = eastl::find_if(all_plugins.begin(), all_plugins.end(), pred);
  if (it == all_plugins.end())
    all_plugins.emplace_back(path);
}

static inline void load_plugin(Plugin &plugin)
{
  FMOD_RESULT result = FMOD_OK;
  G_VERIFYF(FMOD_OK == (result = fmodapi::get_system()->loadPlugin(plugin.path.c_str(), &plugin.handle)),
    "[SNDSYS] error loading plugin '%s': %s", plugin.path.c_str(), FMOD_ErrorString(result));
}

#if (_TARGET_64BIT && _TARGET_PC_WIN) | _TARGET_XBOX
#define PLUGIN_SUBSYSTEM "win64"
#elif _TARGET_PC_WIN
#define PLUGIN_SUBSYSTEM "win32"
#elif _TARGET_PC_MACOSX
#define PLUGIN_SUBSYSTEM "macos"
#elif _TARGET_PC_LINUX
#define PLUGIN_SUBSYSTEM "linux"
#elif _TARGET_C1

#elif _TARGET_C2

#elif _TARGET_ANDROID
#define PLUGIN_SUBSYSTEM "android"
#elif _TARGET_IOS
#define PLUGIN_SUBSYSTEM "iOS"
#elif _TARGET_TVOS
#define PLUGIN_SUBSYSTEM "tvOS"
#elif _TARGET_C3

#endif

static void update_impl(loaded_presets_t &lp)
{
  SNDSYS_BANKS_BLOCK;
  if (!pending_banks.any())
    return;
  bool needUpdatePresets = false;
  for (Bank &bank : all_banks)
  {
    if (!pending_banks[bank.id])
      continue;
    G_ASSERT(bank.fmodBank);
    FMOD_STUDIO_LOADING_STATE state = FMOD_STUDIO_LOADING_STATE_ERROR;
    const FMOD_RESULT result = bank.fmodBank ? bank.fmodBank->getLoadingState(&state) : FMOD_OK;
    if (!bank.fmodBank || result != FMOD_OK)
    {
      pending_banks.set(bank.id, false);
      failed_banks.set(bank.id);
      needUpdatePresets = true;
      if (bank.fmodBank)
      {
        SOUND_VERIFY(bank.fmodBank->unload());
        bank.fmodBank = nullptr;
      }
      err_cb("error loading bank", FMOD_ErrorString(result), bank.path.c_str(), bank.isMod);
      continue;
    }

    if (state == FMOD_STUDIO_LOADING_STATE_LOADING)
      continue;

    if (state == FMOD_STUDIO_LOADING_STATE_LOADED)
    {
      debug("[SNDSYS] loaded bank '%s', isMod=%s", bank.path.c_str(), bank.isMod ? "true" : "false");
      loaded_banks.set(bank.id);
      if (is_preload(bank.id))
      {
        const FMOD_RESULT result = bank.fmodBank->loadSampleData();
        if (result != FMOD_OK)
          err_cb("error preload bank sample data", FMOD_ErrorString(result), bank.path.c_str(), bank.isMod);
      }
      failed_banks.set(bank.id, false);
    }
    else
    {
      failed_banks.set(bank.id);
      SOUND_VERIFY(bank.fmodBank->unload());
      bank.fmodBank = nullptr;
      err_cb("load bank failed", "", bank.path.c_str(), bank.isMod);
    }

    pending_banks.set(bank.id, false);
    needUpdatePresets = true;
  }

  if (needUpdatePresets)
    update_presets(false /*always_update_cache*/, lp);
}

void update()
{
  loaded_presets_t lp;
  update_impl(lp);
  invoke_presets_callback(lp);
}

static __forceinline bool need_load(int bank_id)
{
  for (auto &preset : all_presets)
  {
    if (preset.isEnabled && preset.banks[bank_id])
      return true;
  }
  return false;
}

static __forceinline Preset *find_preset(const char *name)
{
  auto pred = [name](const Preset &preset) { return preset.name == name; };
  Preset *it = eastl::find_if(all_presets.begin(), all_presets.end(), pred);
  return it != all_presets.end() ? it : nullptr;
}

static void add_bank(const char *file_name, const DataBlock &blk, const char *banks_folder, const char *extension, bool enable_mod,
  Preset &preset)
{
  G_ASSERT_RETURN(file_name != nullptr && *file_name, );

  bool isMod = false;
  bool isModAndTagged = false;

  FrameStr path;
  const char *overridedPath = blk.getStr("path", nullptr);

  if (enable_mod)
  {
    if (overridedPath)
      path.sprintf("%s/%s%s%s", overridedPath, mod_path, file_name, extension);
    else if (banks_folder && *banks_folder)
      path.sprintf("%s/%s%s%s", banks_folder, mod_path, file_name, extension);
    else
      path.sprintf("%s%s%s", mod_path, file_name, extension);

    make_localized(path);

    isMod = df_get_real_name(path.c_str()) != nullptr;
    if (path.find("<") != FrameStr::npos)
      isModAndTagged = true;
  }

  if (!isMod)
  {
    if (overridedPath)
      path.sprintf("%s/%s%s", overridedPath, file_name, extension);
    else if (banks_folder && *banks_folder)
      path.sprintf("%s/%s%s", banks_folder, file_name, extension);
    else
      path.sprintf("%s%s", file_name, extension);

    make_localized(path);
  }

  FrameStr label;
  if (const char *separator = strchr(file_name, '.'))
    label.assign(file_name, separator);
  else
    label = file_name;

  G_STATIC_ASSERT((eastl::is_same<Bank::label_t, str_hash_t>::value));
  append_bank(path.c_str(), SND_HASH_SLOW(label.c_str()), blk.getBool("async", false), blk.getBool("preload", false),
    blk.getBool("loadToMemory", false), blk.getBool("optional", false), isMod || isModAndTagged, preset);
};

void init(const DataBlock &blk, const ProhibitedBankDescs &prohibited_bank_descs)
{
  SNDSYS_IS_MAIN_THREAD;
  G_ASSERT_RETURN(sndsys::is_inited(), );
  SNDSYS_BANKS_BLOCK;
  G_ASSERT_RETURN(!g_was_inited, );
  g_was_inited = true;

  const DataBlock &banksBlk = *blk.getBlockByNameEx("banks");
  const DataBlock &modBlk = *blk.getBlockByNameEx("mod");

  init_locale(blk, banksBlk);
  debug("[SNDSYS]: locale is \"%s\"", locale.c_str());

  const char *folder = banksBlk.getStr("folder", "sound");
  g_banks_folder = eastl::string(folder);
  const char *extension = banksBlk.getStr("extension", ".bank");
  master_preset_name = banksBlk.getStr("preset", "master");

  bool enableMod = blk.getBool("enableMod", false) && modBlk.getBool("allow", true);
  if (enableMod && !verify_mods(folder, mod_path, extension, blk, prohibited_bank_descs))
  {
    enableMod = false;
    logwarn("[SNDSYS]: bank mods disallowed: prohibited banks found");
    // TODO: add callback to notify user about disallowed mods
  }
  else
  {
    debug("[SNDSYS]: bank mods %s", enableMod ? "enabled" : "disabled");
  }

  g_report_bank_loading_time = blk.getBool("reportBankLoadingTime", false);

  FrameStr name, path;
  const DataBlock &presetsBlk = *banksBlk.getBlockByNameEx("presets");
  all_presets.reserve(presetsBlk.blockCount());
  for (int j = 0; j < presetsBlk.blockCount(); ++j)
  {
    const DataBlock *presetBlk = presetsBlk.getBlock(j);
    all_presets.emplace_back(presetBlk->getBlockName());
    Preset &preset = all_presets.back();

    for (int i = 0; i < presetBlk->blockCount(); ++i)
    {
      const DataBlock *bankBlk = presetBlk->getBlock(i);
      add_bank(bankBlk->getBlockName(), *bankBlk, folder, extension, enableMod, preset);
    }

    for (int i = 0; i < presetBlk->paramCount(); ++i)
    {
      if (strcmp(presetBlk->getParamName(i), "bank") == 0)
        add_bank(presetBlk->getStr(i), DataBlock::emptyBlock, folder, extension, enableMod, preset);
      else
        G_ASSERTF(false, "Unexpected param name \"%s\"", presetBlk->getParamName(i));
    }
  }

  const DataBlock *pluginsBlk = banksBlk.getBlockByNameEx("plugins");
  all_plugins.reserve(pluginsBlk->blockCount());
  for (int j = 0; j < pluginsBlk->blockCount(); ++j)
  {
    const DataBlock *pluginBlk = pluginsBlk->getBlock(j);
    const char *fileName = pluginBlk->getStr(PLUGIN_SUBSYSTEM);
    all_plugins.emplace_back(fileName);
  }
}

const char *get_master_preset() { return master_preset_name.c_str(); }

using banks_for_unload_t = eastl::vector<FMOD::Studio::Bank *, framemem_allocator>;

static void enable_preset_impl(Preset &preset, bool enable, const PathTags &path_tags, loaded_presets_t &lp,
  banks_for_unload_t &banks_for_unload)
{
  if (preset.isEnabled == enable)
    return;

#if _TARGET_ANDROID || _TARGET_IOS
  crashlytics::AppState appState("snd_banks_load");
#endif

  const int32_t start = get_time_msec();
  preset.isEnabled = enable;

  eastl::vector<Bank::id_t, framemem_allocator> banksToLoad;

  for (auto &bank : all_banks)
  {
    if (need_load(bank.id))
    {
      if (!is_loaded_or_pending(bank.id) && !is_failed(bank.id))
        banksToLoad.push_back(bank.id);
    }
    else
      unload_bank(bank, banks_for_unload);
  }

  for (Plugin &plugin : all_plugins)
    load_plugin(plugin);

  for (Bank::id_t bankId : banksToLoad)
    load_bank(all_banks[bankId], path_tags);

  const int32_t now = get_time_msec();
  debug_trace_log("preset \"%s\" enabled in %d ms", preset.name.c_str(), now - start);

  update_presets(true /*always_update_cache*/, lp);
}

static void unload_banks_impl(banks_for_unload_t &banks_for_unload)
{
  for (auto &it : banks_for_unload)
    SOUND_VERIFY(it->unload());
}

bool are_banks_from_preset_exist(const char *preset_name, const char *lang)
{
  SNDSYS_BANKS_BLOCK;
  if (Preset *preset = find_preset(preset_name))
  {
    for (auto &bank : all_banks)
    {
      if (preset->banks.test(bank.id))
      {
        FrameStr bankPath = bank.path.c_str();
        if (lang)
          replace(bankPath, "<lang>", lang);
        if (!dd_file_exist(bankPath.c_str()))
          return false;
      }
    }
    return true;
  }
  return false;
}

void enable(const char *preset_name, bool enable, const PathTags &path_tags)
{
  loaded_presets_t lp;
  banks_for_unload_t banksForUnload;

  if (preset_name)
  {
    SNDSYS_BANKS_BLOCK;
    if (Preset *preset = find_preset(preset_name))
      enable_preset_impl(*preset, enable, path_tags, lp, banksForUnload);
    else
    {
      logerr("Preset \"%s\" not found, cannot load specified sound banks", preset_name);
    }
  }

  invoke_presets_callback(lp);

  unload_banks_impl(banksForUnload);
}

void enable_starting_with(const char *preset_name_starts_with, bool enable, const PathTags &path_tags)
{
  loaded_presets_t lp;
  banks_for_unload_t banksForUnload;
  if (preset_name_starts_with && *preset_name_starts_with)
  {
    SNDSYS_BANKS_BLOCK;
    const size_t len = strlen(preset_name_starts_with);
    for (Preset &preset : all_presets)
      if (strncmp(preset.name.c_str(), preset_name_starts_with, len) == 0)
        enable_preset_impl(preset, enable, path_tags, lp, banksForUnload);
  }

  invoke_presets_callback(lp);

  unload_banks_impl(banksForUnload);
}

bool is_enabled(const char *preset_name)
{
  SNDSYS_BANKS_BLOCK;
  const Preset *preset = find_preset(preset_name);
  return preset && preset->isEnabled;
}

bool is_loaded(const char *preset_name)
{
  SNDSYS_BANKS_BLOCK;
  const Preset *preset = find_preset(preset_name);
  return preset && preset->isLoaded;
}

bool is_exist(const char *preset_name)
{
  SNDSYS_BANKS_BLOCK;
  return find_preset(preset_name) != nullptr;
}

bool is_preset_has_failed_banks(const char *preset_name)
{
  SNDSYS_BANKS_BLOCK;
  const Preset *preset = find_preset(preset_name);
  return preset && (preset->banks & failed_banks).any();
}

void get_failed_banks_names(eastl::vector<eastl::string, framemem_allocator> &failed_banks_names)
{
  SNDSYS_BANKS_BLOCK;
  for (auto &bank : all_banks)
  {
    if (failed_banks.test(bank.id))
      failed_banks_names.push_back(bank.path);
  }
}

void get_loaded_banks_names(eastl::vector<eastl::string, framemem_allocator> &banks_names)
{
  SNDSYS_BANKS_BLOCK;
  for (auto &bank : all_banks)
  {
    if (loaded_banks.test(bank.id))
      banks_names.push_back(bank.path);
  }
}

void get_mod_banks_names(eastl::vector<eastl::string, framemem_allocator> &banks_names)
{
  SNDSYS_BANKS_BLOCK;
  for (auto &bank : all_banks)
  {
    if (bank.isMod && (bank.path.find("<") == FrameStr::npos))
      banks_names.push_back(bank.path);
  }
}

void set_preset_loaded_cb(PresetLoadedCallback cb)
{
  SNDSYS_BANKS_BLOCK;
  preset_loaded_cb = cb;
}

void set_err_cb(ErrorCallback cb)
{
  SNDSYS_BANKS_BLOCK;
  err_cb = cb;
}

void unload_banks_sample_data()
{
  SNDSYS_BANKS_BLOCK;
  for (auto &bank : all_banks)
  {
    if (!is_preload(bank.id) && bank.fmodBank)
    {
      int count = 0;
      bank.fmodBank->getEventCount(&count);
      if (count > 0)
      {
        eastl::vector<FMOD::Studio::EventDescription *, framemem_allocator> eventList;
        eventList.resize(count);
        bank.fmodBank->getEventList(eventList.begin(), count, &count);
        for (FMOD::Studio::EventDescription *desc : eventList)
          desc->unloadSampleData();
      }
    }
  }
}

bool any_banks_pending()
{
  SNDSYS_BANKS_BLOCK;
  return pending_banks.any();
}

} // namespace sndsys::banks
