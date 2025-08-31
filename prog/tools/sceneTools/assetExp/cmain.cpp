// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define __DEBUG_FILEPATH          "dabuild-dbg"
#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#include <startup/dag_mainCon.inc.cpp>
#undef ERROR

#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetMsgPipe.h>
#include <assets/assetExpCache.h>
#include <assets/assetFolder.h>
#include <assets/assetExporter.h>
#include <assets/assetHlp.h>
#include <generic/dag_smallTab.h>
#include <libTools/util/conLogWriter.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/fileUtils.h>
#include <ioSys/dag_findFiles.h>
#include <ioSys/dag_fileIo.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_sharedMem.h>
#include <osApiWrappers/dag_messageBox.h>
#include <debug/dag_debug.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assets/daBuildInterface.h>
#include <ioSys/dag_memIo.h>
#include "jobSharedMem.h"
#include "daBuild.h"
#include "pkgDepMap.h"
#include <libTools/util/setupTexStreaming.h>
#include "atomicConProgress.h"
#include <util/dag_fastIntList.h>
#include "asset_ref_hlp.h"
#include "loadAssetBase.h"
#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX
#include <spawn.h>
#include <time.h>
extern char **environ;
#endif

extern "C" IDaBuildInterface *__stdcall get_dabuild_interface();
static inline void clearCache(const String &path, const char *mask);
static int findBadReferences(dag::ConstSpan<DagorAsset *> assets, int tex_atype, Tab<IDagorAssetRefProvider::Ref> &tmp_refs);
static int checkAssetReferences(DagorAsset *asset, Tab<IDagorAssetRefProvider::Ref> &tmp_refs);

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

static time_t start_at_time = time(nullptr);
static void showTitle()
{
  char stamp_buf[256], start_time_buf[32] = {0};
  if (!strftime(start_time_buf, sizeof(start_time_buf), "%Y-%m-%d %H:%M:%S", gmtime(&start_at_time)))
    strcpy(start_time_buf, "???");
  printf("daBuild v1.38\n"
         "Copyright (C) Gaijin Games KFT, 2023\n[%s]\n(started at %s UTC+0)\n\n",
    dagor_get_build_stamp_str_ex(stamp_buf, sizeof(stamp_buf), "", "*", "") + 19, start_time_buf);
}
static void showUsage()
{
  printf("usage: daBuild-dev.exe [options] <application.blk> [packs_to_export]\n\n"
         "options are:\n"
         "  -target:{PC|X360|PS3} adds target platform for export\n"
         "  -profile:{name}       uses profile name to read settings from TARGET~PROFILE\n"
         "  -dry_run              perform build without writing any files\n"
         "  -build:{asset}[:out]  performs test build of single <asset> to optional <out> file (may use * for <out>)\n"
         "  -asset:{asset}        builds corresponding pack that contains specified asset\n"
         "  -buildFast            option for -build: to force fast build (generally no packing)\n"
         "  -showProps:{asset}    prints properties of specified asset to stdout\n"
         "  -showRefs:{asset}     prints list of assets referenced by specified asset to stdout\n"
         "  -showDeps:{asset}     prints list of files that specified asset depends on\n"
         "  -package:{pkg}        partial build/list/validate, only specified package(s)\n"
         "  -package_and_deps:{p} partial build/list/validate, only specified package(s) and its dependencies\n"
         "  -rebuild              rebuilds packs ignoring cache\n"
         "  -rebuild:<asset_type> rebuilds assets of specified type ignoring cache\n"
         "  -rebuildAsset:{asset} force rebuild of asset by name\n"
         "  -sharedNm[+|-]        force or disable usage of shared namemap for asset manager\n"
         "  -validate_pkg         validates locations of exported assests\n"
         "  -validate_pkg:strict  the same as -validate_pkg but checks non-exportable assets too\n"
         "  -jobs:<NUM>           create and utilize NUM distribuited dabuild jobs\n"
         "  -only_res             export *.grp packs only\n"
         "  -only_tex             export *.dxp.bin packs only\n"
         "  -packs_re:<regexp>    restrict pack to export by specified regexp\n"
         "  -help                 shows this screen\n"
         "  -q                    quiet mode - show erros and progress bar only\n"
         "  -nopbar               show progress messages but not progress percents\n"
         "  -q0                   equivalent to \"-q -nopbar\"\n"
         "  -show_warn            show important warnings in message box after build\n"
         "  -Q                    really quiet mode - show erros and hide progress bar\n"
         "  -e                    erase bad packs\n\n"
         "  -dumpExpVer           dumps exporter versions for used asset types and exits\n\n"
         "  -maintenance:{list|listExtraTex|delExtraTex|listExtraPack|checkRefs|only}\n"
         "  -maintenance:listProps:{<asset_type>|*}\n\n"
         "when 'packs_to_export' not specified, all asset packs are exported\n\n"
         "examples:\n"
         "  daBuild-dev -target:PC d:/dagor2/Aces/application.blk\n"
         "  daBuild-dev -target:PC -target:X360 d:/dagor2/Aces/application.blk aircrafts.grp\n"
         "  daBuild-dev -target:PS3 -packs_re:entities\\/airfields ../application.blk\n");
}

static DabuildJobSharedMem *jobMem = NULL;
static String shared_mem_fn;
static void __cdecl release_job_mem()
{
  if (!jobMem)
    return;
  DEBUG_CP();
  jobMem->pid = 0xFFFFFFFFU;
  AssetExportCache::setJobSharedMem(NULL);
  intptr_t hMapFile = jobMem->mapHandle;
#if _TARGET_PC_WIN
  for (int i = 0; i < jobMem->jobCount; i++)
    CloseHandle(jobMem->jobHandle[i]);
#endif
  close_global_map_shared_mem(hMapFile, jobMem, sizeof(DabuildJobSharedMem));
  unlink_global_shared_mem(shared_mem_fn);
  jobMem = NULL;
}
static void dgs_release_job_mem() { release_job_mem(); }

void __cdecl ctrl_break_handler(int)
{
  release_job_mem();
  AssetExportCache::saveSharedData();
  ATOMIC_PRINTF("\nAborted by user\n");
  AtomicPrintfMutex::term();
  exit(2);
}

