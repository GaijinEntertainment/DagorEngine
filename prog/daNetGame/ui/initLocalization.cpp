// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_localization.h>
#include <quirrel/bindQuirrelEx/autoBind.h>
#include "main/vromfs.h"
#include "ui/overlay.h"
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_findFiles.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>
#include <util/dag_hash.h>
#include <util/dag_console.h>
#include <util/dag_delayedAction.h>
#include <memory/dag_framemem.h>
#include <debug/dag_debug.h>

static DataBlock localization_blk;
DataBlock &get_localization_blk() { return localization_blk; }

namespace
{
struct LoadedLoc
{
  uint32_t lang = 0;
  uint32_t gameVromVer = 0;
  void clear()
  {
    lang = 0;
    gameVromVer = 0;
  }
};
static LoadedLoc loaded_loc;
} // namespace

struct VromDeleter
{
  void operator()(VirtualRomFsData *vrom) const
  {
    if (vrom)
    {
      remove_vromfs(vrom);
      framemem_ptr()->free(vrom);
    }
  }
};

using TmpVrom = eastl::unique_ptr<VirtualRomFsData, VromDeleter>;

#if DAGOR_DBGLEVEL > 0 && (_TARGET_PC || _TARGET_C3)
#define LOC_SRC_FOLDER_SUPPORTED 1
#else
#define LOC_SRC_FOLDER_SUPPORTED 0
#endif

#if LOC_SRC_FOLDER_SUPPORTED
static bool is_base_path_registered(const char *path)
{
  int idx = 0;
  while (const char *p = df_next_base_path(&idx))
    if (dd_stricmp(p, path) == 0)
      return true;
  return false;
}

static bool use_loc_src_folder(const DataBlock &loc_blk)
{
  const char *src = loc_blk.getStr("src", nullptr);
  if (!src || !*src || !dgs_get_settings()->getBlockByNameEx("debug")->getBool("useAddonVromSrc", false))
    return false;
  String base(0, "%s/", src);
  dd_simplify_fname_c(base);
  const char *path = loc_blk.getStr("path");
  Tab<SimpleString> csvFiles(framemem_ptr());
  if (!find_files_in_folder(csvFiles, String(0, "%s%s", base.str(), path), ".csv", false, true, true))
  {
    logwarn("[LOC] no csv in lang source folder %s%s, using vrom %s", base.str(), path, loc_blk.getStr("vrom"));
    return false;
  }
  if (!is_base_path_registered(base) && !dd_add_base_path(base))
  {
    logerr("[LOC] failed to add base path %s", base.str());
    return false;
  }
  debug("[LOC] use lang source folder: %s, cause useAddonVromSrc specified", base.str());
  return true;
}
#endif

static void remove_underscore_csv(Tab<SimpleString> &csv_files)
{
  for (int i = csv_files.size() - 1; i >= 0; --i)
    if (dd_get_fname(csv_files[i])[0] == '_')
      erase_items(csv_files, i, 1);
}

static const char *install_startup_language_override()
{
  const char *forced = get_force_language();
  if (!forced || !*forced)
    return get_current_language();

  static eastl::string startupLang;
  if (!startupLang.empty())
    return startupLang.c_str();

  const DataBlock *locBlk = dgs_get_settings()->getBlockByNameEx("localization");
  const DataBlock *limits = locBlk->getBlockByNameEx("full_translation");
  if (!limits->getBlockByName(get_platform_string_id()))
    startupLang = forced;
  else
    startupLang = get_default_lang(*locBlk);
  if (startupLang != forced)
    set_default_lang_override(startupLang.c_str());
  return startupLang.c_str();
}

