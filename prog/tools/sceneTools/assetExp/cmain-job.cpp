// Copyright (C) Gaijin Games KFT.  All rights reserved.

static const char *dbg_name();
#define __DEBUG_FILEPATH          dbg_name()
#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#include <startup/dag_mainCon.inc.cpp>
#undef ERROR

#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetMsgPipe.h>
#include <assets/assetExpCache.h>
#include <assets/assetHlp.h>
#include <assets/assetPlugin.h>
#include <generic/dag_smallTab.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/util/conLogWriter.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/fileUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_sharedMem.h>
#include <osApiWrappers/dag_messageBox.h>
#include <debug/dag_debug.h>
#include <stdlib.h>
#include <stdio.h>
#include <assets/daBuildInterface.h>
#include "jobSharedMem.h"
#include "daBuild.h"
#include <libTools/util/setupTexStreaming.h>
#include <perfMon/dag_cpuFreq.h>
#include "atomicConProgress.h"
#include "loadAssetBase.h"

extern "C" IDaBuildInterface *__stdcall get_dabuild_interface();

class ConsoleLogWriterEx : public ConsoleLogWriter
{
public:
  ConsoleLogWriterEx() : level(NOTE), errCount(0), impWarnCnt(0) {}
  String impWarn;
  int impWarnCnt;

  void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum) override
  {
    if (type == REMARK || type < level)
      return;
    if (type == IMPORTANT_NOTE)
      type = NOTE;
    if (unsigned(type) == (WARNING | 0x100))
    {
      if (impWarnCnt < 20)
      {
        impWarn.avprintf(128, fmt, arg, anum);
        impWarn += '\n';
      }
      impWarnCnt++;
      type = WARNING;
    }
    AtomicPrintfMutex::inst.lock();
    ConsoleLogWriter::addMessageFmt(type, fmt, arg, anum);
    AtomicPrintfMutex::inst.unlock(stdout, true);

    char mark = ' ';
    switch (type)
    {
      case NOTE: mark = ' '; break;
      case WARNING: mark = 'W'; break;
      case ERROR: mark = 'E'; break;
      case FATAL: mark = 'F'; break;
      default: break;
    }
    logmessage_fmt(LOGLEVEL_DEBUG, String(0, "%c %s", mark, fmt), arg, anum);

    if (type == ERROR || type == FATAL)
      errCount++;
  }

  void startLog() override { errCount = 0; }
  void endLog() override {}
  bool hasErrors() const override { return errCount > 0; }

  MessageType level;
  int errCount;
};

class ConsoleMsgPipe : public NullMsgPipe
{
public:
  ConsoleMsgPipe(ConsoleLogWriterEx &l) : log(l) {}
  void onAssetMgrMessage(int msg_t, const char *msg, DagorAsset *a, const char *asset_src_fpath) override
  {
    updateErrCount(msg_t);
    if (msg_t == REMARK)
      return;

    if (asset_src_fpath)
      log.addMessage((ILogWriter::MessageType)msg_t, "%s (file %s)", msg, asset_src_fpath);
    else if (a)
      log.addMessage((ILogWriter::MessageType)msg_t, "%s (file %s)", msg, a->getSrcFilePath());
    else
      log.addMessage((ILogWriter::MessageType)msg_t, msg);
  }

  ConsoleLogWriterEx &log;
};

static DabuildJobSharedMem *jobMem = NULL;
static intptr_t jobMemHandle = 0;
static int jobAssignedId = -1;
static int jobIdBroken = -1;
static void __cdecl release_job_mem()
{
  if (!jobMem)
    return;
  if (jobAssignedId >= 0)
  {
    jobIdBroken = jobAssignedId;
    debug("breaking master link, job j%02d", jobAssignedId);
    debug_flush(false);
    jobMem->jobCtx[jobAssignedId].result = 0;
    jobMem->changeCtxState(jobMem->jobCtx[jobAssignedId], 4);
  }
  close_global_map_shared_mem(jobMemHandle, jobMem, sizeof(DabuildJobSharedMem));
  jobMem = NULL;
  jobAssignedId = -1;
}
static void dgs_release_job_mem() { release_job_mem(); }

