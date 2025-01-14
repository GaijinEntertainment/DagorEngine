// Copyright (C) Gaijin Games KFT.  All rights reserved.

#ifndef _TARGET_DABUILD_STATIC
#include <startup/dag_dllMain.inc.cpp>
#endif
#include "daBuild.h"
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMsgPipe.h>
#include <assets/assetExporter.h>
#include <assets/assetExpCache.h>
#include <assets/assetHlp.h>
#include <assets/assetRefs.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/conLogWriter.h>
#include <libTools/util/progressInd.h>
#include <regExp/regExp.h>
#include <generic/dag_sort.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <ioSys/dag_findFiles.h>
#include "packList.h"
#include "jobSharedMem.h"
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_globalMutex.h>
#include <debug/dag_debug.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX
#include <unistd.h>
#endif

static String stripPrefix;
#define RELEASE_ASSETS_PACKS()   \
  clear_all_ptr_items(tex_pack); \
  clear_all_ptr_items(grp_pack)

static int cmp_res_pack_name(AssetPack *const *a, AssetPack *const *b) { return strcmp(a[0]->packName, b[0]->packName); }
static int cmp_tex_pack_name(AssetPack *const *a, AssetPack *const *b)
{
  bool a_hq = trail_stricmp(a[0]->packName, "-hq.dxp.bin");
  bool b_hq = trail_stricmp(b[0]->packName, "-hq.dxp.bin");
  if (!a_hq && b_hq)
    return -1;
  if (a_hq && !b_hq)
    return +1;
  return strcmp(a[0]->packName, b[0]->packName);
}
static int cmp_asset_name(DagorAsset *const *a, DagorAsset *const *b) { return strcmp(a[0]->getName(), b[0]->getName()); }

static int get_pack_id(Tab<AssetPack *> &packs, int pk_id, const char *pack_name, bool add_new = true)
{
  if (!pack_name)
    return -2;

  static String buf;
  buf = pack_name;
  simplify_fname(buf);
  dd_strlwr(buf);
  if (strnicmp(buf, stripPrefix, stripPrefix.length()) == 0)
    erase_items(buf, 0, stripPrefix.length());
  if (dabuild_build_tex_separate && buf.suffix(".dxp.bin"))
  {
    const_cast<char *>(buf.end(8))[0] = '\0';
    buf.updateSz();
  }

  for (int i = packs.size() - 1; i >= 0; i--)
    if (stricmp(buf, packs[i]->packName) == 0 && (packs[i]->packageId == pk_id || pk_id == -2))
      return i;
  if (!add_new)
    return -2;
  packs.push_back(new AssetPack(buf, pk_id));
  return packs.size() - 1;
}
static void makePack(DagorAssetMgr &mgr, dag::ConstSpan<DagorAsset *> assets, dag::ConstSpan<bool> exp_types_mask,
  const DataBlock &expblk, Tab<AssetPack *> &tex_pack, Tab<AssetPack *> &grp_pack, FastNameMapEx &addPackages, ILogWriter &log,
  bool tex, bool res, const char *target_str, const char *profile)
{
  Tab<GrpAndTexPackId> folder_map(tmpmem);
  folder_map.reserve(64);
  int tex_type = mgr.getTexAssetTypeId();
  String tmpFolderPath;
  int tex_pack_init_cnt = tex_pack.size(), grp_pack_init_cnt = grp_pack.size();

  stripPrefix = expblk.getStr("stripPrefix", "");
  simplify_fname(stripPrefix);
  if (stripPrefix.length() && stripPrefix[stripPrefix.length() - 1] != '/')
    stripPrefix += "/";

  debug("total: %d assets", assets.size());
  FastIntList exp_reject_types;
  if (profile)
    if (const DataBlock *b = expblk.getBlockByNameEx("profiles")->getBlockByName(profile))
      for (int nid = b->getNameId("reject_exp"), i = 0; i < b->paramCount(); i++)
        if (b->getParamNameId(i) == nid && b->getParamType(i) == b->TYPE_STRING)
        {
          int t = mgr.getAssetTypeId(b->getStr(i));
          if (t >= 0)
            exp_reject_types.addInt(t);
          else
            log.addMessage(log.ERROR, "bad reject_exp=\"%s\" in profile <%s>", b->getStr(i), profile);
        }

  for (int i = 0; i < assets.size(); i++)
  {
    int fidx = assets[i]->getFolderIndex();
    int a_type = assets[i]->getType();
    G_ASSERT(a_type >= 0 && a_type < exp_types_mask.size());
    if (!exp_types_mask[a_type])
      continue;
    if (exp_reject_types.hasInt(a_type))
      continue;

    if (!(mgr.getFolder(fidx).flags & DagorAssetFolder::FLG_EXPORT_ASSETS))
      continue;
    if (a_type == tex_type && !tex)
      continue;
    if (a_type != tex_type && !res)
      continue;

    if (a_type != tex_type)
    {
      IDagorAssetExporter *e = mgr.getAssetExporter(a_type);
      if (!e || !e->isExportableAsset(*assets[i]))
        continue;
    }

    // handle per-asset packing
    Tab<AssetPack *> &packs = (a_type == tex_type) ? tex_pack : grp_pack;
    const char *pkname = assets[i]->getCustomPackageName(target_str, profile);
    if (pkname && strcmp(pkname, "*") == 0)
      pkname = NULL;
    int pkid = pkname ? addPackages.addNameId(pkname) : -1;
    int id = get_pack_id(packs, pkid, assets[i]->getDestPackName(dabuild_collapse_packs));
    if (id >= 0)
      packs[id]->assets.push_back(assets[i]);
  }

  const DataBlock &pkgBlk = *expblk.getBlockByNameEx("packages");
  if (pkgBlk.paramCount() + pkgBlk.blockCount() > 0)
  {
    bool def = pkgBlk.getBool("defaultOn", true);
    bool main = pkgBlk.getBool("::main", profile ? pkgBlk.getBool(String(0, "::main.%s", profile), true) : true);

    for (int i = tex_pack.size() - 1; i >= tex_pack_init_cnt; i--)
    {
      if (tex_pack[i]->packageId < 0)
      {
        if (main)
          continue;
        else
          goto skip_exp_tex;
      }
      if (!pkgBlk.getBlockByNameEx(addPackages.getName(tex_pack[i]->packageId))->getBool(target_str, def))
      {
      skip_exp_tex:
        // debug("not exporting: %s %s", addPackages.getName(tex_pack[i]->packageId), tex_pack[i]->packName.str());
        delete tex_pack[i];
        erase_items(tex_pack, i, 1);
      }
    }
    for (int i = grp_pack.size() - 1; i >= grp_pack_init_cnt; i--)
    {
      if (grp_pack[i]->packageId < 0)
      {
        if (main)
          continue;
        else
          goto skip_exp_grp;
      }
      if (!pkgBlk.getBlockByNameEx(addPackages.getName(grp_pack[i]->packageId))->getBool(target_str, def))
      {
      skip_exp_grp:
        // debug("not exporting: %s %s", addPackages.getName(grp_pack[i]->packageId), grp_pack[i]->packName.str());
        delete grp_pack[i];
        erase_items(grp_pack, i, 1);
      }
    }
  }
}

void preparePacks(DagorAssetMgr &mgr, dag::ConstSpan<DagorAsset *> assets, dag::ConstSpan<bool> exp_types_mask,
  const DataBlock &expblk, Tab<AssetPack *> &tex_pack, Tab<AssetPack *> &grp_pack, FastNameMapEx &addPackages, ILogWriter &log,
  bool tex, bool res, const char *target_str, const char *profile)
{
  makePack(mgr, assets, exp_types_mask, expblk, tex_pack, grp_pack, addPackages, log, tex, res, target_str, profile);
}

