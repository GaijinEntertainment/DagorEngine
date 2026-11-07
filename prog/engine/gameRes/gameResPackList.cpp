// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_gameResHooks.h>
#include <3d/dag_texPackMgr2.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/basePath.h>
#include <util/dag_fileMd5Validate.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include "grpData.h"
#include <perfMon/dag_perfTimer.h>
#if _TARGET_PC_WIN
#include <direct.h>
#elif _TARGET_PC
#include <unistd.h>
#endif

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

// the hook can find a missing pack elsewhere; then fn takes that name
static bool is_pack_missing(char (&fn)[DAGOR_MAX_PATH])
{
  if (dd_file_exists(fn))
    return false;
  String resolved;
  if (!gamereshooks::on_missing_pack_resolve || !gamereshooks::on_missing_pack_resolve(fn, resolved))
    return true;
  strncpy(fn, resolved, DAGOR_MAX_PATH - 1);
  fn[DAGOR_MAX_PATH - 1] = 0;
  return false;
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
  gameresprivate::curRelFnOfs = (int)strlen(buf);
  int num_dxp = 0;
  for (int i = 0; load_tex && i < b->paramCount(); i++)
    if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
    {
      _snprintf(tempBuf, sizeof(tempBuf), "%s%s", buf, b->getStr(i));
      tempBuf[sizeof(tempBuf) - 1] = 0;
      if (gamereshooks::on_gameres_pack_load_confirm && !gamereshooks::on_gameres_pack_load_confirm(tempBuf, true))
        continue;
      bool tq_pack = i == 0 && strstr(b->getStr(i), "__tq_pack.dxp");
      // texture packs may be optional except for TQ pack
      const bool may_be_optional = opt_tex_per_file && !tq_pack;
      if ((may_be_optional || gamereshooks::on_missing_pack_resolve) && is_pack_missing(tempBuf) && may_be_optional)
      {
        strcat(tempBuf, "cache.bin");
        gameresprivate::registerOptionalDdsxTexPack(tempBuf);
        continue;
      }
      gameresprivate::scanDdsxTexPack(tempBuf);
      num_dxp++;
    }

  if (num_dxp)
    debug("loadDdsxTexPack(%s) of %d packs done for %.4f sec", res_base_dir, num_dxp, profile_time_usec(reft) / 1e6);
  reft = profile_ref_ticks();

  b = blk.getBlockByNameEx("gameResPacks");
  int num_grp = 0;
  for (int i = 0; load_grp && i < b->paramCount(); i++)
    if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
    {
      _snprintf(tempBuf, sizeof(tempBuf), "%s%s", buf, b->getStr(i));
      tempBuf[sizeof(tempBuf) - 1] = 0;
      if (gamereshooks::on_gameres_pack_load_confirm && !gamereshooks::on_gameres_pack_load_confirm(tempBuf, false))
        continue;

      // checked before the scan, which registers the pack by this name (from its header in cache.bin when the file is missing)
      const bool optional =
        (opt_res_per_file || gamereshooks::on_missing_pack_resolve) && is_pack_missing(tempBuf) && opt_res_per_file;
      gameresprivate::scanGameResPack(tempBuf);
      num_grp++;

      if (optional)
      {
        strcat(tempBuf, "cache.bin");
        gameresprivate::registerOptionalGameResPack(tempBuf);
      }
    }
  gameresprivate::curRelFnOfs = 0;
#if !(_TARGET_PC && !_TARGET_STATIC_LIB)
  if (num_grp)
    repack_real_game_res_id_table();
#endif
  if (num_grp)
    debug("scanGameResPack(%s) of %d packs done for %.4f sec", res_base_dir, num_grp, profile_time_usec(reft) / 1e6);
  if (load_tex && num_dxp)
    ddsx::dump_registered_tex_distribution();
  if (gamereshooks::on_load_res_packs_from_list_complete)
    gamereshooks::on_load_res_packs_from_list_complete(pack_list_blk_fname, load_grp, load_tex);
  gameresprivate::gameResPatchInProgress = prev_patchMode;
  if (opt_res_per_file || opt_tex_per_file)
    gameresprivate::dumpOptionalMappigs(res_base_dir);
}

String resolve_res_base_dir(const char *dir)
{
  if (is_path_abs(dir))
    return String::mk_str_cat(dir, "/");
  String path;
  int idx = 0;
  while (const char *base = df_next_base_path(&idx))
  {
    path.setStrCat(base, dir);
#if _TARGET_PC
    char cwd[DAGOR_MAX_PATH];
    if (!is_path_abs(path) && getcwd(cwd, sizeof(cwd)))
      path.setStrCat4(cwd, "/", base, dir);
#endif
    simplify_fname(path);
    path += "/";
    if (is_path_abs(path) && dd_file_exists(path + "grp_hdr.vromfs.bin"))
      return path;
  }
  return String::mk_str_cat(dir, "/");
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
