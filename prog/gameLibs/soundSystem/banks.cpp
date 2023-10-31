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
#include "internal/pathHash.h"
#include "internal/visualLabels.h"
#include "internal/debug.h"
#if _TARGET_ANDROID || _TARGET_IOS
#include <crashlytics/firebase_crashlytics.h>
#endif

namespace sndsys::banks
{
static WinCritSec g_banks_cs;
#define SNDSYS_BANKS_BLOCK WinAutoLock banksLock(g_banks_cs);

static const size_t max_banks = 512;
using Bitset = eastl::bitset<max_banks>;

static bool g_report_bank_loading_time = false;

struct Bank
{
  FMOD::Studio::Bank *fmodBank = nullptr;
  const eastl::string path;
  const uint32_t id = 0;

  const bool isAsync = false;
  const bool isPreload = false;
  const bool isLoadToMemory = false;
  const bool isOptional = false;
  const bool isMod = false;

  Bank() = delete;
  Bank(const char *path_, uint32_t id_, bool is_async, bool is_preload, bool is_load_to_memory, bool is_optional, bool is_mod) :
    path(path_),
    id(id_),
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
static bool is_inited = false;

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

static inline bool is_async(uint32_t bank_id) { return all_banks[bank_id].isAsync; }
static inline bool is_preload(uint32_t bank_id) { return all_banks[bank_id].isPreload; }

static inline bool is_loaded_or_pending(uint32_t bank_id) { return loaded_banks[bank_id] || pending_banks[bank_id]; }
static inline bool is_failed(uint32_t bank_id) { return failed_banks[bank_id]; }

static inline void replace(FrameStr &path, const char *what, const char *with)
{
  eastl_size_t idx = path.find(what);
  if (idx != FrameStr::npos)
    path.replace(idx, strlen(what), with);
}
static inline void make_localized(FrameStr &str) { replace(str, "<loc>", locale.c_str()); }

static inline FMOD_RESULT load_bank_memory(const char *filename, FMOD_STUDIO_LOAD_BANK_FLAGS flags, FMOD::Studio::Bank **fmod_bank)
{
  file_ptr_t fp = df_open(filename, DF_READ);
  if (!fp)
    return FMOD_ERR_FILE_NOTFOUND;
  intptr_t filesize = df_length(fp);
  G_ASSERT_AND_DO(filesize >= 0, {
    df_close(fp);
    return FMOD_ERR_FILE_BAD;
  });

  const intptr_t alignment = 32; // FMOD required
  eastl::vector<char> data;
  data.resize(filesize + alignment);
  G_ASSERT_AND_DO(data.size() == filesize + alignment, {
    df_close(fp);
    return FMOD_ERR_MEMORY;
  });

  char *ptrAligned = (char *)(((size_t)data.begin() + (alignment - 1)) & -alignment);
  G_ASSERT_AND_DO(ptrAligned + filesize <= data.end(), {
    df_close(fp);
    return FMOD_ERR_MEMORY;
  });

  intptr_t sizeRead = df_read(fp, ptrAligned, filesize);
  df_close(fp);
  if (sizeRead != filesize)
    return FMOD_ERR_FILE_BAD;
  return fmodapi::get_studio_system()->loadBankMemory(ptrAligned, filesize, FMOD_STUDIO_LOAD_MEMORY, flags, fmod_bank);
}

static inline void load_bank(Bank &bank, const PathTags &path_tags)
{
  G_ASSERT_RETURN(!is_loaded_or_pending(bank.id), );
  G_ASSERT_RETURN(!is_failed(bank.id), );
  G_ASSERT_RETURN(bank.fmodBank == nullptr, );

  FrameStr taggedPath = bank.path.c_str();
  for (auto &tag : path_tags)
    replace(taggedPath, tag.first, tag.second);

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
      debug_trace_info("load bank \"%s\" in %d ms", realPath, now - start);
    }
  }
}

static inline void unload_bank(Bank &bank, eastl::vector<FMOD::Studio::Bank *, framemem_allocator> &fmod_banks)
{
  if (is_loaded_or_pending(bank.id))
  {
    debug_trace_info("unload bank \"%s\"", bank.path.c_str());
    if (bank.fmodBank != nullptr)
      fmod_banks.push_back(bank.fmodBank);
    bank.fmodBank = nullptr;
    pending_banks.set(bank.id, false);
    loaded_banks.set(bank.id, false);
  }
  failed_banks.set(bank.id, false);
}