bool isAssetExportable(DagorAssetMgr &mgr, DagorAsset *asset, dag::ConstSpan<bool> exp_types_mask)
{
  int tex_type = mgr.getTexAssetTypeId();

  int fidx = asset->getFolderIndex();
  int a_type = asset->getType();
  G_ASSERT(a_type >= 0 && a_type < exp_types_mask.size());
  if (!exp_types_mask[a_type] || !(mgr.getFolder(fidx).flags & DagorAssetFolder::FLG_EXPORT_ASSETS))
    return false;

  if (a_type != tex_type)
  {
    IDagorAssetExporter *e = mgr.getAssetExporter(a_type);
    if (!e || !e->isExportableAsset(*asset))
      return false;
  }

  return true;
}
bool get_exported_assets(Tab<DagorAsset *> &dest, dag::ConstSpan<DagorAsset *> src, const char *ts, const char *profile)
{
  dest.clear();
  dest.reserve(src.size());
  String exp_nm(32, "export_%s", ts);
  String exp_nm2;
  if (profile && *profile)
    exp_nm2.printf(32, "export_%s~%s", ts, profile);
  else
    exp_nm2 = exp_nm;

  for (int i = 0; i < src.size(); i++)
    if (src[i]->props.getBool("export", true) && src[i]->props.getBool(exp_nm2, src[i]->props.getBool(exp_nm, true)))
      dest.push_back(src[i]);
  return dest.size() > 0;
}

static inline size_t get_job_idx(DabuildJobPool::Ctx *ctx)
{
  return ctx - ((DabuildJobSharedMem *)AssetExportCache::getJobSharedMem())->jobCtx;
}

static void process_result(DabuildJobPool::Ctx *ctx, dag::ConstSpan<AssetPack *> tex_pack, dag::ConstSpan<AssetPack *> grp_pack)
{
  if (ctx->cmd == 2)
  {
    debug("  job j%02d: finished texpack %s(%d) res=%d  (%d/%d)", get_job_idx(ctx), ctx->data, ctx->pkgId, ctx->result, ctx->donePk,
      ctx->totalPk);
    tex_pack[ctx->packId]->buildRes = ctx->result;
    stat_tex_sz_diff += ctx->szDiff;
    stat_changed_tex_total_sz += ctx->szChangedTotal;
  }
  else if (ctx->cmd == 3)
  {
    debug("  job j%02d: finished respack %s(%d) res=%d  (%d/%d)", get_job_idx(ctx), ctx->data, ctx->pkgId, ctx->result, ctx->donePk,
      ctx->totalPk);
    grp_pack[ctx->packId]->buildRes = ctx->result;
    stat_grp_sz_diff += ctx->szDiff;
    stat_changed_grp_total_sz += ctx->szChangedTotal;
  }
}

static inline const char *read_str(const DataBlock *b, const char *target_str, const char *profile, const char *def)
{
  if (!b)
    return def;
  if (profile && *profile)
    return b->getStr(String(64, "%s~%s", target_str, profile), def);
  return b->getStr(target_str, def);
}

bool setup_dxp_grp_write_ver(const DataBlock &build_blk, ILogWriter &log)
{
  bool setup_ok = true;
  dabuild_dxp_write_ver = build_blk.getInt("writeDdsxTexPackVer", 3);
  dabuild_grp_write_ver = build_blk.getInt("writeGameResPackVer", 3);

  if (dabuild_dxp_write_ver != 2 && dabuild_dxp_write_ver != 3)
  {
    log.addMessage(log.ERROR, "invalid writeDdsxTexPackVer=%d", dabuild_dxp_write_ver);
    dabuild_dxp_write_ver = 3;
    setup_ok = false;
  }
  if (dabuild_grp_write_ver != 2 && dabuild_grp_write_ver != 3)
  {
    log.addMessage(log.ERROR, "invalid writeGameResPackVer=%d", dabuild_grp_write_ver);
    dabuild_grp_write_ver = 3;
    setup_ok = false;
  }
  return setup_ok;
}

static bool is_cache_outdated(AssetExportCache &gdc, const char *cache_fname, DagorAssetMgr &mgr, const char *pack_fname,
  ILogWriter &log, bool remove_outdated = true)
{
  bool cache_loaded = gdc.load(cache_fname, mgr);
  bool target_file_uptodate = cache_loaded && !gdc.checkTargetFileChanged(pack_fname);
  if (cache_loaded && target_file_uptodate)
    return false;

  if (remove_outdated && dd_file_exist(cache_fname))
  {
    String reason;
    if (!cache_loaded)
      reason = "broken file";
    else if (!dd_file_exist(pack_fname))
      reason.setStrCat("missing target file ", pack_fname);
    else
      reason.setStrCat("target file differs ", pack_fname);
    log.addMessage(log.WARNING, "removed outdated cache: %s (%s)", cache_fname, reason);
    unlink(cache_fname);
  }
  return true;
}