int DagorWinMain(bool debugmode)
{
  dgs_pre_shutdown_handler = dgs_release_job_mem;
  setvbuf(stdout, NULL, _IOFBF, 4096);
  char start_dir[260] = "";
  dag_get_appmodule_dir(start_dir, sizeof(start_dir));
  symhlp_load("daKernel-dev.dll");
  {
    char stamp_buf[256], start_time_buf[32] = {0};
    if (!strftime(start_time_buf, sizeof(start_time_buf), "%Y-%m-%d %H:%M:%S", gmtime(&start_at_time)))
      strcpy(start_time_buf, "???");
    debug("%s [started at %s UTC+0]\n", dagor_get_build_stamp_str_ex(stamp_buf, sizeof(stamp_buf), "", "*", ""), start_time_buf);
  }

  Tab<const char *> arg(tmpmem);
  Tab<unsigned> targets(tmpmem);
  Tab<const char *> packs_re(tmpmem);
  OAHashNameMap<true> rebuild_types;
  FastNameMap rebuild_assets;
  FastNameMap pkg_list, pkg_list_deps;
  Tab<SimpleString> pkg_list_stor(tmpmem);
  FastNameMap listAssetPropsForTypes;
  const char *assets_profile = NULL;
  bool rebuild = false, quiet = false, nopbar = false, eraseBadPacks = false;
  bool export_tex = true, export_res = true;
  bool showpbarpct = true;
  bool show_important_warnings = false;
  int validate_pkg = 0;
  int maintTexOp = 0, maintPackOp = 0;
  int maintListOp = 0;
  bool maintOnly = false;
  bool maintenance_dump_exp_ver = false;
  Tab<SimpleString> singleAssetPairsList;
  Tab<SimpleString> assetsForPackBuild;
  Tab<SimpleString> singleAssetShowPropsList;
  Tab<SimpleString> singleAssetShowRefsList;
  Tab<SimpleString> singleAssetShowDepsList;
  bool singleAssetFastBuild = false;
  int jobs = 0;
  int useSharedNm = 0;

  for (int i = 1; i < __argc; i++)
    if (__argv[i][0] != '-')
      arg.push_back(__argv[i]);
    else if (strnicmp(__argv[i], "-target:", 8) == 0)
    {
      unsigned targetCode = 0;
      const char *targetStr = __argv[i] + 8;
      while (*targetStr)
      {
        targetCode = (targetCode >> 8) | (*targetStr << 24);
        targetStr++;
      }
      targets.push_back(targetCode);
    }
    else if (strnicmp(__argv[i], "-profile:", 9) == 0)
    {
      assets_profile = __argv[i] + 9;
      if (!*assets_profile)
        assets_profile = NULL;
      else if (strlen(assets_profile) > 32)
      {
        showTitle();
        printf("ERR: profile name <%s> is too long, %d max chars allowed", assets_profile, 32);
        return 1;
      }
    }
    else if (strnicmp(__argv[i], "-packs_re:", 10) == 0)
      packs_re.push_back(__argv[i] + 10);
    else if (strnicmp(__argv[i], "-jobs:", 6) == 0)
      jobs = atoi(__argv[i] + 6);
    else if (stricmp(__argv[i], "-only_res") == 0)
      export_tex = false, export_res = true;
    else if (stricmp(__argv[i], "-only_tex") == 0)
      export_tex = true, export_res = false;
    else if (stricmp(__argv[i], "-only_vromfs") == 0)
      dabuild_skip_any_build = true;
    else if (stricmp(__argv[i], "-rebuild") == 0)
      rebuild = true;
    else if (strnicmp(__argv[i], "-rebuild:", 9) == 0)
      rebuild_types.addNameId(__argv[i] + 9);
    else if (strnicmp(__argv[i], "-rebuildAsset:", 14) == 0)
      rebuild_assets.addNameId(__argv[i] + 14);
    else if (strnicmp(__argv[i], "-package:", 9) == 0)
      pkg_list.addNameId(__argv[i] + 9);
    else if (strnicmp(__argv[i], "-package_and_deps:", 18) == 0)
      pkg_list_deps.addNameId(__argv[i] + 18);
    else if (stricmp(__argv[i], "-dry_run") == 0)
      dabuild_dry_run = true;
    else if (stricmp(__argv[i], "-keep_building_after_error") == 0)
      dabuild_stop_on_first_error = false;
    else if (stricmp(__argv[i], "-validate_pkg") == 0)
      validate_pkg = 1;
    else if (stricmp(__argv[i], "-validate_pkg:strict") == 0)
      validate_pkg = 2;
    else if (strnicmp(__argv[i], "-build:", 7) == 0)
    {
      singleAssetPairsList.push_back() = __argv[i] + 7;
      if (char *p = strchr(singleAssetPairsList.back(), ':'))
      {
        singleAssetPairsList.push_back() = p + 1;
        *p = '\0';
      }
      else
        singleAssetPairsList.push_back();
      maintOnly = true;
    }
    else if (strnicmp(__argv[i], "-asset:", 7) == 0)
      assetsForPackBuild.push_back() = __argv[i] + 7;
    else if (strnicmp(__argv[i], "-showProps:", 11) == 0)
      singleAssetShowPropsList.push_back() = __argv[i] + 11, maintOnly = true, useSharedNm = -1;
    else if (strnicmp(__argv[i], "-showRefs:", 10) == 0)
      singleAssetShowRefsList.push_back() = __argv[i] + 10, maintOnly = true, useSharedNm = -1;
    else if (strnicmp(__argv[i], "-showDeps:", 10) == 0)
      singleAssetShowDepsList.push_back() = __argv[i] + 10, maintOnly = true, useSharedNm = -1;
    else if (stricmp(__argv[i], "-buildFast") == 0)
      singleAssetFastBuild = true;
    else if (strnicmp(__argv[i], "-maintenance:", 13) == 0)
    {
      if (strcmp(__argv[i] + 13, "listExtraTex") == 0)
        maintTexOp = 1;
      else if (strcmp(__argv[i] + 13, "delExtraTex") == 0)
        maintTexOp = 2;
      else if (strcmp(__argv[i] + 13, "listExtraPack") == 0)
        maintPackOp = 1;
      else if (strcmp(__argv[i] + 13, "delExtraPack") == 0)
        maintPackOp = 2;
      else if (strcmp(__argv[i] + 13, "list") == 0)
        maintListOp = 1, maintOnly = true;
      else if (strncmp(__argv[i] + 13, "listProps:", 10) == 0)
        listAssetPropsForTypes.addNameId(__argv[i] + 23);

      else if (strcmp(__argv[i] + 13, "checkRefs") == 0)
        maintListOp = 2;
      else if (strcmp(__argv[i] + 13, "only") == 0)
        maintOnly = true;
      else
      {
        showTitle();
        printf("ERR: unrecognized switch: %s\n\n", __argv[i]);
        return 1;
      }
    }
    else if (strcmp(__argv[i], "-dumpExpVer") == 0)
      maintenance_dump_exp_ver = true;
    else if (strcmp(__argv[i], "-q") == 0)
      quiet = true;
    else if (strcmp(__argv[i], "-nopbar") == 0)
      showpbarpct = false;
    else if (strcmp(__argv[i], "-show_warn") == 0)
      show_important_warnings = true;
    else if (strcmp(__argv[i], "-q0") == 0)
      quiet = true, showpbarpct = false;
    else if (strcmp(__argv[i], "-Q") == 0)
      quiet = nopbar = true;
    else if (strcmp(__argv[i], "-quiet") == 0)
      dgs_execute_quiet = true;
    else if (strcmp(__argv[i], "-e") == 0)
      eraseBadPacks = true;
    else if (strcmp(__argv[i], "-sharedNm+") == 0 || strcmp(__argv[i], "-sharedNm") == 0)
      useSharedNm = 1;
    else if (strcmp(__argv[i], "-sharedNm-") == 0)
      useSharedNm = -1;
    else if (strcmp(__argv[i], "-noeh") == 0)
      ; // just ignore
    else if (stricmp(__argv[i], "-help") == 0)
    {
      showTitle();
      showUsage();
      return 0;
    }
    else
    {
      showTitle();
      printf("ERR: unrecognized switch: %s\n\n", __argv[i]);
      return 1;
    }

  if (!useSharedNm && (validate_pkg || maintListOp || maintOnly || jobs > 0))
    useSharedNm = 1;

  if (dabuild_skip_any_build)
    jobs = 0;
  if (!quiet)
    showTitle();

  ConsoleLogWriterEx log;
  ConsoleMsgPipe pipe(log);
  dgs_execute_quiet = dgs_execute_quiet || quiet;

  if (dabuild_dry_run)
    log.addMessage(log.NOTE, "DRY RUN (no file writes!)");
  if (quiet)
    log.level = log.ERROR;

  fflush(stdout);
  if (arg.size() < 1)
  {
    printf("ERR: insufficient params\n\n");
    showUsage();
    return 1;
  }
  if (!targets.size())
  {
    log.addMessage(log.NOTE, "no target platform specified, default to PC");
    targets.push_back(_MAKE4C('PC'));
  }


  DagorAssetMgr mgr(useSharedNm > 0);
  mgr.setMsgPipe(&pipe);

  IGenericProgressIndicator &pbar = nopbar ? *(new QuietProgressIndicator) : *AtomicConsoleProgressIndicator::create(showpbarpct);

  char app_dir[260];
  dd_get_fname_location(app_dir, arg[0]);
  if (!app_dir[0])
    strcpy(app_dir, "./");

  IDaBuildInterface *dabuild = get_dabuild_interface();
  if (!dabuild)
  {
    printf("ERR: cannot load daBuild-dev.dll\n");
    return 13;
  }

  AssetExportCache::createSharedData(String(260, "%s/develop/.asset-local/assets-hash.bin", app_dir));
  dabuild->setExpCacheSharedData(AssetExportCache::getSharedDataPtr());
  DataBlock::setRootIncludeResolver(app_dir);

  dabuild->setupReports(&log, &pbar);

  DataBlock appblk;

  // read application.blk
  if (!appblk.load(arg[0]))
  {
    printf("ERR: cannot load application.blk from <%s>\n", arg[0]);
    return 13;
  }

  setup_dxp_grp_write_ver(*appblk.getBlockByNameEx("assets")->getBlockByNameEx("build"), log);
  appblk.setStr("appDir", app_dir);
  int max_jobs = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getInt("maxJobs", 32);
  if (jobs > max_jobs)
    jobs = max_jobs;

  bool q = dgs_execute_quiet;
  dgs_execute_quiet = true;
  ::load_tex_streaming_settings(arg[0], NULL, true);
  dgs_execute_quiet = q;

  // setup shaders dump path
  if (appblk.getStr("shaders", NULL))
    appblk.setStr("shadersAbs", String(260, "%s/%s", app_dir, appblk.getStr("shaders", NULL)));
  else
    appblk.setStr("shadersAbs", String(260, "%s/../common/compiledShaders/tools", start_dir));

  String cdk_dir(260, "%s/../", start_dir);
  appblk.setStr("dagorCdkDir", simplify_fname(cdk_dir));

  if (eraseBadPacks)
    appblk.addBlock("assets")->addBlock("export")->addBool("eraseBadPacks", true);

  if (assets_profile)
  {
    if (appblk.getBlockByName("assets")->getBlockByName("export")->getBlockByNameEx("profiles")->getBlockByName(assets_profile))
    {
      dabuild_strip_d3d_res = appblk.getBlockByName("assets")
                                ->getBlockByName("export")
                                ->getBlockByNameEx("profiles")
                                ->getBlockByName(assets_profile)
                                ->getBool("strip_d3d_res", false);
      dabuild_collapse_packs = appblk.getBlockByName("assets")
                                 ->getBlockByName("export")
                                 ->getBlockByNameEx("profiles")
                                 ->getBlockByName(assets_profile)
                                 ->getBool("collapse_packs", false);
      appblk.setBool("strip_d3dres", dabuild_strip_d3d_res);
      appblk.setBool("collapse_packs", dabuild_collapse_packs);
    }
    else if (appblk.getBlockByName("assets")->getBlockByName("export")->getBlockByNameEx("profiles")->getBool(assets_profile, false))
      ;
    else
    {
      printf("ERR: profile <%s> is not enabled in application.blk\n", assets_profile);
      return 13;
    }
  }

  // setup allowed asset types
  mgr.setupAllowedTypes(*appblk.getBlockByNameEx("assets")->getBlockByNameEx("types"),
    appblk.getBlockByNameEx("assets")->getBlockByName("export"));

  AssetExportCache::sharedDataResetRebuildTypesList();
  iterate_names(rebuild_types, [&](int, const char *name) {
    AssetExportCache::sharedDataAddRebuildType(mgr.getAssetTypeId(name));
    if (strcmp(name, "tex") == 0)
      dabuild_force_dxp_rebuild = true;
  });

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

  if (maintenance_dump_exp_ver)
  {
    printf("dabuild asset exporters version:\n");
    for (int i = 0, ie = mgr.getAssetTypesCount(); i < ie; i++)
      if (IDagorAssetExporter *exp = mgr.getAssetExporter(i))
      {
        const char *name = mgr.getAssetTypeName(i);
        name = name ? name : "";
        printf("  %s%*s  ver:%3d  cls:0x%08X\n", name, int(16 - i_strlen(name)), "", exp->getGameResVersion(),
          exp->getGameResClassId());
      }
    dabuild->unloadExporterPlugins();
    return 0;
  }

  // load asset base
  if (!loadAssetBase(mgr, app_dir, appblk, pbar, log))
  {
    dabuild->unloadExporterPlugins();
    return 13;
  }
  dabuild->processSrcHashForDestPacks();
  AssetExportCache::saveSharedData();

  const DataBlock &pkgBlk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export")->getBlockByNameEx("packages");
  dabuild_build_tex_separate = appblk.getBlockByNameEx("assets")->getBlockByNameEx("export")->getBool("buildTexSeparate", false);

  if (pkg_list_deps.nameCount())
  {
    PkgDepMap pkgDepMap(pkgBlk);
    iterate_names(pkg_list_deps, [&](int, const char *name) {
      int id = pkgDepMap.nm.getNameId(name);
      if (id < 0)
      {
        log.addMessage(log.WARNING, "know nothing about dependencies for package %s", name);
        return;
      }
      for (int j = 0; j < pkgDepMap.stride; j++)
        if (pkgDepMap.dep.get(id * pkgDepMap.stride + j))
          pkg_list.addNameId(pkgDepMap.nm.getName(j));
    });
    if (!pkg_list.nameCount())
      log.addMessage(log.ERROR, " -package_and_deps: switches had no effect!");
    pkg_list_deps.reset(true);
  }
  int ord_idx = 1;
  iterate_names(pkg_list, [&](int, const char *name) {
    pkg_list_stor.push_back() = String(0, "\1%s", name);
    insert_item_at(arg, ord_idx++, pkg_list_stor.back());
  });

  if (pkgBlk.getBool("forceExplicitList", false) || validate_pkg)
  {
    PkgDepMap pkgDepMap(pkgBlk);
    bool allow_any_pkg = !pkgBlk.getBool("forceExplicitList", false);
    const DataBlock &expblk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");

    for (int t = 0; t < targets.size(); t++)
    {
      uint64_t tc_storage = 0;
      const char *ts = mkbindump::get_target_str(targets[t], tc_storage);
      dag::ConstSpan<DagorAsset *> assets = mgr.getAssets();
      const char *pkg_by_folder_tex = NULL;
      const char *pkg_by_folder_res = NULL;
      for (int i = 0, last_fidx = -2; i < assets.size(); i++)
      {
        bool tex_asset = assets[i]->getType() == mgr.getTexAssetTypeId();
        const char *pkg = assets[i]->getCustomPackageName(ts, assets_profile, false);

        if (!pkg)
        {
          if (last_fidx != assets[i]->getFolderIndex())
          {
            last_fidx = assets[i]->getFolderIndex();
            pkg_by_folder_tex = mgr.getPackageName(last_fidx, "ddsxTex", ts, assets_profile);
            pkg_by_folder_res = mgr.getPackageName(last_fidx, "gameRes", ts, assets_profile);
            if (!pkg_by_folder_tex)
              pkg_by_folder_tex = mgr.getBasePkg();
            if (!pkg_by_folder_res)
              pkg_by_folder_res = mgr.getBasePkg();
          }

          pkg = tex_asset ? pkg_by_folder_tex : pkg_by_folder_res;
        }

        if (!pkgDepMap.setPkgToUserFlags(assets[i], pkg, allow_any_pkg))
          log.addMessage(log.ERROR, "package %s (reference from asset %s:%s) not listed in application.blk", pkg, assets[i]->getName(),
            assets[i]->getTypeStr());
      }

      if (validate_pkg)
      {
        pkgDepMap.rebuildMap(pkgBlk);
        if (!pkgDepMap.validateAssetDeps(mgr, expblk, pkg_list, targets[t], assets_profile, log, validate_pkg > 1))
          log.addMessage(log.ERROR, "%s: packages validation failed", ts);
        else
          for (DagorAsset *a1 : mgr.getAssets())
            if (!a1->isGloballyUnique() && a1->getType() != mgr.getTexAssetTypeId() && //
                mgr.getAssetExporter(a1->getType()) && a1->props.getBool("export", true))
              for (DagorAsset *a2 : mgr.getAssets())
                if (a1 != a2 && a2->getNameId() == a1->getNameId() && a2->getType() != mgr.getTexAssetTypeId())
                  if (auto *e2 = mgr.getAssetExporter(a2->getType()))
                    if (e2->isExportableAsset(*a2) && a2->props.getBool("export", true))
                      log.addMessage(ILogWriter::ERROR, "%s: asset is not unique, name clash with %s", //
                        a1->getNameTypified(), a2->getNameTypified());
      }
    }
    if (log.hasErrors())
    {
      dabuild->unloadExporterPlugins();
      return 13;
    }
    pkgDepMap.resetUserFlags(mgr);
  }

  FastIntList force_rebuild_assets;
  iterate_names_breakable(rebuild_assets, [&](int, const char *name) {
    if (DagorAsset *a = mgr.findAsset(name))
    {
      if (jobs > 0 && force_rebuild_assets.size() == countof(DabuildJobSharedMem::forceRebuildAssetIdx))
      {
        log.addMessage(ILogWriter::WARNING, "some assets scheduled for rebuild are omitted due to limitation(%d)",
          countof(DabuildJobSharedMem::forceRebuildAssetIdx));
        return IterateStatus::StopFail;
      }
      int ai = find_value_idx(mgr.getAssets(), a);
      force_rebuild_assets.addInt(ai);
      AssetExportCache::sharedDataAddForceRebuildAsset(a->getNameTypified());
      if (a->getType() == mgr.getTexAssetTypeId() && !strchr(name, '$') &&
          force_rebuild_assets.size() + 3 <= countof(DabuildJobSharedMem::forceRebuildAssetIdx))
      {
        if (DagorAsset *a1 = mgr.findAsset(String::mk_str_cat(name, "$tq")))
        {
          force_rebuild_assets.addInt(find_value_idx(mgr.getAssets(), a1));
          AssetExportCache::sharedDataAddForceRebuildAsset(a1->getNameTypified());
        }
        if (DagorAsset *a1 = mgr.findAsset(String::mk_str_cat(name, "$hq")))
        {
          force_rebuild_assets.addInt(find_value_idx(mgr.getAssets(), a1));
          AssetExportCache::sharedDataAddForceRebuildAsset(a1->getNameTypified());
        }
        if (DagorAsset *a1 = mgr.findAsset(String::mk_str_cat(name, "$uhq")))
        {
          force_rebuild_assets.addInt(find_value_idx(mgr.getAssets(), a1));
          AssetExportCache::sharedDataAddForceRebuildAsset(a1->getNameTypified());
        }
      }
    }
    else
      log.addMessage(ILogWriter::WARNING, "can't find asset <%s> to force rebuild", name);
    return IterateStatus::Continue;
  });

  for (int t = 0; t < targets.size(); t++)
    for (int i = 0; i < mgr.getAssetTypesCount(); i++)
      if (const char *fn = appblk.getBlockByNameEx("assets")
                             ->getBlockByNameEx("build")
                             ->getBlockByNameEx(mgr.getAssetTypeName(i))
                             ->getStr("descListOutPath", NULL))
      {
        if (!*fn)
          continue;
        if (assets_profile && *assets_profile)
          if (appblk.getBlockByName("assets")
                ->getBlockByName("export")
                ->getBlockByNameEx("profiles")
                ->getBlockByNameEx(assets_profile)
                ->getBool("skipDescList", false))
            continue;

        bool pkg_exp_def = pkgBlk.getBool("defaultOn", true);
        const DataBlock &expBlk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");
        const DataBlock &destBlk = *expBlk.getBlockByNameEx("destination");

        uint64_t tc_storage = 0;
        const char *ts = mkbindump::get_target_str(targets[t], tc_storage);
        String profile_suffix;
        if (assets_profile && *assets_profile)
          profile_suffix.printf(0, ".%s", assets_profile);

        String mntPoint;
        DataBlock blk;
        for (int p = -1, pe = pkgBlk.blockCount(); p < pe; p++)
        {
          const char *pk_name = p < 0 ? "." : pkgBlk.getBlock(p)->getBlockName();
          assethlp::build_package_dest(mntPoint, expBlk, pk_name, app_dir, ts, assets_profile);
          String abs_fn(0, "%s/%s%s.bin", mntPoint, fn, profile_suffix);

          DataBlock pk_blk;
          if (dd_file_exist(abs_fn) && pk_blk.load(abs_fn) && !pk_blk.isEmpty())
            blk.addNewBlock(&pk_blk, pk_name);
        }

        Tab<int> asset_idx_to_rebuild;
        bool has_missing = false, desc_changed = false;
        IDagorAssetExporter *e = mgr.getAssetExporter(i);
        if (e)
          e->setBuildResultsBlk(&blk);
        for (int j = 0; j < mgr.getAssetCount(); j++)
        {
          if (mgr.getAsset(j).getType() != i)
            continue;
          if (!dabuild->isAssetExportable(&mgr.getAsset(j)))
            continue;

          const char *pkname = mgr.getAsset(j).getCustomPackageName(ts, assets_profile);

          if (pkname && !pkgBlk.getBlockByNameEx(pkname)->getBool(ts, pkg_exp_def))
            continue;

          if (!blk.getBlockByNameEx(pkname ? pkname : ".")->getBlockByName(mgr.getAsset(j).getName()))
          {
            if (e && e->updateBuildResultsBlk(mgr.getAsset(j), AssetExportCache::getSharedDataPtr(), _MAKE4C('PC')))
              desc_changed = true;
            else
              asset_idx_to_rebuild.push_back(j);
          }
        }

        if (asset_idx_to_rebuild.size())
        {
          if (
            jobs > 0 && force_rebuild_assets.size() + asset_idx_to_rebuild.size() > countof(DabuildJobSharedMem::forceRebuildAssetIdx))
          {
            log.addMessage(log.NOTE, "%s.descListOutPath: need to rebuild all assets (due to %d missing desc)",
              mgr.getAssetTypeName(i), asset_idx_to_rebuild.size());
            rebuild_types.addNameId(mgr.getAssetTypeName(i));
            AssetExportCache::sharedDataAddRebuildType(i);
            if (mgr.getTexAssetTypeId())
              dabuild_force_dxp_rebuild = true;
          }
          else
          {
            log.addMessage(log.NOTE, "%s.descListOutPath: need to rebuild %d assets (due to missing desc)", mgr.getAssetTypeName(i),
              asset_idx_to_rebuild.size());
            for (int ai : asset_idx_to_rebuild)
            {
              force_rebuild_assets.addInt(ai);
              AssetExportCache::sharedDataAddForceRebuildAsset(mgr.getAsset(ai).getNameTypified());
            }
          }
        }
        for (int p = -1, pe = pkgBlk.blockCount(); p < pe; p++)
        {
          DataBlock *b = blk.getBlockByName(p < 0 ? "." : pkgBlk.getBlock(p)->getBlockName());
          if (!b)
            continue;
          for (int j = b->blockCount() - 1; j >= 0; j--)
          {
            DagorAsset *a = mgr.findAsset(b->getBlock(j)->getBlockName(), i);
            if (!a || !dabuild->isAssetExportable(a))
            {
              b->removeBlock(j);
              desc_changed = true;
            }
            else
            {
              const char *pkname = a->getCustomPackageName(ts, assets_profile);
              if (strcmp(p < 0 ? "." : pkgBlk.getBlock(p)->getBlockName(), pkname ? pkname : ".") != 0)
              {
                b->removeBlock(j);
                desc_changed = true;
              }
            }
          }
        }
        if (desc_changed && !maintOnly)
        {
          for (int p = -1, pe = pkgBlk.blockCount(); p < pe; p++)
          {
            const char *pk_name = p < 0 ? "." : pkgBlk.getBlock(p)->getBlockName();
            assethlp::build_package_dest(mntPoint, expBlk, pk_name, app_dir, ts, assets_profile);
            String abs_fn(0, "%s/%s%s.bin", mntPoint, fn, profile_suffix);

            if (blk.getBlockByNameEx(pk_name)->blockCount())
            {
              dd_mkpath(abs_fn);
              dblk::pack_to_binary_file(*blk.getBlockByName(pk_name), abs_fn);
              log.addMessage(log.NOTE, "%s.descListOutPath: removed extra entries (%s)", mgr.getAssetTypeName(i), pk_name);
            }
            else if (dd_file_exists(abs_fn))
              FullFileSaveCB zero_cwr(abs_fn);
          }
        }
        if (e)
          e->setBuildResultsBlk(nullptr);
      }

  if (assets_profile)
    log.addMessage(log.NOTE, "Exporting using profile: %s", assets_profile);


  String cacheBase, cacheDdsxBase;
  {
    const char *profile = (assets_profile && !*assets_profile) ? NULL : assets_profile;
    const DataBlock &expblk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");

    cacheBase.printf(260, profile ? "%s/%s/~%s%s/" : "%s/%s/", app_dir, expblk.getStr("cache", "/develop/.cache"), profile,
      dabuild_strip_d3d_res ? "~strip_d3d_res" : "");
    cacheDdsxBase.printf(260, "%s/%s/ddsx~cvt/", app_dir, expblk.getStr("cache", "/develop/.cache"));

    simplify_fname(cacheBase);
    simplify_fname(cacheDdsxBase);
  }

  Tab<int> exp_types;
  if (singleAssetPairsList.size() + assetsForPackBuild.size() + rebuild_assets.nameCount() + singleAssetShowPropsList.size() +
        singleAssetShowRefsList.size() + singleAssetShowDepsList.size() >
      0)
  {
    const DataBlock &typeBlk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export")->getBlockByNameEx("types");
    for (int i = 0, nid = typeBlk.getNameId("type"); i < typeBlk.paramCount(); i++)
      if (typeBlk.getParamNameId(i) == nid && typeBlk.getParamType(i) == typeBlk.TYPE_STRING)
      {
        int type = mgr.getAssetTypeId(typeBlk.getStr(i));
        if (type >= 0 && find_value_idx(exp_types, type) < 0)
          exp_types.push_back(type);
      }
  }

  for (unsigned ai = 0; ai < assetsForPackBuild.size(); ai++)
  {
    const SimpleString &assetName = assetsForPackBuild[ai];
    DagorAsset *a = mgr.findAsset(assetName, exp_types);
    if (!a)
    {
      log.addMessage(ILogWriter::ERROR, "can't get asset <%s> (tried %d exp types)", assetName, exp_types.size());
      return 1;
    }
    if (const char *p = a->getDestPackName())
    {
      debug("Package to be built is %s", p);
      const char *pkg = str_dup(p, strmem); // should have memory allocation, otherwise can be overwritten
      arg.push_back(pkg);
    }
  }

  for (const SimpleString &a_nm : singleAssetShowPropsList)
  {
    DagorAsset *a = mgr.findAsset(a_nm, exp_types);
    if (!a)
      a = get_asset_by_name(mgr, a_nm, -1);
    if (!a)
    {
      log.addMessage(ILogWriter::ERROR, "can't get asset <%s> (tried %d exp types)", a_nm, exp_types.size());
      return 1;
    }
    printf("\n--- asset <%s> props:\n", a_nm.c_str());
    DynamicMemGeneralSaveCB cwr(tmpmem, 0, 64 << 10);
    a->props.saveToTextStream(cwr);
    printf("%.*s", (int)cwr.size(), cwr.data());
  }

  Tab<IDagorAssetRefProvider::Ref> tmp_refs;
  for (const SimpleString &a_nm : singleAssetShowRefsList)
  {
    DagorAsset *a = mgr.findAsset(a_nm, exp_types);
    if (!a)
      a = get_asset_by_name(mgr, a_nm, -1);
    if (!a)
    {
      log.addMessage(ILogWriter::ERROR, "can't get asset <%s> (tried %d exp types)", a_nm, exp_types.size());
      return 1;
    }
    IDagorAssetRefProvider *refProvider = a->getMgr().getAssetRefProvider(a->getType());
    if (refProvider)
    {
      refProvider->getAssetRefs(*a, tmp_refs);
      printf("\n--- asset <%s> refs(%d):\n", a->getNameTypified().c_str(), (int)tmp_refs.size());
      for (const IDagorAssetRefProvider::Ref &r : tmp_refs)
        printf("  %c[%c%c] %s\n", r.flags & IDagorAssetRefProvider::RFLG_BROKEN ? '-' : ' ',
          r.flags & IDagorAssetRefProvider::RFLG_EXTERNAL ? 'X' : ' ', r.flags & IDagorAssetRefProvider::RFLG_OPTIONAL ? 'o' : ' ',
          r.getAsset() ? r.getAsset()->getNameTypified().c_str() //-V522
                       : (r.flags & IDagorAssetRefProvider::RFLG_BROKEN ? r.getBrokenRef() : ""));
    }
    else
      printf("\n--- asset <%s> refs: no refs provider", a->getNameTypified().c_str());
  }

  for (const SimpleString &a_nm : singleAssetShowDepsList)
  {
    DagorAsset *a = mgr.findAsset(a_nm, exp_types);
    if (!a)
      a = get_asset_by_name(mgr, a_nm, -1);
    if (!a)
    {
      log.addMessage(ILogWriter::ERROR, "can't get asset <%s> (tried %d exp types)", a_nm, exp_types.size());
      return 1;
    }
    if (IDagorAssetExporter *e = mgr.getAssetExporter(a->getType()))
    {
      Tab<SimpleString> files;
      e->gatherSrcDataFiles(*a, files);
      printf("\n--- asset <%s> depends on %d file(s):\n", a->getNameTypified().c_str(), (int)files.size());
      for (SimpleString &s : files)
        printf("  %s\n", s.c_str());
    }
    else
      log.addMessage(ILogWriter::ERROR, "can't get exporter plugin for %s (type=%s)", a->getName(), a->getTypeStr());
  }

  for (unsigned sai = 0; sai < singleAssetPairsList.size(); sai += 2)
  {
    const SimpleString &singleAsset = singleAssetPairsList[sai + 0];
    SimpleString &singleAssetOut = singleAssetPairsList[sai + 1];
    // test build of single asset
    DagorAsset *a = mgr.findAsset(singleAsset, exp_types);
    if (!a)
    {
      log.addMessage(ILogWriter::ERROR, "can't get asset <%s> (tried %d exp types)", singleAsset, exp_types.size());
      return 1;
    }
    if (singleAssetOut == "*")
      singleAssetOut = String(0, "%s.%s.%s", a->getName(), a->getTypeStr(), strcmp(a->getTypeStr(), "tex") == 0 ? "dds" : "bin");
    checkAssetReferences(a, tmp_refs);

    IDagorAssetExporter *e = mgr.getAssetExporter(a->getType());
    if (!e)
    {
      log.addMessage(ILogWriter::ERROR, "can't get exporter plugin for %s (type=%s)", a->getName(), a->getTypeStr());
      return 1;
    }
    DataBlock aux_result_blk;
    e->setBuildResultsBlk(&aux_result_blk);
    Tab<SimpleString> files;
    e->gatherSrcDataFiles(*a, files);
    debug("%s depends on %d file(s)", a->getName(), files.size());
    for (SimpleString &s : files)
      debug("  %s", s);

    mkbindump::BinDumpSaveCB cwr(16 << 10, targets[0], false);
    cwr.setProfile(assets_profile);
    if (singleAssetFastBuild)
      cwr.setFastBuildFlag(true);
    int t0 = get_time_msec();
    if (!e->exportAsset(*a, cwr, log))
    {
      log.addMessage(ILogWriter::ERROR, "can't build asset <%s> data", a->getName());
      return 1;
    }
    e->setBuildResultsBlk(nullptr);
    t0 = get_time_msec() - t0;

    if (!singleAssetOut.empty())
    {
      dd_mkpath(singleAssetOut);
      FullFileSaveCB fcwr(singleAssetOut);
      if (!fcwr.fileHandle)
      {
        printf("can't create %s to build asset <%s>\n", singleAssetOut.str(), singleAsset.str());
        return 1;
      }
      cwr.copyDataTo(fcwr);
      printf("asset <%s> successfuly built%s to %s: sz=%d (for %.3f sec)\n", singleAsset.str(), singleAssetFastBuild ? "(fast)" : "",
        singleAssetOut.str(), cwr.getSize(), t0 / 1000.f);
    }
    else
      printf("asset <%s> successfuly built%s: sz=%d (for %.3f sec)\n", singleAsset.str(), singleAssetFastBuild ? "(fast)" : "",
        cwr.getSize(), t0 / 1000.f);
    if (aux_result_blk.blockCount() + aux_result_blk.paramCount() > 0)
    {
      if (!singleAssetOut.empty())
        aux_result_blk.saveToTextFile(String::mk_str_cat(singleAssetOut, ".desc.blk"));

      DynamicMemGeneralSaveCB cwr(tmpmem, 0, 64 << 10);
      aux_result_blk.saveToTextStream(cwr);
      printf("--- build results:\n%.*s\n", (int)cwr.size(), cwr.data());
    }

    fflush(stdout);
  }
  if (singleAssetPairsList.size())
    return 0;

  if (maintTexOp)
  {
    Tab<SimpleString> list;
    list.reserve(256 << 10);

    find_files_in_folder(list, cacheDdsxBase, ".bin", false, true);
    log.addMessage(log.NOTE, "scanned  %d files in %s", list.size(), cacheDdsxBase);

    OAHashNameMap<true> names;
    Tab<int> name2list;
    name2list.reserve(list.size());
    uint64_t tc_storage = 0;
    for (const auto &fn : list)
      for (int t = 0; t < targets.size(); t++)
        if (trail_stricmp(fn, String(260, "%s.c4.bin", mkbindump::get_target_str(targets[t], tc_storage))) ||
            trail_stricmp(fn, String(260, "%s.ct.bin", mkbindump::get_target_str(targets[t], tc_storage))))
        {
          int id = names.addNameId(dd_get_fname(fn));
          if (id == name2list.size()) // must be always true!
            name2list.push_back(&fn - list.cbegin());
          break;
        }

    dag::ConstSpan<DagorAsset *> assets = mgr.getAssets();
    size_t tex_asset_num = 0;
    String cfn;
    cfn.reserve(260);

    for (int t = 0; t < targets.size(); t++)
    {
      const char *ts = mkbindump::get_target_str(targets[t], tc_storage);
      for (int i = 0; i < assets.size(); i++)
      {
        int fidx = assets[i]->getFolderIndex();
        int a_type = assets[i]->getType();

        if (!(mgr.getFolder(fidx).flags & DagorAssetFolder::FLG_EXPORT_ASSETS))
          continue;
        if (a_type != mgr.getTexAssetTypeId())
          continue;

        tex_asset_num++;

        cfn.printf(260, "%s.ddsx.%s.c4.bin", assets[i]->getName(), ts);
        int nid = names.getNameId(cfn);
        if (nid >= 0)
          name2list[nid] = -1;

        cfn.printf(260, "%s.ddsx.%s.ct.bin", assets[i]->getName(), ts);
        nid = names.getNameId(cfn);
        if (nid >= 0)
          name2list[nid] = -1;
      }
    }
    size_t unused_cache_files = name2list.size();
    for (int idx : name2list)
      if (idx == -1)
        unused_cache_files--;
    log.addMessage(log.NOTE, "found    %d cache-files for %d tex assets", name2list.size(), tex_asset_num);

    log.addMessage(log.NOTE, "detected %d extra tex:", unused_cache_files);
    DagorStat buf;
    uint64_t total_sz = 0;
    for (int j : name2list)
    {
      if (j == -1)
        continue;
      log.addMessage(log.NOTE, "  extra tex: %s", list[j]);
      if (df_stat(list[j], &buf) == 0)
        total_sz += buf.size;
      if (maintTexOp == 2)
        ::dd_erase(list[j]);
    }
    log.addMessage(log.NOTE, "%s %uM (%llu bytes) total in %d extra tex", maintTexOp == 2 ? "cleaned" : "found", total_sz >> 20,
      total_sz, unused_cache_files);
  }

  if (maintPackOp)
  {
    for (int t = 0; t < targets.size(); t++)
      dabuild_list_extra_packs(app_dir, appblk, mgr, pkg_list, targets[t], assets_profile, log);
  }
  if (maintListOp == 1)
  {
    for (int t = 0; t < targets.size(); t++)
      dabuild_list_packs(app_dir, appblk, mgr, pkg_list, targets[t], assets_profile, export_tex, export_res, packs_re, log);
    if (listAssetPropsForTypes.nameCount())
    {
      Tab<bool> show_props;
      show_props.resize(mgr.getAssetTypesCount());
      mem_set_0(show_props);
      iterate_names(listAssetPropsForTypes, [&](int, const char *name) {
        if (strcmp(name, "*") == 0)
          mem_set_ff(show_props);
        else
        {
          int atype = mgr.getAssetTypeId(name);
          if (atype >= 0 && atype < show_props.size())
            show_props[atype] = true;
          else
            post_error(mgr.getMsgPipe(), "bad asset type %s for %s", name, "-maintenance:listProps:");
        }
      });

      DynamicMemGeneralSaveCB cwr(tmpmem, 0, 64 << 10);
      for (DagorAsset *a : mgr.getAssets())
        if (unsigned(a->getType()) < show_props.size() && show_props[a->getType()])
        {
          printf("\n--- asset <%s> props:\ntarget=%s\n", a->getNameTypified().c_str(), a->getTargetFilePath().c_str());
          cwr.resize(0);
          dblk::print_to_text_stream_limited(a->props, cwr, 0x7FFFFF, 16, 0);
          printf("%.*s", (int)cwr.size(), cwr.data());
        }
    }
  }
  else if (maintListOp == 2)
  {
    log.addMessage(ILogWriter::NOTE, "checking references for %d assets...", mgr.getAssets().size());
    int bad_refs = findBadReferences(mgr.getAssets(), mgr.getTexAssetTypeId(), tmp_refs);
    if (bad_refs)
      post_error(mgr.getMsgPipe(), "found %d bad references in asset base!", bad_refs);
    else
      log.addMessage(ILogWriter::NOTE, "checked references, no problems found", mgr.getAssets().size());
    if (bad_refs && maintOnly)
      return 13;
  }

  if (maintOnly)
    return 0;

  if (rebuild)
  {
    if (arg.size() == 1 && export_tex && export_res)
      dabuild->destroyCache(targets, assets_profile);
    else // remove only specified packs
    {
      String mask;

      for (int i = 0; i < targets.size(); i++)
      {
        log.addMessage(ILogWriter::NOTE, "clear cache for platform '%c%c%c%c'", _DUMP4C(targets[i]));
        for (int j = 1; j < arg.size(); j++)
        {
          debug("remove %s", cacheBase + arg[j]);
          ::dd_erase(cacheBase + arg[j]);
        }

        uint64_t tc_storage = 0;
        const char *tc_str = mkbindump::get_target_str(targets[i], tc_storage);
        if (export_tex && arg.size() == 1)
        {
          mask.printf(128, "*.dxp.bin.%s.*", tc_str);
          clearCache(cacheBase, mask);
          mask.printf(128, "*.ddsx.%s.*", tc_str);
          clearCache(cacheBase, mask);
        }
        if (export_res && arg.size() == 1)
        {
          mask.printf(128, "*.grp.%s.*", tc_str);
          clearCache(cacheBase, mask);
        }

        log.addMessage(ILogWriter::NOTE, "complete");
      }
    }
  }

  // start dabuild jobs
  jobMem = NULL;
  if (jobs)
  {
    AtomicPrintfMutex::init("dabuild", __argv[0]);

    // create file mapping
#if _TARGET_PC_WIN
    int pid = GetProcessId(GetCurrentProcess());
#elif _TARGET_PC_LINUX | _TARGET_PC_MACOSX
    pid_t pid = getpid();
#endif
    shared_mem_fn.printf(0, "daBuild-shared-mem-%d", pid);
    intptr_t hMapFile = 0;
    jobMem = (DabuildJobSharedMem *)create_global_map_shared_mem(shared_mem_fn, nullptr, sizeof(DabuildJobSharedMem), hMapFile);
    debug("%d jobs: %s -> 0x%x (sz=%d)", jobs, shared_mem_fn.str(), hMapFile, sizeof(DabuildJobSharedMem));

    if (jobMem)
    {
      memset(jobMem, 0, sizeof(DabuildJobSharedMem));
      jobMem->fullMemSize = sizeof(DabuildJobSharedMem);
      jobMem->mapHandle = hMapFile;
      jobMem->pid = pid;
      jobMem->jobCount = jobs;
      jobMem->dryRun = dabuild_dry_run;
      jobMem->stopOnFirstError = dabuild_stop_on_first_error;
      jobMem->stripD3Dres = dabuild_strip_d3d_res;
      jobMem->collapsePacks = dabuild_collapse_packs;
      jobMem->logLevel = quiet ? log.ERROR : log.NOTE;
      jobMem->nopbar = nopbar;
      jobMem->quiet = dgs_execute_quiet;
      jobMem->showImportantWarnings = show_important_warnings;
      jobMem->expTex = export_tex;
      jobMem->expRes = export_res;
      jobMem->forceRebuildAssetIdxCount = force_rebuild_assets.size();
      G_ASSERTF(jobMem->forceRebuildAssetIdxCount <= countof(DabuildJobSharedMem::forceRebuildAssetIdx),
        "forceRebuildAssetIdxCount=%d max=%d", jobMem->forceRebuildAssetIdxCount, countof(DabuildJobSharedMem::forceRebuildAssetIdx));
      mem_copy_to(force_rebuild_assets.getList(), jobMem->forceRebuildAssetIdx);
      if (int cnt = jobMem->forceRebuildAssetIdxCount)
      {
        log.addMessage(log.NOTE, "force rebuild of %d asset(s):", cnt);
        for (int i = 0, cnt = jobMem->forceRebuildAssetIdxCount; i < cnt; i++)
          log.addMessage(log.NOTE, "  [%d] %s", i, mgr.getAsset(jobMem->forceRebuildAssetIdx[i]).getNameTypified());
      }
      atexit(release_job_mem);
      signal(SIGINT, ctrl_break_handler);
    }
    else
      jobs = 0;

    if (jobMem)
      AssetExportCache::setJobSharedMem(jobMem);

    String dabuild_job_nm(0, "%s/%s", start_dir, strstr(__argv[0], "-asan-") ? "daBuild-job-asan-dev" : "daBuild-job-dev");
#if _TARGET_PC_WIN
    dabuild_job_nm += ".exe";
#endif
    for (int i = 0; i < jobs; i++)
    {
#if _TARGET_PC_WIN
      PROCESS_INFORMATION pi;
      STARTUPINFO si;

      ::ZeroMemory(&si, sizeof(STARTUPINFO));
      si.cb = sizeof(STARTUPINFO);
      String cmd(260, "%s %s %d %d", dabuild_job_nm, arg[0], pid, i);
      iterate_names(rebuild_types, [&cmd](int, const char *name) { cmd.aprintf(16, " %s", name); });

      if (::CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) // DETACHED_PROCESS
      {
        CloseHandle(pi.hThread);
        jobMem->jobHandle[i] = pi.hProcess;
      }
      else
        debug("FAILED to launch process: %s", cmd.str());
#elif _TARGET_PC_LINUX | _TARGET_PC_MACOSX
      String pid_str(0, "%d", pid), i_str(0, "%d", i);

      Tab<const char *> arguments;
      arguments.push_back(dabuild_job_nm);
      arguments.push_back(arg[0]);
      arguments.push_back(pid_str);
      arguments.push_back(i_str);
      arguments.push_back(nullptr);
      if (posix_spawn(&jobMem->jobHandle[i], arguments[0], nullptr, nullptr, (char *const *)arguments.data(), environ) != 0)
        debug("FAILED to launch process: %s %s %s %d", arguments[0], arguments[1], arguments[2], arguments[3]);
#endif
    }
    if (quiet)
      log.level = log.NOTE;
    log.addMessage(log.NOTE, "Distributed build with %d jobs", jobs);
    if (quiet)
      log.level = log.ERROR;
  }

  // export assets
  bool success = dabuild->exportRaw(targets, make_span_const(arg).subspan(1), packs_re, export_tex, export_res, assets_profile);

  dabuild->unloadExporterPlugins();
  dabuild->setupReports(NULL, NULL);
  pbar.destroy();

  log.level = log.NOTE;

  int total, built, failed, packs_removed;
  dabuild->getStat(true, total, built, failed);
  if (total)
    log.addMessage(log.NOTE,
      "processed %3d texture pack(s), %d updated, %d failed, %d unchanged, size change %+lldK (size of changed packs %lldM)", total,
      built, failed, stat_tex_unchanged, stat_tex_sz_diff / 1024, stat_changed_tex_total_sz >> 20);

  dabuild->getStat(false, total, built, failed);
  if (total)
    log.addMessage(log.NOTE,
      "processed %3d gameres pack(s), %d updated, %d failed, %d unchanged, size change %+lldK (size of changed packs %lldM)", total,
      built, failed, stat_grp_unchanged, stat_grp_sz_diff / 1024, stat_changed_grp_total_sz >> 20);

  packs_removed = dabuild->getRemovedPackCount();
  if (packs_removed)
    log.addMessage(log.NOTE, "removed %d old pack(s)", packs_removed);

  AssetExportCache::sharedDataResetRebuildTypesList();
  AssetExportCache::saveSharedData();
  dabuild->term();

  dabuild->release();
  release_job_mem();
  ATOMIC_PRINTF("\n");
  if (show_important_warnings && log.impWarnCnt)
    os_message_box(String(2048, "daBuild registered %d serious warnings%s:\n\n%s", log.impWarnCnt,
                     log.impWarnCnt > 20 ? ";\nfirst 20 assets are" : "", log.impWarn.str()),
      "daBuild warnings", GUI_MB_OK | GUI_MB_ICON_ERROR);
  return !success;
}