void dng_load_localization()
{
  const char *curLang = install_startup_language_override();
  uint32_t langHash = str_hash_fnv1(curLang);
  bool ver_checked = false;

  dag::Vector<TmpVrom, framemem_allocator> tmpVroms;

  const DataBlock *langBlk = dgs_get_settings()->getBlockByNameEx("localizationVroms");
  const int tmpVromsCount = langBlk->blockCount();
  tmpVroms.resize(tmpVromsCount);
  for (int i = 0; i < tmpVromsCount; ++i)
  {
    const DataBlock &loc = *langBlk->getBlock(i);
#if LOC_SRC_FOLDER_SUPPORTED
    if (use_loc_src_folder(loc))
      continue;
#endif
    const char *vromPath = loc.getStr("vrom");
    if (!ver_checked)
    {
      uint32_t game_ver = get_vromfs_dump_version(get_eff_vrom_fpath(vromPath));
      if (loaded_loc.lang == langHash && loaded_loc.gameVromVer == game_ver)
        return;
      loaded_loc.lang = langHash;
      loaded_loc.gameVromVer = game_ver;
      ver_checked = true;
    }
    tmpVroms[i].reset(load_vromfs_dump(get_eff_vrom_fpath(vromPath), framemem_ptr(), NULL, NULL, DF_IGNORE_MISSING));
    if (tmpVroms[i])
      add_vromfs(tmpVroms[i].get());
    else if (!loc.getBool("optional", i > 0))
      DAG_FATAL("missing (or invalid) mandatory lang vrom: %s", vromPath);
  }

  Tab<SimpleString> csvFiles;
  for (int i = 0; i < tmpVromsCount; ++i)
    find_files_in_folder(csvFiles, langBlk->getBlock(i)->getStr("path"), ".csv", true, true, true);
  remove_underscore_csv(csvFiles);

  DataBlock *locBlk = &get_localization_blk();
  locBlk->clearData();
  locBlk->setStr("english_us", "English");
  locBlk->setStr("default_lang", "English");
  DataBlock *b = locBlk->addBlock("locTable");
  for (int i = 0; i < csvFiles.size(); i++)
    b->addStr("file", csvFiles[i]);

  G_ASSERT(locBlk && locBlk->isValid());
  debug("currentLanguage: %s", curLang);
  startup_localization_V2(*locBlk, curLang);

  if (!ver_checked)
    loaded_loc.gameVromVer = 0;
  debug("localization loaded: lang=\"%s\"  vrom.version=0x%X", curLang, loaded_loc.gameVromVer);
}

void unload_localization()
{
  loaded_loc.clear();
  shutdown_localization();
}


static void sq_reinit_localization(const char *lang)
{
  const char *curLang = lang && *lang ? lang : get_current_language();
  uint32_t langHash = str_hash_fnv1(curLang);
  debug("reiniting localization for %s (%s)", curLang, get_current_language());
  set_language_to_settings(curLang);

  shutdown_localization();

  dag::Vector<TmpVrom, framemem_allocator> tmpVroms;

  const DataBlock *langBlk = dgs_get_settings()->getBlockByNameEx("localizationVroms");
  const int tmpVromsCount = langBlk->blockCount();
  tmpVroms.resize(tmpVromsCount);
  for (int i = 0; i < tmpVromsCount; ++i)
  {
    const DataBlock &loc = *langBlk->getBlock(i);
#if LOC_SRC_FOLDER_SUPPORTED
    if (use_loc_src_folder(loc))
      continue;
#endif
    const char *vromPath = loc.getStr("vrom");
    tmpVroms[i].reset(load_vromfs_dump(get_eff_vrom_fpath(vromPath), framemem_ptr(), NULL, NULL, DF_IGNORE_MISSING));
    if (tmpVroms[i])
      add_vromfs(tmpVroms[i].get());
  }

  if (startup_localization_V2(get_localization_blk(), curLang))
  {
    loaded_loc.lang = langHash;
    debug("localization reinited (%s)", get_current_language());
  }
  else
    logerr("Failed to init localization - see log for details");
}

static bool localization_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("localization", "reinit", 1, 2)
  {
    sq_reinit_localization(argc > 1 ? argv[1] : nullptr);
    delayed_call([]() { overlay_ui::reload_scripts(true); });
  }
  return found;
}
REGISTER_CONSOLE_HANDLER(localization_console_handler);

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_localizations, "app", sq::VM_ALL)
{
  Sqrat::Table tbl(vm);

  tbl //
    .Func("reinit_localization", sq_reinit_localization)
    /**/;
  return tbl;
}