bool exportAssets(DagorAssetMgr &mgr, const char *app_dir, unsigned targetCode, const char *profile,
  dag::ConstSpan<const char *> packs_to_build, dag::ConstSpan<const char *> packs_re_src, dag::ConstSpan<bool> exp_types_mask,
  const DataBlock &appblk, IGenericProgressIndicator &pbar, ILogWriter &log, bool export_tex, bool export_res)
{
  clear_and_shrink(dabuild_progress_prefix_text);

  const DataBlock &expblk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");
  const DataBlock &build_blk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("build");
  dabuild_dxp_write_ver = build_blk.getInt("writeDdsxTexPackVer", 3);

  if (!setup_dxp_grp_write_ver(build_blk, log))
    return false;

  FastNameMapEx pkg_to_build;
#define SKIP_NON_TARGET_PACKAGES(PKGID, EXPFLG)                                                                                       \
  if (pkg_to_build.nameCount())                                                                                                       \
  {                                                                                                                                   \
    if (                                                                                                                              \
      ((PKGID) < 0 && pkg_to_build.getNameId("*") < 0) || ((PKGID) >= 0 && pkg_to_build.getNameId(addPackages.getName((PKGID))) < 0)) \
      continue;                                                                                                                       \
    else if (packs_to_build.size() == pkg_to_build.nameCount() && !packs_re_src.size())                                               \
    {                                                                                                                                 \
      EXPFLG = true;                                                                                                                  \
      continue;                                                                                                                       \
    }                                                                                                                                 \
  }

  for (int i = 0; i < packs_to_build.size(); i++)
    if (packs_to_build[i][0] == '\1')
      pkg_to_build.addNameId(&packs_to_build[i][1]);

  const char *addTexPfx = expblk.getStr("addTexPrefix", "");
  bool full_build = packs_re_src.size() == 0 && packs_to_build.size() == 0 && export_tex && export_res;

  Tab<AssetPack *> grp_pack(tmpmem), tex_pack(tmpmem);
  FastNameMapEx addPackages;
  char target_str[6];
  strcpy(target_str, mkbindump::get_target_str(targetCode));
  makePack(mgr, mgr.getAssets(), exp_types_mask, expblk, tex_pack, grp_pack, addPackages, log, export_tex, export_res, target_str,
    profile);

  AssetExportCache::setSdkRoot(app_dir, "develop");
  AssetExportCache gdc;
  PackListMgr plm;
  String cache_fname, pack_fname, pack_fname_prefix;

  const char *pack_list_fname = expblk.getStr("packlist", NULL);
  if (!assethlp::validate_exp_blk(addPackages.nameCount() > 0, expblk, target_str, profile, log))
  {
    RELEASE_ASSETS_PACKS();
    return false;
  }

  PackListMgr::PackOptions pack_opt;
  if (const DataBlock *profile_blk = expblk.getBlockByNameEx("profiles")->getBlockByName(profile && *profile ? profile : "*"))
  {
    pack_opt.perFileOptRes = profile_blk->getBool("perFileOptionalResPacks", pack_opt.perFileOptRes);
    pack_opt.perFileOptTex = profile_blk->getBool("perFileOptionalTexPacks", pack_opt.perFileOptTex);
    pack_opt.patchMode = profile_blk->getBool("patchMode", pack_opt.patchMode);
  }

  Tab<RegExp *> packs_re(tmpmem);
  for (int i = 0; i < packs_re_src.size(); i++)
  {
    RegExp *re = new RegExp;
    if (re->compile(packs_re_src[i], "i"))
      packs_re.push_back(re);
    else
    {
      log.addMessage(log.ERROR, "invalid regexp: %s", packs_re_src[i]);
      delete re;
    }
  }

  DataBlock out_blk;
  dabuild_prepare_out_blk(out_blk, mgr, build_blk);

  if (packs_to_build.size() || packs_re.size())
  {
    String tmp;
    String pack_fname_prefix;
    int cur_pkg = -2;
    for (int i = 0; i < grp_pack.size(); i++)
    {
      grp_pack[i]->exported = false;
      SKIP_NON_TARGET_PACKAGES(grp_pack[i]->packageId, grp_pack[i]->exported);
      if (cur_pkg != grp_pack[i]->packageId)
      {
        cur_pkg = grp_pack[i]->packageId;
        assethlp::build_package_pack_fname_prefix(pack_fname_prefix, expblk, cur_pkg < 0 ? nullptr : addPackages.getName(cur_pkg),
          app_dir, target_str, profile);
      }
      const char *pack_fn = grp_pack[i]->packName.str();
      if (!pack_fname_prefix.empty())
      {
        tmp.printf(128, "%s%s", pack_fname_prefix, pack_fn);
        pack_fn = tmp.str();
      }
      for (int j = 0; j < packs_re.size(); j++)
        if (packs_re[j]->test(pack_fn))
        {
          grp_pack[i]->exported = true;
          break;
        }
    }
    for (int i = 0; i < tex_pack.size(); i++)
    {
      tex_pack[i]->exported = false;
      SKIP_NON_TARGET_PACKAGES(tex_pack[i]->packageId, tex_pack[i]->exported);
      if (cur_pkg != tex_pack[i]->packageId)
      {
        cur_pkg = tex_pack[i]->packageId;
        assethlp::build_package_pack_fname_prefix(pack_fname_prefix, expblk, cur_pkg < 0 ? nullptr : addPackages.getName(cur_pkg),
          app_dir, target_str, profile);
      }
      const char *pack_fn = tex_pack[i]->packName.str();
      if (!pack_fname_prefix.empty())
      {
        tmp.printf(128, "%s%s", pack_fname_prefix, pack_fn);
        pack_fn = tmp.str();
      }
      for (int j = 0; j < packs_re.size(); j++)
        if (packs_re[j]->test(pack_fn))
        {
          tex_pack[i]->exported = true;
          break;
        }
    }

    for (int i = 0; i < packs_to_build.size(); i++)
    {
      if (packs_to_build[i][0] == '\1')
        continue;
      bool found = false, dummy = false;
      for (int j = -1; j < addPackages.nameCount(); j++)
      {
        SKIP_NON_TARGET_PACKAGES(j, dummy);

        int g_id = get_pack_id(grp_pack, j, packs_to_build[i], false);
        int t_id = get_pack_id(tex_pack, j, packs_to_build[i], false);

        if (g_id >= 0)
          grp_pack[g_id]->exported = true, found = true;
        if (t_id >= 0)
          tex_pack[t_id]->exported = true, found = true;
      }
      if (!found)
        log.addMessage(log.WARNING, "unknown pack: %s, skipped", packs_to_build[i]);
    }
  }
#undef SKIP_NON_TARGET_PACKAGES
  clear_all_ptr_items(packs_re);

  DabuildJobSharedMem *jobMem = (DabuildJobSharedMem *)AssetExportCache::getJobSharedMem();
  if (jobMem)
    debug("jobMem=%p, jobs=%d", jobMem, jobMem->jobCount);

  String cache_base(260, "%s/%s/", app_dir, expblk.getStr("cache", "/develop/.cache"));
  String dest_base;
  String tmp_str;
  String grpvromfs_fname;

  simplify_fname(cache_base);

  bool eraseBadPacks = expblk.getBool("eraseBadPacks", false);

  bool success = true;
  Tab<DagorAsset *> asset_list(tmpmem);
  Tab<DagorAsset *> ctex(tmpmem);
  const DataBlock &pkgBlk = *expblk.getBlockByNameEx("packages");

  for (int ti = 0; ti < 1; ti++) //-V1008
  {
    bool write_be = dagor_target_code_be(targetCode);
    if (!assethlp::validate_exp_blk(addPackages.nameCount() > 0, expblk, target_str, profile, log))
      continue;

    const char *grpvromfs_dest_fname = read_str(expblk.getBlockByNameEx("destGrpHdrCache"), target_str, profile, NULL);
    const char *grpvromfs_game_relpath = expblk.getBlockByNameEx("destGrpHdrCache")->getStr("gameRelatedPath", NULL);
    bool writeGrpHdrCache = !dabuild_build_tex_separate &&
                            expblk.getBlockByNameEx("writeGrpHdrCache")->getBool(target_str, expblk.getBool("writeGrpHdrCache", true));

    if (pack_list_fname && grpvromfs_dest_fname && strstr(pack_list_fname, "../"))
    {
      log.addMessage(log.ERROR, "bad packlist name: <%s>, incompatible with grpHdr-cache settings, target %s", pack_list_fname,
        target_str);
      continue;
    }
    if (pack_list_fname && !grpvromfs_dest_fname && addPackages.nameCount())
    {
      log.addMessage(log.ERROR, "cannot export additional packages - assets/export/destGrpHdrCache not set");
      continue;
    }

    int total_tpacks = 0, done_tpacks = 0;
    for (int i = 0; i < tex_pack.size(); i++)
      if (tex_pack[i]->exported)
        total_tpacks++;

    int total_rpacks = 0, done_rpacks = 0;
    for (int i = 0; i < grp_pack.size(); i++)
      if (grp_pack[i]->exported)
        total_rpacks++;

    // distributed textures build
    if (jobMem)
    {
      G_ASSERT(!dabuild_skip_any_build);
      DabuildJobPool::Ctx *ctx;
      DabuildJobPool jp(jobMem);

      if (export_tex && !dabuild_dry_run && !dabuild_strip_d3d_res)
      {
        log.addMessage(IMPORTANT_NOTE, "Updating textures cache (distributed using %d jobs)...", jp.jobs);
        int t0 = get_time_msec();
        static const int UPD_LOG_INTERVAL_MS = 180 * 1000;
        int next_log_upd_t = t0 + UPD_LOG_INTERVAL_MS;
        int tex_cnt = 0;
        int tex_checked_cnt = 0;
        int tex_to_build = 0, tex_total = 0;

        for (int i = 0; i < tex_pack.size(); i++)
          if (tex_pack[i]->exported)
          {
            if (!get_exported_assets(asset_list, tex_pack[i]->assets, target_str, profile))
              continue;

            int pkid = -1;
            if (dabuild_build_tex_separate)
              goto process_tex_assets;

            pkid = tex_pack[i]->packageId;
            assethlp::build_package_dest_strings(dest_base, pack_fname_prefix, expblk, pkid < 0 ? nullptr : addPackages.getName(pkid),
              app_dir, target_str, profile);
            make_cache_fname(cache_fname, cache_base, addPackages.getName(pkid), tex_pack[i]->packName, target_str, profile);

            simplify_fname(cache_fname);

            pack_fname.printf(260, "%s%s/%s%s", dest_base.str(), addTexPfx, pack_fname_prefix.str(), tex_pack[i]->packName.str());
            simplify_fname(pack_fname);

            if (is_cache_outdated(gdc, cache_fname, mgr, pack_fname, log))
              goto process_tex_assets;
            else if (gdc.isTimeChanged())
              gdc.save(cache_fname, mgr);

            if (!checkDdsxTexPackUpToDate(targetCode, profile, write_be, asset_list, gdc, pack_fname, 1))
            {
              if (gdc.isTimeChanged())
                gdc.save(cache_fname, mgr);
              debug("skip %s", pack_fname);
              tex_checked_cnt += asset_list.size();
              tex_total += asset_list.size();
              tex_pack[i]->buildRes = 2;
              continue;
            }

          process_tex_assets:
            for (int j = 0; j < asset_list.size(); j++)
              if (!asset_list[j]->testUserFlags(1) && texconvcache::is_tex_asset_convertible(*asset_list[j]))
              {
                DagorAsset *tex = asset_list[j];
                if (get_time_msec() > next_log_upd_t)
                {
                  log.addMessage(IMPORTANT_NOTE, "  %4d tex done so far (and %d checked)...", tex_cnt, tex_checked_cnt);
                  next_log_upd_t = get_time_msec() + UPD_LOG_INTERVAL_MS;
                }

                tex_total++;
                if (texconvcache::validate_tex_asset_cache(*tex, targetCode, profile, NULL, false) == 1)
                {
                  tex_checked_cnt++;
                  continue;
                }
                tex_to_build++;
                ctx = jp.getAvailableJobId(0);
                if (!ctx)
                  ctex.push_back(tex);
                else
                {
                  if (ctx->state == 3)
                    debug("  job j%02d: finished %s", get_job_idx(ctx), ctx->data);
                  ctx->setup(targetCode, profile);
                  strncpy(ctx->data, tex->getName(), sizeof(ctx->data));
                  jp.startJob(ctx, 1);
                  tex_cnt++;
                  log.addMessage(ILogWriter::NOTE, "scheduled tex asset <%s> conversion", tex->getName());
                }
              }
          }


        if (tex_to_build)
          log.addMessage(IMPORTANT_NOTE, "  %4d tex done so far (and %d checked), total to build %d/%d...", tex_cnt, tex_checked_cnt,
            tex_to_build, tex_total);

        for (int i = 0; i < ctex.size(); i++)
        {
          ctx = jp.getAvailableJobId(36000000);
          if (!ctx)
          {
            debug("failed to alloc context");
            for (int i = 0; i < mgr.getAssetTypesCount(); i++)
              if (IDagorAssetExporter *exporter = mgr.getAssetExporter(i))
                exporter->setBuildResultsBlk(NULL);
            RELEASE_ASSETS_PACKS();
            return false;
          }
          if (ctx->state == 3)
            debug("  job j%02d: finished %s", get_job_idx(ctx), ctx->data);
          ctx->setup(targetCode, profile);
          strncpy(ctx->data, ctex[i]->getName(), sizeof(ctx->data));
          jp.startJob(ctx, 1);
          log.addMessage(ILogWriter::NOTE, "scheduled tex asset <%s> conversion", ctex[i]->getName());

          tex_cnt++;
          if (get_time_msec() > next_log_upd_t)
          {
            log.addMessage(IMPORTANT_NOTE, "  %4d tex done so far, %d rest...", tex_cnt, tex_to_build - tex_cnt);
            next_log_upd_t = get_time_msec() + UPD_LOG_INTERVAL_MS;
          }
        }

        while (!jp.waitAllJobsDone(10000, &ctx))
          if (ctx)
            debug("  job j%02d: finished %s", get_job_idx(ctx), ctx->data);

        log.addMessage(IMPORTANT_NOTE, "  Updated %d tex for %.3f sec (%d tex checked)", tex_cnt, (get_time_msec() - t0) / 1000.0f,
          tex_total);
      }

      if (AssetExportCache::saveSharedData())
        jp.startAllJobs(7, targetCode, profile);
      else
        jp.startAllJobs(9, targetCode, profile);
      ctex.clear();

      log.addMessage(IMPORTANT_NOTE, "Building packs (distributed using %d jobs)...", jp.jobs);
      int t0 = get_time_msec();
      int tp_cnt = 0, rp_cnt = 0;

      for (int pkid = -1; pkid < addPackages.nameCount(); pkid++)
      {
        assethlp::build_package_dest_strings(dest_base, pack_fname_prefix, expblk, pkid < 0 ? nullptr : addPackages.getName(pkid),
          app_dir, target_str, profile);
        if (strlen(dest_base) > 290 || strlen(pack_fname_prefix) > 100)
          continue;

        for (int i = 0; i < tex_pack.size(); i++)
        {
          if (tex_pack[i]->buildRes == 2)
            continue;
          if (!tex_pack[i]->exported || tex_pack[i]->packageId != pkid)
            continue;
          if (strlen(tex_pack[i]->packName) > 490)
            continue;
          if (!get_exported_assets(asset_list, tex_pack[i]->assets, target_str, profile))
            continue;

          ctx = jp.getAvailableJobId(72000000); // 20 hours
          if (ctx)
          {
            if (ctx->state == 3)
              process_result(ctx, tex_pack, grp_pack);
            ctx->setup(targetCode, profile);
            ctx->pkgId = pkid;
            ctx->packId = i;
            strncpy(ctx->data, tex_pack[i]->packName, sizeof(ctx->data));
            strncpy(ctx->data + 500, dest_base, sizeof(ctx->data) - 500);
            strncpy(ctx->data + 800, pack_fname_prefix, sizeof(ctx->data) - 800);
            ctx->donePk = ++done_tpacks;
            ctx->totalPk = total_tpacks;
            jp.startJob(ctx, 2);
            tp_cnt++;
          }
        }
        for (int i = 0; i < grp_pack.size(); i++)
        {
          if (!grp_pack[i]->exported || grp_pack[i]->packageId != pkid)
            continue;
          if (strlen(grp_pack[i]->packName) > 490)
            continue;

          if (!get_exported_assets(asset_list, grp_pack[i]->assets, target_str, profile))
            continue;

          ctx = jp.getAvailableJobId(72000000); // 20 hours
          if (ctx)
          {
            if (ctx->state == 3)
              process_result(ctx, tex_pack, grp_pack);
            ctx->setup(targetCode, profile);
            ctx->pkgId = pkid;
            ctx->packId = i;
            strncpy(ctx->data, grp_pack[i]->packName, sizeof(ctx->data));
            strncpy(ctx->data + 500, dest_base, sizeof(ctx->data) - 500);
            strncpy(ctx->data + 800, pack_fname_prefix, sizeof(ctx->data) - 800);
            ctx->donePk = ++done_rpacks;
            ctx->totalPk = total_rpacks;
            jp.startJob(ctx, 3);
            rp_cnt++;
          }
        }
      }
      while (!jp.waitAllJobsDone(10000, &ctx))
        if (ctx)
          process_result(ctx, tex_pack, grp_pack);

      log.addMessage(IMPORTANT_NOTE, "  Processed %d texpacks and %d respacks for %.3f sec", tp_cnt, rp_cnt,
        (get_time_msec() - t0) / 1000.0f);
      if (dabuild_dry_run)
        continue;
    }

    int pkid = -1;

  build_packages_loop:
    pack_list_fname = expblk.getStr("packlist", NULL);
    assethlp::build_package_dest_strings(dest_base, pack_fname_prefix, expblk, pkid < 0 ? nullptr : addPackages.getName(pkid), app_dir,
      target_str, profile);
    if (pkid < 0)
    {
      if (!pkgBlk.getBool("::main", profile ? pkgBlk.getBool(String(0, "::main.%s", profile), true) : true))
      {
        log.addMessage(IMPORTANT_NOTE, "  Skip exporting main assets");
        pack_list_fname = NULL;
      }
    }
    else
    {
      if (!expblk.getBlockByNameEx("destGrpHdrCache")->getBool(addPackages.getName(pkid), true))
      {
        log.addMessage(IMPORTANT_NOTE, "  Exporting %s without grpHdrCache", addPackages.getName(pkid));
        pack_list_fname = NULL;
      }
    }

    if (pack_list_fname)
    {
      if (!grpvromfs_dest_fname)
        plm.load(dest_base + pack_list_fname);
      else
      {
        if (pkid < 0)
          grpvromfs_fname.printf(260, "%s/%s", app_dir, grpvromfs_dest_fname);
        else
          grpvromfs_fname.printf(260, "%s/%s", dest_base.str(), dd_get_fname(grpvromfs_dest_fname));
        plm.loadVromfs(grpvromfs_fname, pkid < 0 ? String(260, "%s/%s", grpvromfs_game_relpath, pack_list_fname) : pack_list_fname,
          targetCode);
      }
    }
    debug("grpvromfs_fname=<%s>", grpvromfs_fname.str());

    if (full_build)
      plm.invalidate(true, true);

    OAHashNameMap<true> built_ddsx_names; // only for dabuild_build_tex_separate case

    // build texture packs
    for (int i = 0; i < tex_pack.size(); i++)
    {
      if (!tex_pack[i]->exported || tex_pack[i]->packageId != pkid)
        continue;
      done_tpacks++;
      if (!get_exported_assets(asset_list, tex_pack[i]->assets, target_str, profile))
        continue;

      make_cache_fname(cache_fname, cache_base, addPackages.getName(pkid), tex_pack[i]->packName, target_str, profile);
      gdc.load(cache_fname, mgr);

      pack_fname.printf(260, "%s%s/%s%s", dest_base.str(), addTexPfx, pack_fname_prefix.str(), tex_pack[i]->packName.str());
      simplify_fname(pack_fname);

      if (dabuild_build_tex_separate)
        for (int ai = 0; ai < asset_list.size(); ai++)
        {
          String fn(0, "%s/%s.ddsx", pack_fname.str(), asset_list[ai]->getName());
          simplify_fname(fn);
          built_ddsx_names.addNameId(fn);
        }

      mkbindump::BinDumpSaveCB cwr(8 << 20, targetCode, write_be);
      cwr.setProfile(profile);
      ::dd_mkpath(pack_fname);
      log.startLog();
      stat_tex_total++;
      bool upToDate = false;
      int build_res = tex_pack[i]->buildRes;
      if (build_res == -1)
        dabuild_progress_prefix_text.printf(0, "[%d/%d] ", done_tpacks, total_tpacks);
      else
        done_tpacks--;
      if (build_res != -1)
      {
        if (build_res == 0)
        {
          if (eraseBadPacks)
          {
            dd_erase(cache_fname);
            stat_tex_sz_diff -= get_file_sz(pack_fname);
            dd_erase(pack_fname);
            plm.registerBadPack(pack_fname_prefix + tex_pack[i]->packName.str(), false, cache_fname);
          }

          log.addMessage(log.ERROR, "error writing ddsxTexPack: %s", pack_fname.str());
          success = false;
          stat_tex_failed++;
          continue;
        }
        else if (build_res == 1)
          stat_tex_built++;
        else if (build_res == 3)
        {
          stat_tex_built++;
          stat_tex_unchanged++;
        }
        else
          upToDate = true;
      }
      else if (!buildDdsxTexPack(cwr, asset_list, gdc, pack_fname, log, pbar, upToDate))
      {
        if (eraseBadPacks)
        {
          dd_erase(cache_fname);
          stat_tex_sz_diff -= get_file_sz(pack_fname);
          dd_erase(pack_fname);
          plm.registerBadPack(pack_fname_prefix + tex_pack[i]->packName.str(), false, cache_fname);
        }

        log.addMessage(log.ERROR, "error writing ddsxTexPack: %s", pack_fname.str());
        success = false;
        stat_tex_failed++;
        continue;
      }

      if (addTexPfx && *addTexPfx)
      {
        tmp_str.printf(260, "%s/%s%s", addTexPfx, pack_fname_prefix.str(), tex_pack[i]->packName.str());
        simplify_fname(tmp_str);
        plm.registerGoodPack(tmp_str, false, cache_fname);
      }
      else
        plm.registerGoodPack(pack_fname_prefix + tex_pack[i]->packName.str(), false, cache_fname);

      ::dd_mkpath(cache_fname);
      if (build_res == -1 && !dabuild_dry_run && (!upToDate || gdc.isTimeChanged()))
        gdc.save(cache_fname, mgr);
    }

    // build game resource packs
    for (int i = 0; i < grp_pack.size(); i++)
    {
      if (!grp_pack[i]->exported || grp_pack[i]->packageId != pkid)
        continue;
      done_rpacks++;
      if (!get_exported_assets(asset_list, grp_pack[i]->assets, target_str, profile))
        continue;

      make_cache_fname(cache_fname, cache_base, addPackages.getName(pkid), grp_pack[i]->packName, target_str, profile);
      gdc.load(cache_fname, mgr);

      pack_fname.printf(260, "%s%s%s", dest_base.str(), pack_fname_prefix.str(), grp_pack[i]->packName.str());
      simplify_fname(pack_fname);

      mkbindump::BinDumpSaveCB cwr(8 << 20, targetCode, write_be);
      cwr.setProfile(profile);
      ::dd_mkpath(pack_fname);
      log.startLog();
      stat_grp_total++;
      bool upToDate = false;
      int build_res = grp_pack[i]->buildRes;
      if (build_res == -1)
        dabuild_progress_prefix_text.printf(0, "[%d/%d] ", done_rpacks, total_rpacks);
      else
        done_rpacks--;
      if (build_res != -1)
      {
        if (build_res == 0)
        {
          if (eraseBadPacks)
          {
            dd_erase(cache_fname);
            stat_tex_sz_diff -= get_file_sz(pack_fname);
            dd_erase(pack_fname);
            plm.registerBadPack(pack_fname_prefix + grp_pack[i]->packName.str(), true, cache_fname);
          }

          log.addMessage(log.ERROR, "error writing gameResPack: %s", pack_fname.str());
          success = false;
          stat_grp_failed++;
          continue;
        }
        else if (build_res == 1)
          stat_grp_built++;
        else if (build_res == 3)
        {
          stat_grp_built++;
          stat_grp_unchanged++;
        }
        else
          upToDate = true;
      }
      else if (!buildGameResPack(cwr, asset_list, gdc, pack_fname, log, pbar, upToDate, build_blk))
      {
        if (eraseBadPacks)
        {
          dd_erase(cache_fname);
          stat_grp_sz_diff -= get_file_sz(pack_fname);
          dd_erase(pack_fname);
          plm.registerBadPack(pack_fname_prefix + grp_pack[i]->packName.str(), true, cache_fname);
        }

        log.addMessage(log.ERROR, "error writing gameResPack: %s", pack_fname.str());
        success = false;
        stat_grp_failed++;
        continue;
      }

      plm.registerGoodPack(pack_fname_prefix + grp_pack[i]->packName.str(), true, cache_fname);

      ::dd_mkpath(cache_fname);
      if (build_res == -1 && !dabuild_dry_run && (!upToDate || gdc.isTimeChanged()))
        gdc.save(cache_fname, mgr);
    }

    if (pack_list_fname && !dabuild_dry_run)
    {
      plm.sort();

      if (pkid >= 0 && !pkgBlk.getBlockByNameEx(addPackages.getName(pkid))->getBool(target_str, pkgBlk.getBool("defaultOn", true)))
        dd_erase((grpvromfs_dest_fname && grpvromfs_game_relpath) ? grpvromfs_fname : (dest_base + pack_list_fname));
      else if (grpvromfs_dest_fname && grpvromfs_game_relpath)
        plm.writeHdrCacheVromfs(dest_base, pkid < 0 ? grpvromfs_game_relpath : ".", grpvromfs_fname, pack_list_fname, targetCode,
          writeGrpHdrCache, pack_opt);
      else
        plm.save(dest_base + pack_list_fname, pack_opt);

      for (int i = 0; i < plm.list.size(); i++)
        if (!plm.list[i].marked)
        {
          log.addMessage(log.NOTE, "remove obsolete pack: %s", plm.list[i].fn.str());
          int old_sz = get_file_sz(dest_base + plm.list[i].fn.str());
          if (plm.list[i].grp)
            stat_grp_sz_diff -= old_sz;
          else
            stat_tex_sz_diff -= old_sz;
          dd_erase(dest_base + plm.list[i].fn.str());
          stat_pack_removed++;
        }
    }

    if (dabuild_build_tex_separate)
    {
      Tab<SimpleString> found_ddsx_files;
      find_files_in_folder(found_ddsx_files, String(0, "%s%s", dest_base, addTexPfx), ".ddsx", false, true, true);
      int removed_cnt = 0;
      for (int fi = 0; fi < found_ddsx_files.size(); fi++)
      {
        dd_simplify_fname_c(found_ddsx_files[fi].str());
        if (built_ddsx_names.getNameId(found_ddsx_files[fi]) < 0)
        {
          log.addMessage(log.NOTE, "remove obsolete tex: %s", found_ddsx_files[fi]);
          stat_tex_sz_diff -= get_file_sz(found_ddsx_files[fi]);
          dd_erase(found_ddsx_files[fi]);
          removed_cnt++;
        }
      }
      if (removed_cnt)
        log.addMessage(IMPORTANT_NOTE, "  removed %d old/extraneous texture(s)", removed_cnt);
      built_ddsx_names.reset(false);
    }

    pkid++;
    if (pkid < addPackages.nameCount())
      goto build_packages_loop;
  }

  clear_and_shrink(dabuild_progress_prefix_text);
  dabuild_finish_out_blk(out_blk, mgr, build_blk, expblk, app_dir, targetCode, profile);
  RELEASE_ASSETS_PACKS();
  return success && !log.hasErrors();
}