static inline void clearCache(const String &path, const char *mask)
{
  String mp(512, "%s%s", path.str(), mask);

  for (const alefind_t &ff : dd_find_iterator(mp, DA_FILE))
  {
    debug("remove %s", path + ff.name);
    ::dd_erase(path + ff.name);
  }

  mp = path + "*";
  for (const alefind_t &ff : dd_find_iterator(mp, DA_SUBDIR))
  {
    clearCache(path + ff.name + "/", mask);
    ::dd_rmdir(path + ff.name);
  }
}

static int findBadReferences(dag::ConstSpan<DagorAsset *> assets, int tex_atype, Tab<IDagorAssetRefProvider::Ref> &tmp_refs)
{
  int bad_rc = 0;
  for (DagorAsset *a : assets)
    if (a && a->getType() != tex_atype) // subtle optimization: textures don't reference anything now
      bad_rc += checkAssetReferences(a, tmp_refs);
  return bad_rc;
}
static int checkAssetReferences(DagorAsset *a, Tab<IDagorAssetRefProvider::Ref> &tmp_refs)
{
  IDagorAssetRefProvider *refProvider = a->getMgr().getAssetRefProvider(a->getType());
  if (!refProvider)
    return 0;

  refProvider->getAssetRefs(*a, tmp_refs);
  int bad_rc = 0;

  for (const IDagorAssetRefProvider::Ref &r : tmp_refs)
  {
    if (r.getAsset())
      continue;

    if ((r.flags & IDagorAssetRefProvider::RFLG_BROKEN) || !(r.flags & IDagorAssetRefProvider::RFLG_OPTIONAL))
    {
      post_error(a->getMgr().getMsgPipe(), "asset '%s:%s' has invalid reference '%s'", a->getName(), a->getTypeStr(),
        r.getBrokenRef());
      bad_rc++;
    }
  }
  return bad_rc;
}
