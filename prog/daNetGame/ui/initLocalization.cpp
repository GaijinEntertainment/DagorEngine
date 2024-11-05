// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_localization.h>
#include "main/vromfs.h"
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_findFiles.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>
#include <util/dag_hash.h>
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

void dng_load_localization()
{
  const char *curLang = get_current_language();
  uint32_t langHash = str_hash_fnv1(curLang);
  bool ver_checked = false;

  dag::Vector<TmpVrom, framemem_allocator> tmpVroms;

  const DataBlock *langBlk = dgs_get_settings()->getBlockByNameEx("localizationVroms");
  const int tmpVromsCount = langBlk->blockCount();
  tmpVroms.resize(tmpVromsCount);
  for (int i = 0; i < tmpVromsCount; ++i)
  {
    const char *vromPath = langBlk->getBlock(i)->getStr("vrom");
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
    else if (!langBlk->getBlock(i)->getBool("optional", i > 0))
      DAG_FATAL("missing (or invalid) mandatory lang vrom: %s", vromPath);
  }

  Tab<SimpleString> csvFiles;
  for (int i = 0; i < tmpVromsCount; ++i)
    find_files_in_folder(csvFiles, langBlk->getBlock(i)->getStr("path"), ".csv", true, true, true);

  DataBlock *locBlk = &get_localization_blk();
  locBlk->clearData();
  locBlk->setStr("english_us", "English");
  locBlk->setStr("default_audio", "English");
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