bool checkUpToDate(DagorAssetMgr &mgr, const char *app_dir, unsigned targetCode, const char *profile, int tc_flags,
  dag::ConstSpan<const char *> packs_to_check, dag::ConstSpan<bool> exp_types_mask, const DataBlock &appblk,
  IGenericProgressIndicator &pbar, ILogWriter &log)
{
  TIME_PROFILE(checkUpToDate);
  const DataBlock &expblk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");

  const char *addTexPfx = expblk.getStr("addTexPrefix", "");

  Tab<AssetPack *> grp_pack(tmpmem), tex_pack(tmpmem);
  FastNameMapEx addPackages;
  char target_str[6];
  strcpy(target_str, mkbindump::get_target_str(targetCode));
  makePack(mgr, mgr.getAssets(), exp_types_mask, expblk, tex_pack, grp_pack, addPackages, log, true, true, target_str, profile);

  AssetExportCache::setSdkRoot(app_dir, "develop");
  AssetExportCache gdc;
  String cache_fname, pack_fname, pack_fname_prefix;

  if (packs_to_check.size())
  {
    for (int i = 0; i < grp_pack.size(); i++)
      grp_pack[i]->exported = false;
    for (int i = 0; i < tex_pack.size(); i++)
      tex_pack[i]->exported = false;

    int packsCnt = packs_to_check.size();
    for (int i = 0; i < packsCnt; i++)
    {
      int g_id = get_pack_id(grp_pack, -2, packs_to_check[i], false);
      int t_id = get_pack_id(tex_pack, -2, packs_to_check[i], false);

      if (g_id >= 0)
        grp_pack[g_id]->exported = true;
      if (t_id >= 0)
        tex_pack[t_id]->exported = true;
      if (g_id < 0 && t_id < 0)
        log.addMessage(log.WARNING, "unknown pack: %s, skipped", packs_to_check[i]);
    }
  }

  String cache_base(260, "%s/%s/", app_dir, expblk.getStr("cache", "/develop/.cache"));
  String dest_base;

  bool changed = false;
  Tab<DagorAsset *> asset_list(tmpmem);

  for (int ti = 0; ti < 1; ti++) //-V1008
  {
    int flag = tc_flags;

    const DataBlock *destBlk = expblk.getBlockByName("destination");

    bool be = dagor_target_code_be(targetCode);

    if (!assethlp::validate_exp_blk(addPackages.nameCount() > 0, expblk, target_str, profile, log))
      continue;

    // check texture packs
    for (int i = 0; i < tex_pack.size(); i++)
    {
      if (!tex_pack[i]->exported)
        continue;
      if (!get_exported_assets(asset_list, tex_pack[i]->assets, target_str, profile))
        continue;

      int pkid = tex_pack[i]->packageId;
      assethlp::build_package_dest_strings(dest_base, pack_fname_prefix, expblk, pkid < 0 ? nullptr : addPackages.getName(pkid),
        app_dir, target_str, profile);
      make_cache_fname(cache_fname, cache_base, addPackages.getName(pkid), tex_pack[i]->packName, target_str, profile);

      simplify_fname(cache_fname);

      pack_fname.printf(260, "%s%s/%s%s", dest_base.str(), addTexPfx, pack_fname_prefix.str(), tex_pack[i]->packName.str());
      simplify_fname(pack_fname);

      if (is_cache_outdated(gdc, cache_fname, mgr, pack_fname, log))
        gdc.reset();
      else if (gdc.isTimeChanged())
        gdc.save(cache_fname, mgr);

      log.startLog();
      if (checkDdsxTexPackUpToDate(targetCode, profile, be, asset_list, gdc, pack_fname, flag))
        changed = true;
      else if (gdc.isTimeChanged())
        gdc.save(cache_fname, mgr);
    }

    // check game resource packs
    for (int i = 0; i < grp_pack.size(); i++)
    {
      if (!grp_pack[i]->exported)
        continue;
      if (!get_exported_assets(asset_list, grp_pack[i]->assets, target_str, profile))
        continue;


      int pkid = grp_pack[i]->packageId;
      assethlp::build_package_dest_strings(dest_base, pack_fname_prefix, expblk, pkid < 0 ? nullptr : addPackages.getName(pkid),
        app_dir, target_str, profile);
      make_cache_fname(cache_fname, cache_base, addPackages.getName(pkid), grp_pack[i]->packName, target_str, profile);

      simplify_fname(cache_fname);

      pack_fname.printf(260, "%s%s%s", dest_base.str(), pack_fname_prefix.str(), grp_pack[i]->packName.str());
      simplify_fname(pack_fname);

      if (is_cache_outdated(gdc, cache_fname, mgr, pack_fname, log))
        gdc.reset();
      else if (gdc.isTimeChanged())
        gdc.save(cache_fname, mgr);

      log.startLog();
      if (checkGameResPackUpToDate(asset_list, gdc, pack_fname, flag))
        changed = true;
      else if (gdc.isTimeChanged())
        gdc.save(cache_fname, mgr);
    }
  }

  RELEASE_ASSETS_PACKS();
  return changed;
}

