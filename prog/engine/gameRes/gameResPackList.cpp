// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_gameResSystem.h>
#include <3d/dag_texPackMgr2.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_fileMd5Validate.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include "grpData.h"
#include <perfMon/dag_perfTimer.h>

#define debug(...) logmessage(_MAKE4C('GRSS'), __VA_ARGS__)

namespace gameresprivate
{
extern int curRelFnOfs;
extern bool gameResPatchInProgress;
extern Tab<int> patchedGameResIds;

void scanGameResPack(const char *filename);
void scanDdsxTexPack(const char *filename);
void registerOptionalGameResPack(const char *filename);
void registerOptionalDdsxTexPack(const char *filename);
void dumpOptionalMappigs(const char *pkg_name);
} // namespace gameresprivate
namespace gamereshooks
{
extern void (*on_load_res_packs_from_list_complete)(const char *pack_list_blk_fname, bool load_grp, bool load_tex);
}

void load_res_packs_from_list(const char *pack_list_blk_fname, bool load_grp, bool load_tex, const char *res_base_dir)
{
  DataBlock blk;
  if (!blk.load(pack_list_blk_fname))
  {
    logerr("cannot load res pack list: %s", pack_list_blk_fname);
    return;
  }
  load_res_packs_from_list_blk(blk, pack_list_blk_fname, load_grp, load_tex, res_base_dir);
}

void load_res_packs_from_list_blk(const DataBlock &blk, const char *pack_list_blk_fname, bool load_grp, bool load_tex,
  const char *res_base_dir)
{
  bool prev_patchMode = gameresprivate::gameResPatchInProgress;
  bool opt_res_per_file = blk.getBool("perFileOptRes", false);
  bool opt_tex_per_file = blk.getBool("perFileOptTex", false);
  if (blk.getBool("patchMode", false))
    gameresprivate::gameResPatchInProgress = true;

  char buf[512];
  if (res_base_dir)
    strcpy(buf, res_base_dir);
  else
    dd_get_fname_location(buf, pack_list_blk_fname);

  dd_simplify_fname_c(buf);
  if (!buf[0])
    strcpy(buf, "./");
  dd_append_slash_c(buf);

  char tempBuf[DAGOR_MAX_PATH];
  int64_t reft = profile_ref_ticks();

  const DataBlock *b;
  int nid = blk.getNameId("pack");
  b = blk.getBlockByNameEx("ddsxTexPacks");
  gameresprivate::curRelFnOfs = i_strlen(buf);
  int num = 0;
  for (int i = 0; load_tex && i < b->paramCount(); i++)
    if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
    {
      _snprintf(tempBuf, sizeof(tempBuf), "%s%s", buf, b->getStr(i));
      tempBuf[sizeof(tempBuf) - 1] = 0;
      bool tq_pack = i == 0 && strstr(b->getStr(i), "__tq_pack.dxp");
      if (opt_tex_per_file && !tq_pack && !dd_file_exists(tempBuf)) // texture packs may be optional except for TQ pack
      {
        strcat(tempBuf, "cache.bin");
        gameresprivate::registerOptionalDdsxTexPack(tempBuf);
        continue;
      }
      gameresprivate::scanDdsxTexPack(tempBuf);
      num++;
    }

  debug("loadDdsxTexPack(%s) of %d packs done for %.4f sec", res_base_dir, num, profile_time_usec(reft) / 1e6);
  reft = profile_ref_ticks();

  b = blk.getBlockByNameEx("gameResPacks");
  num = 0;
  for (int i = 0; load_grp && i < b->paramCount(); i++)
    if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
    {
      _snprintf(tempBuf, sizeof(tempBuf), "%s%s", buf, b->getStr(i));
      tempBuf[sizeof(tempBuf) - 1] = 0;

      gameresprivate::scanGameResPack(tempBuf);
      num++;

      if (opt_res_per_file && !dd_file_exists(tempBuf))
      {
        strcat(tempBuf, "cache.bin");
        gameresprivate::registerOptionalGameResPack(tempBuf);
      }
    }
  gameresprivate::curRelFnOfs = 0;
#if !(_TARGET_PC && !_TARGET_STATIC_LIB)
  if (num)
    repack_real_game_res_id_table();
#endif
  debug("scanGameResPack(%s) of %d packs done for %.4f sec", res_base_dir, num, profile_time_usec(reft) / 1e6);
  if (load_tex)
    ddsx::dump_registered_tex_distribution();
  if (gamereshooks::on_load_res_packs_from_list_complete)
    gamereshooks::on_load_res_packs_from_list_complete(pack_list_blk_fname, load_grp, load_tex);
  gameresprivate::gameResPatchInProgress = prev_patchMode;
  if (opt_res_per_file || opt_tex_per_file)
    gameresprivate::dumpOptionalMappigs(res_base_dir);
}

bool load_res_packs_patch(const char *patch_pack_list_blk_fn, const char *base_grp_vromfs_fn, bool load_grp, bool load_tex)
{
  DataBlock blk;
  if (!blk.load(patch_pack_list_blk_fn))
  {
    logerr("cannot load res pack list: %s", patch_pack_list_blk_fn);
    return false;
  }
  const char *md5 = blk.getStr("base_grpHdr_md5", NULL);
  if (!md5 || !*md5)
  {
    logerr("cannot validate base vrom for patch: %s", patch_pack_list_blk_fn);
    return false;
  }

  const bool silent = ::dgs_get_settings()->getBlockByNameEx("debug")->getBool("silentPatchValidation", false);
  const char *errPrefix = !silent ? "validation failed for base vrom for patch: " : nullptr;
  if (!validate_file_md5_hash(base_grp_vromfs_fn, md5, errPrefix))
    return false;

  debug("loading gameres patch: %s", patch_pack_list_blk_fn);
  gameresprivate::gameResPatchInProgress = true;
  load_res_packs_from_list(patch_pack_list_blk_fn, load_grp, load_tex, NULL);
  gameresprivate::gameResPatchInProgress = false;
  clear_and_shrink(gameresprivate::patchedGameResIds);
  return true;
}