static void dabuild_job_report_fatal(const char *title, const char *msg, const char *call_stk)
{
  if (strstr(msg, "---fatal context---"))
    ATOMIC_PRINTF("job j%02d: %s\n%s\n%s\n", jobIdBroken >= 0 ? jobIdBroken : jobAssignedId, title, msg, call_stk);
  else
  {
    char tmpbuf[2 << 10] = {0};
    ATOMIC_PRINTF("job j%02d: %s\n%s%s\n%s\n", jobIdBroken >= 0 ? jobIdBroken : jobAssignedId, title, msg,
      dgs_get_fatal_context(tmpbuf, sizeof(tmpbuf), false), call_stk);
  }
}

static int do_main(bool debugmode)
{
  if (__argc < 4)
  {
    printf("daBuild job v1.21\n"
           "Copyright (C) Gaijin Games KFT, 2023\n\n"
           "usage: daBuild-job-dev.exe <application.blk> <pid> <index>\n");
    return 1;
  }

  AtomicPrintfMutex::init("dabuild", __argv[1]);
  dgs_pre_shutdown_handler = dgs_release_job_mem;
  setvbuf(stdout, NULL, _IOFBF, 4096);
  char start_dir[260] = "";
  dag_get_appmodule_dir(start_dir, sizeof(start_dir));
  symhlp_load("daKernel" DAGOR_DLL);
  {
    char stamp_buf[256];
    debug(dagor_get_build_stamp_str_ex(stamp_buf, sizeof(stamp_buf), "\n", "*", ""));
  }

  String fn(128, "daBuild-shared-mem-%s", __argv[2]);
  int job_id = atoi(__argv[3]);
  jobAssignedId = job_id;
  jobMem = (DabuildJobSharedMem *)open_global_map_shared_mem(fn, nullptr, sizeof(DabuildJobSharedMem), jobMemHandle);

  if (!jobMem)
  {
    logerr("failed to open shared memory (or to map view of shared memory)");
    return 1;
  }
  atexit(release_job_mem);
  if (jobMem->fullMemSize != sizeof(DabuildJobSharedMem))
  {
    debug("jobMem->fullMemSize=%d != %d", jobMem->fullMemSize, sizeof(DabuildJobSharedMem));
    return 1;
  }
  dgs_execute_quiet = jobMem->quiet;
  dabuild_dry_run = jobMem->dryRun;
  dabuild_stop_on_first_error = jobMem->stopOnFirstError;
  dabuild_strip_d3d_res = jobMem->stripD3Dres;
  dabuild_collapse_packs = jobMem->collapsePacks;
  dgs_report_fatal_error = dabuild_job_report_fatal;

  bool show_important_warnings = jobMem->showImportantWarnings;

  ConsoleLogWriterEx log;
  ConsoleMsgPipe pipe(log);
  log.level = log.ERROR;

  DagorAssetMgr mgr(true);
  mgr.setMsgPipe(&pipe);

  IGenericProgressIndicator &pbar = jobMem->nopbar ? *(new QuietProgressIndicator) : *AtomicConsoleProgressIndicator::create(false);

  char app_dir[260];
  dd_get_fname_location(app_dir, __argv[1]);
  if (!app_dir[0])
    strcpy(app_dir, "./");

  DataBlock appblk;

  // read application.blk
  if (!appblk.load(__argv[1]))
  {
    debug("cannot load application.blk from <%s>", __argv[1]);
    return false;
  }

  bool q = dgs_execute_quiet;
  dgs_execute_quiet = true;
  appblk.setStr("appDir", app_dir);
  ::load_tex_streaming_settings(__argv[1], NULL, true);
  dgs_execute_quiet = q;

  // setup shaders dump path
  if (appblk.getStr("shaders", NULL))
    appblk.setStr("shadersAbs", String(260, "%s/%s", app_dir, appblk.getStr("shaders", NULL)));
  else
    appblk.setStr("shadersAbs", String(260, "%s/../common/compiledShaders/tools", start_dir));

  String cdk_dir(260, "%s/../", start_dir);
  appblk.setStr("dagorCdkDir", simplify_fname(cdk_dir));
  appblk.setBool("strip_d3dres", dabuild_strip_d3d_res);
  appblk.setBool("collapse_packs", dabuild_collapse_packs);
  appblk.setInt("dabuildJobCount", jobMem->jobCount);

  DataBlock::setRootIncludeResolver(app_dir);

  // setup allowed asset types
  mgr.setupAllowedTypes(*appblk.getBlockByNameEx("assets")->getBlockByNameEx("types"),
    appblk.getBlockByNameEx("assets")->getBlockByName("export"));

  // load asset base
  QuietProgressIndicator qpind;
  if (!loadAssetBase(mgr, app_dir, appblk, qpind, log))
    return 13;

  // init dabuild
  IDaBuildInterface *dabuild = get_dabuild_interface();
  if (!dabuild)
  {
    printf("ERR: cannot load daBuild-dev.dll\n");
    return 13;
  }

  dabuild->setupReports(&log, &pbar);

  if (!dabuild->init(start_dir, mgr, appblk, app_dir))
  {
    log.addMessage(log.WARNING, "no asset types to export; assets{export{types block missing?");
    return 0;
  }

  // load exporter plugins
  if (!dabuild->loadExporterPlugins())
  {
    dabuild->unloadExporterPlugins();
    return 13;
  }

  AssetExportCache::createSharedData(String(260, "%s/develop/.asset-local/assets-hash.bin", app_dir));
  AssetExportCache::sharedDataResetRebuildTypesList();
  for (int i = 4; i < __argc; i++)
  {
    AssetExportCache::sharedDataAddRebuildType(mgr.getAssetTypeId(__argv[i]));
    if (strcmp(__argv[i], "tex") == 0)
      dabuild_force_dxp_rebuild = true;
  }
  dabuild->setExpCacheSharedData(AssetExportCache::getSharedDataPtr());
  dabuild->processSrcHashForDestPacks();

  log.level = (ILogWriter::MessageType)jobMem->logLevel;


  // work loop
  debug("jobMem->pid=%d jobMem=%p", jobMem->pid, jobMem);
  if (int cnt = jobMem->forceRebuildAssetIdxCount)
  {
    debug("need to force rebuild %d assets", cnt);
    for (int i = 0; i < cnt; i++)
      AssetExportCache::sharedDataAddForceRebuildAsset(mgr.getAsset(jobMem->forceRebuildAssetIdx[i]).getNameTypified());
  }

  String name;

  int cmdGen = -1;
  DabuildJobSharedMem::JobCtx &ctx = jobMem->jobCtx[jobAssignedId];
  Tab<DagorAsset *> asset_list(tmpmem);
  Tab<AssetPack *> tex_pack(tmpmem);
  Tab<AssetPack *> grp_pack(tmpmem);
  FastNameMapEx addPackages;
  const DataBlock &exp_blk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");
  const DataBlock &build_blk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("build");
  setup_dxp_grp_write_ver(build_blk, log);

  dabuild_build_tex_separate = exp_blk.getBool("buildTexSeparate", false);
  Tab<bool> expTypesMask;

  AssetExportCache gdc;
  String cache_base(260, "%s/%s/", app_dir, exp_blk.getStr("cache", "/develop/.cache"));
  String dest_base, pack_fname_prefix;
  String cache_fname, pack_fname;
  const char *addTexPfx = exp_blk.getStr("addTexPrefix", "");

  simplify_fname(cache_base);

  make_exp_types_mask(expTypesMask, mgr, exp_blk, log);

  DataBlock out_blk;
  SimpleString lastProfile;
  unsigned lastTargetCode = 0;
  unsigned last_flush_time = 0, last_idle_reported_time = 0, idle_report_interval = 2000;

  dabuild_prepare_out_blk(out_blk, mgr, build_blk);
  while (jobMem->pid != 0xFFFFFFFFU)
  {
    uint64_t tc_storage = 0;
    int gen = jobMem->cmdGen;
    if (get_time_msec() > last_flush_time + 2000)
    {
      debug_flush(false);
      fflush(stdout);
      last_flush_time = get_time_msec();
    }
    if (gen == cmdGen)
    {
      sleep_msec(100);
      if (get_time_msec() > last_idle_reported_time + idle_report_interval)
      {
        idle_report_interval *= 10;
        debug("-- job is idle --");
        debug_flush(false);
        fflush(stdout);
        last_flush_time = last_idle_reported_time = get_time_msec();
      }
      continue;
    }
    cmdGen = gen;
    idle_report_interval = 2000;
    if (ctx.state == 1)
    {
      ctx.result = 0;
      if (lastTargetCode != ctx.targetCode || strcmp(lastProfile, SimpleString((char *)ctx.profileName)) != 0)
      {
        if (lastTargetCode)
        {
          dabuild_finish_out_blk(out_blk, mgr, build_blk, exp_blk, app_dir, lastTargetCode, lastProfile);
          dabuild_prepare_out_blk(out_blk, mgr, build_blk);
        }
        lastTargetCode = ctx.targetCode;
        lastProfile = (char *)ctx.profileName;
      }

      if (ctx.cmd == 1)
      {
        debug("=== update tex %s", ctx.data);
        FATAL_CONTEXT_AUTO_SCOPE(ctx.data);
        jobMem->changeCtxState(ctx, 2);
        DagorAsset *a = mgr.findAsset(ctx.data, mgr.getTexAssetTypeId());
        if (a && texconvcache::validate_tex_asset_cache(*a, ctx.targetCode, (char *)ctx.profileName, &log) >= 1)
          ctx.result = 1;
      }
      else if (ctx.cmd == 2)
      {
        debug("=== update texpack %s(%d)", ctx.data, ctx.pkgId);
        FATAL_CONTEXT_AUTO_SCOPE(ctx.data);
        jobMem->changeCtxState(ctx, 2);
        ctx.szDiff = stat_tex_sz_diff = 0;
        ctx.szChangedTotal = stat_changed_tex_total_sz = 0;
        int pkid = ctx.pkgId;
        int targetCode = ctx.targetCode;
        const char *target_str = mkbindump::get_target_str(targetCode, tc_storage);
        char profile[34];
        strcpy(profile, (const char *)ctx.profileName);
        bool write_be = dagor_target_code_be(targetCode);
        bool upToDate = false;
        int i = ctx.packId;

        if (i < 0 || i >= tex_pack.size() || stricmp(tex_pack[i]->packName, ctx.data) != 0)
          for (i = 0; i < tex_pack.size() && stricmp(tex_pack[i]->packName, ctx.data) != 0; i++)
            ;

        if (i >= tex_pack.size())
          goto finish_procssing;

        if (!get_exported_assets(asset_list, tex_pack[i]->assets, target_str, profile))
          goto finish_procssing;

        dest_base = ctx.data + 500;
        pack_fname_prefix = ctx.data + 800;
        make_cache_fname(cache_fname, cache_base, addPackages.getName(pkid), tex_pack[i]->packName, target_str, profile);
        gdc.load(cache_fname, mgr);

        pack_fname.printf(260, "%s%s/%s%s", dest_base.str(), addTexPfx, pack_fname_prefix.str(), tex_pack[i]->packName.str());
        simplify_fname(pack_fname);

        mkbindump::BinDumpSaveCB cwr(8 << 20, ctx.targetCode, write_be);
        cwr.setProfile(profile);
        ::dd_mkpath(pack_fname);
        log.startLog();

        int stu = stat_tex_unchanged;
        dabuild_progress_prefix_text.printf(0, "[%d/%d] ", ctx.donePk, ctx.totalPk);
        if (!buildDdsxTexPack(cwr, asset_list, gdc, pack_fname, log, pbar, upToDate))
        {
          log.addMessage(log.ERROR, "error writing ddsxTexPack: %s", pack_fname.str());
          goto finish_procssing;
        }

        ::dd_mkpath(cache_fname);
        if (!dabuild_dry_run && (!upToDate || gdc.isTimeChanged()))
          gdc.save(cache_fname, mgr);

        ctx.szDiff = stat_tex_sz_diff;
        ctx.szChangedTotal = stat_changed_tex_total_sz;
        ctx.result = (stat_tex_unchanged != stu) ? 3 : (upToDate ? 2 : 1);
      }
      else if (ctx.cmd == 3)
      {
        debug("=== update respack %s(%d)", ctx.data, ctx.pkgId);
        FATAL_CONTEXT_AUTO_SCOPE(ctx.data);
        jobMem->changeCtxState(ctx, 2);
        ctx.szDiff = stat_grp_sz_diff = 0;
        ctx.szChangedTotal = stat_changed_grp_total_sz = 0;
        int pkid = ctx.pkgId;
        int targetCode = ctx.targetCode;
        const char *target_str = mkbindump::get_target_str(targetCode, tc_storage);
        char profile[34];
        strcpy(profile, (const char *)ctx.profileName);
        bool write_be = dagor_target_code_be(targetCode);
        bool upToDate = false;
        int i = ctx.packId;

        if (i < 0 || i >= grp_pack.size() || stricmp(grp_pack[i]->packName, ctx.data) != 0)
          for (i = 0; i < grp_pack.size() && stricmp(grp_pack[i]->packName, ctx.data) != 0; i++)
            ;

        if (i >= grp_pack.size())
          goto finish_procssing;

        if (!get_exported_assets(asset_list, grp_pack[i]->assets, target_str, profile))
          goto finish_procssing;

        dest_base = ctx.data + 500;
        pack_fname_prefix = ctx.data + 800;
        make_cache_fname(cache_fname, cache_base, addPackages.getName(pkid), grp_pack[i]->packName, target_str, profile);
        gdc.load(cache_fname, mgr);

        pack_fname.printf(260, "%s%s%s", dest_base.str(), pack_fname_prefix.str(), grp_pack[i]->packName.str());
        simplify_fname(pack_fname);

        mkbindump::BinDumpSaveCB cwr(8 << 20, targetCode, write_be);
        cwr.setProfile(profile);
        ::dd_mkpath(pack_fname);
        log.startLog();
        int stu = stat_grp_unchanged;

        dabuild_progress_prefix_text.printf(0, "[%d/%d] ", ctx.donePk, ctx.totalPk);
        if (!buildGameResPack(cwr, asset_list, gdc, pack_fname, log, pbar, upToDate, build_blk))
        {
          log.addMessage(log.ERROR, "error writing gameResPack: %s", pack_fname.str());
          goto finish_procssing;
        }

        ::dd_mkpath(cache_fname);
        if (!dabuild_dry_run && (!upToDate || gdc.isTimeChanged()))
          gdc.save(cache_fname, mgr);

        ctx.szDiff = stat_grp_sz_diff;
        ctx.szChangedTotal = stat_changed_grp_total_sz;

        ctx.result = (stat_grp_unchanged != stu) ? 3 : (upToDate ? 2 : 1);
      }
      else if (ctx.cmd == 7)
      {
        DEBUG_CP();
        jobMem->changeCtxState(ctx, 2);
        AssetExportCache::reloadSharedData();
        goto prepare_packs;
      }
      else if (ctx.cmd == 9)
      {
        DEBUG_CP();
        jobMem->changeCtxState(ctx, 2);
      prepare_packs:
        AssetExportCache::sharedDataRemoveRebuildType(mgr.getTexAssetTypeId());
        const char *target_str = mkbindump::get_target_str(ctx.targetCode, tc_storage);
        preparePacks(mgr, mgr.getAssets(), expTypesMask, exp_blk, tex_pack, grp_pack, addPackages, log, jobMem->expTex, jobMem->expRes,
          target_str, (const char *)ctx.profileName);
        debug("=== prepared packs: %d (tex) and %d (res)", tex_pack.size(), grp_pack.size());
      }
      else
        log.addMessage(log.FATAL, "unrecognized command %d", ctx.cmd);

    finish_procssing:
      if (!jobMem)
        break;
      jobMem->changeCtxState(ctx, 3);
    }
  }
  if (lastTargetCode)
    dabuild_finish_out_blk(out_blk, mgr, build_blk, exp_blk, app_dir, lastTargetCode, lastProfile);
  clear_all_ptr_items(tex_pack);
  clear_all_ptr_items(grp_pack);
  DEBUG_CP();
  release_job_mem();

  // release dabuild
  texconvcache::term();
  dabuild->unloadExporterPlugins();
  dabuild->setupReports(NULL, NULL);
  // pbar.destroy(); //if we destroy ConsoleProgressIndicator explicitly, it will add \n to stdout

  AssetExportCache::sharedDataResetRebuildTypesList();
  dabuild->term();

  if (dabuild)
    dabuild->release();

  if (show_important_warnings && log.impWarnCnt)
    os_message_box(String(2048, "daBuild job%d registered %d serious warnings%s:\n\n%s", job_id, log.impWarnCnt,
                     log.impWarnCnt > 20 ? ";\nfirst 20 assets are" : "", log.impWarn.str()),
      "daBuild warnings", GUI_MB_OK | GUI_MB_ICON_ERROR);
  return 0;
}

int DagorWinMain(bool debugmode)
{
  int ret = 0;
  DAGOR_TRY { ret = do_main(debugmode); }
  DAGOR_CATCH(DagorException x)
  {
#if DAGOR_EXCEPTIONS_ENABLED
    logerr("CATCHED EXCEPTION: %s\n%s", x.excDesc, DAGOR_EXC_STACK_STR(x));
#endif
    ret = 13;
    release_job_mem();
  }
  return ret;
}

static const char *dbg_name()
{
  static char buf[32];
  if (__argc < 4)
    return NULL;
  _snprintf(buf, 32, "dabuild-dbg-j%02d", atoi(__argv[3]));
  return buf;
}