bool cmp_data_eq(mkbindump::BinDumpSaveCB &cwr, const char *pack_fname)
{
  static const int BUF_SZ = 16 << 10;
  char buf[BUF_SZ], buf2[BUF_SZ];
  file_ptr_t fp = df_open(pack_fname, DF_READ);

  if (!fp)
    return false;
  int sz = cwr.getSize();
  if (df_length(fp) != sz)
  {
    df_close(fp);
    return false;
  }

  MemoryLoadCB crd(cwr.getMem(), false);
  while (sz > 0)
  {
    int rdsz = eastl::min(sz, BUF_SZ);
    df_read(fp, buf, rdsz);
    crd.read(buf2, rdsz);
    if (memcmp(buf, buf2, rdsz) != 0)
    {
      df_close(fp);
      return false;
    }
    sz -= rdsz;
  }
  df_close(fp);
  return true;
}
bool cmp_data_eq(const void *data, int sz, const char *pack_fname)
{
  static const int BUF_SZ = 16 << 10;
  char buf[BUF_SZ], buf2[BUF_SZ];
  file_ptr_t fp = df_open(pack_fname, DF_READ);

  if (!fp)
    return false;
  if (df_length(fp) != sz)
  {
    df_close(fp);
    return false;
  }

  InPlaceMemLoadCB crd(data, sz);
  while (sz > 0)
  {
    int rdsz = eastl::min(sz, BUF_SZ);
    df_read(fp, buf, rdsz);
    crd.read(buf2, rdsz);
    if (memcmp(buf, buf2, rdsz) != 0)
    {
      df_close(fp);
      return false;
    }
    sz -= rdsz;
  }
  df_close(fp);
  return true;
}