static inline void append_bank(const char *path, bool is_async, bool is_preload, bool is_load_to_memory, bool is_optional, bool is_mod,
  Preset &preset)
{
  auto pred = [path](const Bank &bank) { return bank.path == path; };
  auto it = eastl::find_if(all_banks.begin(), all_banks.end(), pred);
  uint32_t bankId = 0;
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
    all_banks.emplace_back(path, bankId, is_async, is_preload, is_load_to_memory, is_optional, is_mod);
    G_ASSERT(all_banks.size() <= max_banks);
  }
  preset.banks.set(bankId);
}

static inline bool is_preset_loaded(const Preset &preset)
{
  return preset.banks.any() && preset.banks == (preset.banks & (loaded_banks | failed_banks));
}

static inline void update_presets()
{
  invalidate_visual_labels();
  for (auto &preset : all_presets)
  {
    const bool isLoaded = is_preset_loaded(preset);
    if (preset.isLoaded != isLoaded)
    {
      preset.isLoaded = isLoaded;
      debug_trace_info("%s preset \"%s\"", isLoaded ? "loaded" : "unloaded", preset.name.c_str());
      if (preset_loaded_cb)
        preset_loaded_cb(preset.hash, isLoaded);
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
    const char *language = ::dgs_get_settings()->getStr("language", "English");
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

#if _TARGET_PC_WIN
#define PLUGIN_SUBSYSTEM "win32"
#elif (_TARGET_64BIT && _TARGET_PC_WIN) | _TARGET_XBOX
#define PLUGIN_SUBSYSTEM "win64"
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

void update()
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
    update_presets();
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

static HashedKeySet<guid_hash_t> g_guid_prohibited;

static void add_bank(const char *name, const DataBlock &blk, const char *banks_folder, const char *extension, bool enable_mod,
  Preset &preset)
{
  const char *overridedPath = blk.getStr("path", nullptr);

  bool isMod = false;

  FrameStr path;

  if (enable_mod)
  {
    if (overridedPath)
      path.sprintf("%s/mod/%s%s", overridedPath, name, extension);
    else if (banks_folder && *banks_folder)
      path.sprintf("%s/mod/%s%s", banks_folder, name, extension);
    else
      path.sprintf("mod/%s%s", name, extension);

    make_localized(path);

    isMod = df_get_real_name(path.c_str()) != nullptr;
  }

  if (!isMod)
  {
    if (overridedPath)
      path.sprintf("%s/%s%s", overridedPath, name, extension);
    else if (banks_folder && *banks_folder)
      path.sprintf("%s/%s%s", banks_folder, name, extension);
    else
      path.sprintf("%s%s", name, extension);

    make_localized(path);
  }

  append_bank(path.c_str(), blk.getBool("async", false), blk.getBool("preload", false), blk.getBool("loadToMemory", false),
    blk.getBool("optional", false), isMod, preset);
};

void init(const DataBlock &blk)
{
  G_ASSERT_RETURN(sndsys::is_inited(), );
  SNDSYS_BANKS_BLOCK;
  G_ASSERT_RETURN(!is_inited, );

  const DataBlock &banksBlk = *blk.getBlockByNameEx("banks");
  const DataBlock &modBlk = *blk.getBlockByNameEx("mod");

  init_locale(blk, banksBlk);
  debug_trace_log("locale is \"%s\"", locale.c_str());

  const char *folder = banksBlk.getStr("folder", "sound");
  const char *extension = banksBlk.getStr("extension", ".bank");

  const bool enableMod = blk.getBool("enableMod", false) && modBlk.getBool("allow", true);

  g_report_bank_loading_time = blk.getBool("reportBankLoadingTime", false);

  FrameStr name, path;
  const DataBlock *presetsBlk = banksBlk.getBlockByNameEx("presets");
  all_presets.reserve(presetsBlk->blockCount());
  for (int j = 0; j < presetsBlk->blockCount(); ++j)
  {
    const DataBlock *presetBlk = presetsBlk->getBlock(j);
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

  is_inited = true;
}

static constexpr const char *g_default_preset_name = "master";
const char *get_default_preset(const DataBlock &blk)
{
  const DataBlock &banksBlk = *blk.getBlockByNameEx("banks");
  const DataBlock &presetsBlk = *banksBlk.getBlockByNameEx("presets", banksBlk.getBlockByNameEx("games"));

  const char *preset = banksBlk.getStr("preset", g_default_preset_name);
  if (presetsBlk.blockExists(preset))
    return preset;

  G_ASSERTF(!preset || !*preset || !banksBlk.paramExists("preset"),
    "Missing block sound{presets{%s{}}} in '%s'; parameter preset:t=\"%s\" is not valid", preset, blk.resolveFilename(), preset);
  return "";
}

static void enable_preset_impl(Preset &preset, bool enable, const PathTags &path_tags)
{
  if (preset.isEnabled == enable)
    return;

#if _TARGET_ANDROID || _TARGET_IOS
  crashlytics::AppState appState("snd_banks_load");
#endif

  invalidate_attributes_cache();

  const int32_t start = get_time_msec();
  preset.isEnabled = enable;

  eastl::vector<FMOD::Studio::Bank *, framemem_allocator> banksForUnload;
  eastl::vector<uint32_t, framemem_allocator> banksToLoad;

  for (auto &bank : all_banks)
  {
    if (need_load(bank.id))
    {
      if (!is_loaded_or_pending(bank.id) && !is_failed(bank.id))
        banksToLoad.push_back(bank.id);
    }
    else
      unload_bank(bank, banksForUnload);
  }

  for (Plugin &plugin : all_plugins)
    load_plugin(plugin);

  for (uint32_t bankId : banksToLoad)
    load_bank(all_banks[bankId], path_tags);

  const int32_t now = get_time_msec();
  debug_trace_log("preset \"%s\" enabled in %d ms", preset.name.c_str(), now - start);

  update_presets();

  for (auto bank : banksForUnload)
    SOUND_VERIFY(bank->unload());
}

void enable(const char *preset_name, bool enable, const PathTags &path_tags)
{
  SNDSYS_BANKS_BLOCK;
  if (Preset *preset = find_preset(preset_name))
    enable_preset_impl(*preset, enable, path_tags);
  else
  {
    logerr("Preset \"%s\" not found, cannot load specified sound banks", preset_name);
  }
}

void enable_starting_with(const char *preset_name_starts_with, bool enable, const PathTags &path_tags)
{
  if (!preset_name_starts_with || !*preset_name_starts_with)
    return;
  SNDSYS_BANKS_BLOCK;
  const size_t len = strlen(preset_name_starts_with);
  for (Preset &preset : all_presets)
    if (strncmp(preset.name.c_str(), preset_name_starts_with, len) == 0)
      enable_preset_impl(preset, enable, path_tags);
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
  const Preset *preset = find_preset(preset_name);
  return preset && (preset->banks & failed_banks).any();
}

void get_failed_banks_names(eastl::vector<eastl::string, framemem_allocator> &failed_banks_names)
{
  for (auto &bank : all_banks)
  {
    if (failed_banks.test(bank.id))
      failed_banks_names.push_back(bank.path);
  }
}

void get_loaded_banks_names(eastl::vector<eastl::string, framemem_allocator> &banks_names)
{
  for (auto &bank : all_banks)
  {
    if (loaded_banks.test(bank.id))
      banks_names.push_back(bank.path);
  }
}

void add_guid_to_prohibited(const FMODGUID &event_id) { g_guid_prohibited.insert(hash_fun(event_id)); }

bool is_guid_prohibited(const FMODGUID &event_id)
{
  return g_guid_prohibited.empty() ? false : g_guid_prohibited.has_key(hash_fun(event_id));
}
void clear_prohibited_guids() { g_guid_prohibited.clear(); }

void prohibit_bank_events(const eastl::string &bank_name)
{
  auto pred = [bank_name](const Bank &bank) { return bank.path == bank_name; };
  auto it = eastl::find_if(all_banks.begin(), all_banks.end(), pred);
  if (it != all_banks.end())
  {
    int count = 0;
    it->fmodBank->getEventCount(&count);
    if (count > 0)
    {
      eastl::vector<FMOD::Studio::EventDescription *, framemem_allocator> eventList;
      eventList.resize(count);
      it->fmodBank->getEventList(eventList.begin(), count, &count);
      for (FMOD::Studio::EventDescription *desc : eventList)
      {
        FMODGUID guid;
        if (FMOD_OK == desc->getID(reinterpret_cast<FMOD_GUID *>(&guid)))
          add_guid_to_prohibited(guid);
      }
    }
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

bool any_banks_pending() { return pending_banks.any(); }

bool are_inited() { return is_inited; }

} // namespace sndsys::banks