static void scan_pack_files(const char *folder_path, Tab<String> &packs)
{
  alefind_t ff;
  String tmpPath;

  tmpPath.printf(260, "%s/*.grp", folder_path);
  if (::dd_find_first(tmpPath, 0, &ff))
  {
    do
    {
      if (ff.name[0] != '.')
        packs.push_back().printf(0, "%s/%s", folder_path, ff.name);
    } while (dd_find_next(&ff));
    dd_find_close(&ff);
  }
  tmpPath.printf(260, "%s/*.dxp.bin", folder_path);
  if (::dd_find_first(tmpPath, 0, &ff))
  {
    do
    {
      if (ff.name[0] != '.')
        packs.push_back().printf(0, "%s/%s", folder_path, ff.name);
    } while (dd_find_next(&ff));
    dd_find_close(&ff);
  }

  // scan for sub-folders
  tmpPath.printf(260, "%s/*", folder_path);
  if (::dd_find_first(tmpPath, DA_SUBDIR, &ff))
  {
    do
      if (ff.attr & DA_SUBDIR)
      {
        if (dd_stricmp(ff.name, "cvs") == 0 || dd_stricmp(ff.name, ".svn") == 0)
          continue;

        scan_pack_files(String(260, "%s/%s", folder_path, ff.name), packs);
      }
    while (dd_find_next(&ff));
    dd_find_close(&ff);
  }
}

void dabuild_list_extra_packs(const char *app_dir, const DataBlock &appblk, DagorAssetMgr &mgr, const FastNameMap &pkg_to_list,
  unsigned targetCode, const char *profile, ILogWriter &log)
{
  const DataBlock &expblk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");
  const DataBlock &build_blk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("build");
  const char *addTexPfx = expblk.getStr("addTexPrefix", "");
  Tab<bool> expTypesMask(tmpmem);

  make_exp_types_mask(expTypesMask, mgr, expblk, log);

  String dest_base, pack_fname_prefix, pack_fname;
  Tab<AssetPack *> grp_pack(tmpmem), tex_pack(tmpmem);
  FastNameMapEx addPackages;
  char target_str[6];
  strcpy(target_str, mkbindump::get_target_str(targetCode));
  makePack(mgr, mgr.getAssets(), expTypesMask, expblk, tex_pack, grp_pack, addPackages, log, true, true, target_str, profile);

  if (!assethlp::validate_exp_blk(addPackages.nameCount() > 0, expblk, target_str, profile, log))
  {
    RELEASE_ASSETS_PACKS();
    return;
  }

  const DataBlock &pkgBlk = *expblk.getBlockByNameEx("packages");
  for (int i = 0; i < pkgBlk.blockCount(); i++)
    addPackages.addNameId(pkgBlk.getBlock(i)->getBlockName());

  for (int pkid = -1; pkid < addPackages.nameCount(); pkid++)
  {
    if (pkid < 0)
    {
      if (pkg_to_list.nameCount() && pkg_to_list.getNameId("*") < 0)
        continue;
    }
    else
    {
      if (pkg_to_list.nameCount() && pkg_to_list.getNameId(addPackages.getName(pkid)) < 0)
        continue;
    }
    assethlp::build_package_dest_strings(dest_base, pack_fname_prefix, expblk, pkid < 0 ? nullptr : addPackages.getName(pkid), app_dir,
      target_str, profile);

    Tab<String> pack_files;
    scan_pack_files(dest_base, pack_files);
    for (int f = 0; f < pack_files.size(); f++)
      simplify_fname(pack_files[f]);

    Tab<DagorAsset *> asset_list(tmpmem);
    for (int i = 0; i < tex_pack.size(); i++)
    {
      if (!tex_pack[i]->exported || tex_pack[i]->packageId != pkid)
        continue;
      asset_list.clear();
      if (!get_exported_assets(asset_list, tex_pack[i]->assets, target_str, profile))
        continue;

      pack_fname.printf(260, "%s%s/%s%s", dest_base.str(), addTexPfx, pack_fname_prefix.str(), tex_pack[i]->packName.str());
      simplify_fname(pack_fname);
      erase_item_by_value(pack_files, pack_fname);
    }

    for (int i = 0; i < grp_pack.size(); i++)
    {
      if (!grp_pack[i]->exported || grp_pack[i]->packageId != pkid)
        continue;
      asset_list.clear();
      if (!get_exported_assets(asset_list, grp_pack[i]->assets, target_str, profile))
        continue;

      pack_fname.printf(260, "%s%s%s", dest_base.str(), pack_fname_prefix.str(), grp_pack[i]->packName.str());
      simplify_fname(pack_fname);
      erase_item_by_value(pack_files, pack_fname);
    }

    if (!pack_files.size())
      continue;

    printf("-- found extra files in %s\n", dest_base.str());
    for (int f = 0; f < pack_files.size(); f++)
      printf("%s\n", pack_files[f].str());
  }
  clear_all_ptr_items(grp_pack);
  clear_all_ptr_items(tex_pack);
}

void dabuild_list_packs(const char *app_dir, const DataBlock &appblk, DagorAssetMgr &mgr, const FastNameMap &pkg_to_list,
  unsigned targetCode, const char *profile, bool export_tex, bool export_res, dag::ConstSpan<const char *> packs_re_src,
  ILogWriter &log)
{
  const DataBlock &expblk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");
  const DataBlock &build_blk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("build");
  const char *addTexPfx = expblk.getStr("addTexPrefix", "");
  Tab<bool> expTypesMask(tmpmem);

  make_exp_types_mask(expTypesMask, mgr, expblk, log);

  String dest_base, pack_fname_prefix, pack_fname;
  Tab<AssetPack *> grp_pack(tmpmem), tex_pack(tmpmem);
  FastNameMapEx addPackages;
  char target_str[6];
  strcpy(target_str, mkbindump::get_target_str(targetCode));
  makePack(mgr, mgr.getAssets(), expTypesMask, expblk, tex_pack, grp_pack, addPackages, log, export_tex, export_res, target_str,
    profile);

  sort(grp_pack, &cmp_res_pack_name);
  sort(tex_pack, &cmp_tex_pack_name);
  if (!assethlp::validate_exp_blk(addPackages.nameCount() > 0, expblk, target_str, profile, log))
  {
    RELEASE_ASSETS_PACKS();
    return;
  }

  Tab<RegExp *> packs_re(tmpmem);
  for (int i = 0; i < packs_re_src.size(); i++)
  {
    RegExp *re = new RegExp;
    if (re->compile(packs_re_src[i], "i"))
      packs_re.push_back(re);
    else
    {
      log.addMessage(log.ERROR, "invalid regexp: %s", packs_re_src[i]);
      delete re;
    }
  }

  if (packs_re.size())
    printf("\n=== %s (using %d regexp(s) to filter packs) ===\n", target_str, (int)packs_re.size());
  else
    printf("\n=== %s (full packs list) ===\n", target_str);
  if (!export_tex || !export_res)
    printf("=== list only %s %s ===\n", export_tex ? "tex" : "", export_res ? "res" : "");
  DataBlock sumBlk;
  auto process_package_func = [&](int pkid, const char *pkg_name) {
    if (pkg_to_list.nameCount() && pkg_to_list.getNameId(pkg_name) < 0)
      return;
    assethlp::build_package_dest_strings(dest_base, pack_fname_prefix, expblk, pkid < 0 ? nullptr : addPackages.getName(pkid), app_dir,
      target_str, profile);
    printf("package %s: %s\n", pkid < 0 ? "MAIN" : addPackages.getName(pkid), dest_base.str());

    DataBlock &sumPkgBlk = *sumBlk.addBlock(dest_base);
    sumPkgBlk.setInt("pkid", pkid);
    sumPkgBlk.setInt("cntT", 0);
    sumPkgBlk.setInt("cntR", 0);

    Tab<DagorAsset *> asset_list(tmpmem);
    for (int i = 0; i < tex_pack.size(); i++)
    {
      if (!tex_pack[i]->exported || tex_pack[i]->packageId != pkid)
        continue;

      pack_fname.printf(260, "%s%s/%s%s", dest_base.str(), addTexPfx, pack_fname_prefix.str(), tex_pack[i]->packName.str());
      simplify_fname(pack_fname);
      bool pack_used = !packs_re.size();
      for (int j = 0; j < packs_re.size(); j++)
        if (packs_re[j]->test(pack_fname))
        {
          pack_used = true;
          break;
        }

      asset_list.clear();
      if (!get_exported_assets(asset_list, tex_pack[i]->assets, target_str, profile))
        continue;

      sumPkgBlk.setInt("cntT", sumPkgBlk.getInt("cntT", 0) + 1);
      sumPkgBlk.setInt(pack_fname, asset_list.size());
      if (!pack_used)
        continue;

      sort(asset_list, &cmp_asset_name);
      printf("  texPack: %s  (%d textures)\n", pack_fname.str(), (int)asset_list.size());
      for (int j = 0; j < asset_list.size(); j++)
        printf("    %s\n", asset_list[j]->getName());
      printf("\n");
    }

    for (int i = 0; i < grp_pack.size(); i++)
    {
      if (!grp_pack[i]->exported || grp_pack[i]->packageId != pkid)
        continue;

      pack_fname.printf(260, "%s%s%s", dest_base.str(), pack_fname_prefix.str(), grp_pack[i]->packName.str());
      simplify_fname(pack_fname);
      bool pack_used = !packs_re.size();
      for (int j = 0; j < packs_re.size(); j++)
        if (packs_re[j]->test(pack_fname))
        {
          pack_used = true;
          break;
        }

      asset_list.clear();
      if (!get_exported_assets(asset_list, grp_pack[i]->assets, target_str, profile))
        continue;

      sumPkgBlk.setInt("cntR", sumPkgBlk.getInt("cntR", 0) + 1);
      sumPkgBlk.setInt(pack_fname, asset_list.size());
      if (!pack_used)
        continue;

      sort(asset_list, &cmp_asset_name);
      printf("  gameRes: %s  (%d resources)\n", pack_fname.str(), (int)asset_list.size());
      for (int j = 0; j < asset_list.size(); j++)
        printf("    %s\n", asset_list[j]->getName());
      printf("\n");
    }
    printf("\n");
  };
  process_package_func(-1, "*");
  iterate_names_in_lexical_order(addPackages, process_package_func);

  if (!packs_re.size())
  {
    printf("\n=== %s summary ===\n", target_str);
    for (int ordi = 0; ordi < sumBlk.blockCount(); ordi++)
    {
      DataBlock &sumPkgBlk = *sumBlk.getBlock(ordi);
      int pkid = sumPkgBlk.getInt("pkid");
      printf("package %s: %s (%d tex packs, %d res packs)\n", pkid < 0 ? "MAIN" : addPackages.getName(pkid), sumPkgBlk.getBlockName(),
        sumPkgBlk.getInt("cntT"), sumPkgBlk.getInt("cntR"));

      for (int i = 3; i < sumPkgBlk.paramCount(); i++)
        printf("  %4d  in %s\n", sumPkgBlk.getInt(i), sumPkgBlk.getParamName(i));
      printf("\n");
    }
  }

  clear_all_ptr_items(packs_re);
  RELEASE_ASSETS_PACKS();
}

void make_cache_fname(String &cache_fname, const char *cache_base, const char *pkg_name, const char *pack_name, const char *target_str,
  const char *profile)
{
  if (profile && !*profile)
    profile = NULL;

  if (!pkg_name)
  {
    if (!profile)
      cache_fname.printf(260, "%s%s.%s.bin", cache_base, pack_name, target_str);
    else if (!dabuild_strip_d3d_res)
      cache_fname.printf(260, "%s~%s/%s.%s.bin", cache_base, profile, pack_name, target_str);
    else
      cache_fname.printf(260, "%s~%s~strip_d3d_res/%s.%s.bin", cache_base, profile, pack_name, target_str);
  }
  else
  {
    if (!profile)
      cache_fname.printf(260, "%s%s~%s.%s.bin", cache_base, pkg_name, pack_name, target_str);
    else if (!dabuild_strip_d3d_res)
      cache_fname.printf(260, "%s~%s/%s~%s.%s.bin", cache_base, profile, pkg_name, pack_name, target_str);
    else
      cache_fname.printf(260, "%s~%s~strip_d3d_res/%s~%s.%s.bin", cache_base, profile, pkg_name, pack_name, target_str);
  }
}

int make_exp_types_mask(Tab<bool> &expTypesMask, DagorAssetMgr &mgr, const DataBlock &expblk, ILogWriter &log)
{
  expTypesMask.resize(mgr.getAssetTypesCount());
  mem_set_0(expTypesMask);
  int nid = expblk.getNameId("type"), cnt = 0;
  for (int i = 0, ie = expblk.getBlockByNameEx("types")->paramCount(); i < ie; i++)
    if (expblk.getBlockByNameEx("types")->getParamType(i) == DataBlock::TYPE_STRING &&
        expblk.getBlockByNameEx("types")->getParamNameId(i) == nid)
    {
      int atype = mgr.getAssetTypeId(expblk.getBlockByNameEx("types")->getStr(i));
      if (atype == -1)
        log.addMessage(log.ERROR, "unknown asset type for export: %s", expblk.getBlockByNameEx("types")->getStr(i));
      else
      {
        expTypesMask[atype] = true;
        cnt++;
      }
    }
  return cnt;
}

int stat_grp_total = 0, stat_grp_built = 0, stat_grp_failed = 0;
int stat_tex_total = 0, stat_tex_built = 0, stat_tex_failed = 0;
int stat_grp_unchanged = 0, stat_tex_unchanged = 0;
int64_t stat_grp_sz_diff = 0, stat_tex_sz_diff = 0;
int64_t stat_changed_grp_total_sz = 0, stat_changed_tex_total_sz = 0;

int stat_pack_removed = 0;
bool dabuild_dry_run = false;
bool dabuild_strip_d3d_res = false;
bool dabuild_collapse_packs = false;
bool dabuild_stop_on_first_error = true;
bool dabuild_skip_any_build = false;
bool dabuild_force_dxp_rebuild = false;
bool dabuild_build_tex_separate = false;
String dabuild_progress_prefix_text;

int64_t get_file_sz(const char *fn)
{
  DagorStat st;
  if (df_stat(fn, &st) == 0)
    return st.size;
  return 0;
}

void dabuild_prepare_out_blk(DataBlock &dest, DagorAssetMgr &mgr, const DataBlock &build_blk)
{
  for (int i = 0; i < mgr.getAssetTypesCount(); i++)
    if (IDagorAssetExporter *exporter = mgr.getAssetExporter(i))
      if (const char *fn = build_blk.getBlockByNameEx(mgr.getAssetTypeName(i))->getStr("descListOutPath", NULL))
        if (*fn)
          exporter->setBuildResultsBlk(dest.addBlock(mgr.getAssetTypeName(i)));
}
void dabuild_finish_out_blk(DataBlock &dest, DagorAssetMgr &mgr, const DataBlock &build_blk, const DataBlock &export_blk,
  const char *app_dir, unsigned tc, const char *profile)
{
  const DataBlock &destCfg = *export_blk.getBlockByNameEx("destination");
  for (int i = 0; i < mgr.getAssetTypesCount(); i++)
    if (IDagorAssetExporter *exporter = mgr.getAssetExporter(i))
    {
      exporter->setBuildResultsBlk(NULL);
      if (const char *fn = build_blk.getBlockByNameEx(mgr.getAssetTypeName(i))->getStr("descListOutPath", NULL))
      {
        if (!*fn)
          continue;
        bool patch_mode = false;
        if (profile && *profile)
        {
          if (export_blk.getBlockByNameEx("profiles")->getBlockByNameEx(profile)->getBool("skipDescList", false))
            continue;
          else
            patch_mode = export_blk.getBlockByNameEx("profiles")->getBlockByNameEx(profile)->getBool("patchMode", false);
        }
        const DataBlock &src_blk = *dest.getBlockByNameEx(mgr.getAssetTypeName(i));
        if (!src_blk.blockCount())
          continue;

        String mntPoint;
        String ts(mkbindump::get_target_str(tc));
        String profile_suffix;
        if (profile && *profile)
          profile_suffix.printf(0, ".%s", profile);

        for (int p = 0; p < src_blk.blockCount(); p++)
        {
          const DataBlock &sblk = *src_blk.getBlock(p);
          if (!sblk.blockCount())
            continue;

          String packFnamePrefix;
          assethlp::build_package_dest_strings(mntPoint, packFnamePrefix, export_blk, src_blk.getBlock(p)->getBlockName(), app_dir, ts,
            profile);

          String abs_fn(0, "%s/%s%s.bin", mntPoint, fn, profile_suffix);

          void *m = global_mutex_create(dd_get_fname(fn));
          if (!m)
          {
            logerr("failed to create mutex %s", dd_get_fname(fn));
            continue;
          }
          if (global_mutex_enter(m, 2 * 60 * 1000) != 0)
          {
            global_mutex_destroy(m, dd_get_fname(fn));
            logerr("failed to wait on mutex %s for 2 min!", dd_get_fname(fn));
            continue;
          }
          DAGOR_TRY
          {
            DataBlock blk;

            if (dd_file_exist(abs_fn))
              blk.load(abs_fn);
            for (int j = 0; j < sblk.blockCount(); j++)
              blk.addBlock(sblk.getBlock(j)->getBlockName())->setFrom(sblk.getBlock(j));
            blk.removeParam("patchMode");
            blk.removeParam("__pack");
            if (patch_mode)
            {
              OAHashNameMap<true> pack_fn;
              blk.setBool("patchMode", patch_mode);
              for (int j = 0; j < blk.blockCount(); j++)
                if (DagorAsset *a = mgr.findAsset(blk.getBlock(j)->getBlockName(), i))
                  blk.getBlock(j)->setInt("__pack", pack_fn.addNameId(a->getDestPackName()));
              for (int j = 0; j < pack_fn.nameCount(); j++)
                blk.addStr("__pack", String::mk_str_cat(packFnamePrefix, pack_fn.getName(j)).toLower());
            }

            if (blk.blockCount())
            {
              dd_mkpath(abs_fn);
              dblk::pack_to_binary_file(blk, abs_fn);
              debug("sync %d asset descriptions to %s", src_blk.blockCount(), abs_fn);
            }
            else if (dd_file_exists(abs_fn))
              FullFileSaveCB zero_cwr(abs_fn);
          }
          DAGOR_CATCH(DagorException) { logerr("exception during %s update", abs_fn); }

          global_mutex_leave_destroy(m, dd_get_fname(fn));
        }
      }
    }
}
