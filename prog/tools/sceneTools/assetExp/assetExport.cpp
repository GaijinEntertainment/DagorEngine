// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildInterface.h>
#include "daBuild.h"
#include <assets/assetExpCache.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetExporter.h>
#include <assets/assetHlp.h>
#include <libTools/util/progressInd.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/makeBindump.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <generic/dag_span.h>
#include <generic/dag_smallTab.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <util/dag_simpleString.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <debug/dag_logSys.h>
#include <stdio.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_string.h>
#include "jobSharedMem.h"

static bool reqFastConv = false;
static debug_log_callback_t original_log_callback = nullptr;
static dag::Vector<String> *build_warnings = nullptr;

class WarningCheckingLogWriter : public ILogWriter
{
public:
  explicit WarningCheckingLogWriter(ILogWriter &original_log_writer) : originalLogWriter(original_log_writer) {}

  void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum) override
  {
    G_ASSERT(is_main_thread());

    if (type == ILogWriter::WARNING || type == ILogWriter::ERROR || type == ILogWriter::FATAL)
    {
      String message;
      message += getLogPrefix(type);
      message.avprintf(0, fmt, arg, anum);
      build_warnings->emplace_back(message);
    }

    originalLogWriter.addMessageFmt(type, fmt, arg, anum);
  }

  bool hasErrors() const override { return originalLogWriter.hasErrors(); }
  void startLog() override { originalLogWriter.startLog(); }
  void endLog() override { originalLogWriter.endLog(); }

  static char getLogPrefix(MessageType type)
  {
    switch (type)
    {
      case ILogWriter::NOTE: return 'N';
      case ILogWriter::WARNING: return 'W';
      case ILogWriter::ERROR: return 'E';
      case ILogWriter::FATAL: return 'F';
      case ILogWriter::REMARK: return 'R';
    }

    G_ASSERT(false);
    return ' ';
  }

  static MessageType getLogMessageType(char prefix)
  {
    switch (prefix)
    {
      case 'N': return ILogWriter::NOTE;
      case 'W': return ILogWriter::WARNING;
      case 'E': return ILogWriter::ERROR;
      case 'F': return ILogWriter::FATAL;
      case 'R': return ILogWriter::REMARK;
    }

    G_ASSERT(false);
    return ILogWriter::REMARK;
  }

private:
  ILogWriter &originalLogWriter;
};

static int warning_checking_log_callback(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  // AssetViewer only redirects errors to the console, so capture only those.
  // Ignore messages redirected by the console to log (these start with "CON: ") otherwise they would be added twice.
  if (lev_tag == LOGLEVEL_ERR && is_main_thread() && strncmp(fmt, "CON: ", 5) != 0)
  {
    String message;
    message += WarningCheckingLogWriter::getLogPrefix(ILogWriter::ERROR);
    message.avprintf(0, fmt, (const DagorSafeArg *)arg, anum);
    build_warnings->emplace_back(message);
  }

  return original_log_callback ? original_log_callback(lev_tag, fmt, arg, anum, ctx_file, ctx_line) : 1;
}

static bool warning_checking_build_asset(IDagorAssetExporter &exp, DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log,
  dag::Vector<String> &warnings)
{
  G_ASSERT(is_main_thread());

  const bool reentrance = build_warnings != nullptr;
  if (!reentrance)
    original_log_callback = debug_set_log_callback(&warning_checking_log_callback);

  dag::Vector<String> *originalBuildWarnings = build_warnings;
  build_warnings = &warnings;

  WarningCheckingLogWriter logWriter(log);
  const bool result = exp.exportAsset(a, cwr, logWriter);

  build_warnings = originalBuildWarnings;

  if (!reentrance)
  {
    debug_set_log_callback(original_log_callback);
    original_log_callback = nullptr;
  }

  return result;
}

static inline void clearCache(const String &path, const char *mask)
{
  alefind_t ff;

  String mp(512, "%s%s", path.str(), mask);

  for (const alefind_t &ff : dd_find_iterator(mp, DA_FILE))
    ::dd_erase(path + ff.name);

  mp = path + "*";
  for (const alefind_t &ff : dd_find_iterator(mp, DA_SUBDIR))
  {
    clearCache(path + ff.name + "/", mask);
    ::dd_rmdir(path + ff.name);
  }
}

static bool checkCacheChanged(AssetExportCache &c4, DagorAsset &a, IDagorAssetExporter *exp)
{
  bool curChanged = false;
  if (c4.checkDataBlockChanged(a.getNameTypified(), a.props))
    curChanged = true;
  if (c4.checkAssetExpVerChanged(a.getType(), exp->getGameResClassId(), exp->getGameResVersion()))
    curChanged = true;

  if (a.isVirtual() && c4.checkFileChanged(a.getTargetFilePath()))
    curChanged = true;

  Tab<SimpleString> a_files(tmpmem);
  exp->gatherSrcDataFiles(a, a_files);
  // A lot of assets add the asset itself to the list of src data files.
  // checkFileChanged is quite slow and we checked this asset already above.
  // Removing this asset from gather list is faster then calling checkFileChanged one extra time
  if (a.isVirtual() && a_files.size() > 0 && strcmp(a_files[0].c_str(), a.getTargetFilePath().c_str()) == 0)
    a_files.erase(a_files.begin());
  int cnt = a_files.size();
  for (int j = 0; j < cnt; j++)
    if (c4.checkFileChanged(a_files[j]))
      curChanged = true;

  return curChanged;
}

static bool is_asset_in_pack(dag::ConstSpan<AssetPack *> pack, int package_id, const char *target_str, const char *profile,
  const DagorAsset *asset)
{
  Tab<DagorAsset *> asset_list(tmpmem);
  for (const AssetPack *assetPack : pack)
  {
    if (!assetPack->exported || assetPack->packageId != package_id)
      continue;

    asset_list.clear();
    if (!get_exported_assets(asset_list, assetPack->assets, target_str, profile))
      continue;

    if (eastl::find(asset_list.begin(), asset_list.end(), asset) != asset_list.end())
      return true;
  }

  return false;
}

String get_name_of_package_containing_asset(const DataBlock &app_blk, DagorAssetMgr &mgr, unsigned target_code, const char *profile,
  ILogWriter &log, DagorAsset *asset)
{
  const DataBlock &expblk = *app_blk.getBlockByNameEx("assets")->getBlockByNameEx("export");
  Tab<bool> expTypesMask(tmpmem);
  make_exp_types_mask(expTypesMask, mgr, expblk, log);

  Tab<AssetPack *> grpPack(tmpmem), texPack(tmpmem);
  FastNameMapEx packages;
  uint64_t tc_storage = 0;
  const char *targetStr = mkbindump::get_target_str(target_code, tc_storage);
  preparePacks(mgr, make_span_const<DagorAsset *>(&asset, 1), expTypesMask, expblk, texPack, grpPack, packages, log,
    /*export_tex = */ true, /*export_res = */ true, targetStr, profile);

  String packageName;
  if (assethlp::validate_exp_blk(packages.nameCount() > 0, expblk, targetStr, profile, log))
  {
    for (int packageId = -1; packageId < packages.nameCount(); ++packageId)
    {
      if (is_asset_in_pack(texPack, packageId, targetStr, profile, asset) ||
          is_asset_in_pack(grpPack, packageId, targetStr, profile, asset))
      {
        packageName = packageId >= 0 ? packages.getName(packageId) : "MAIN";
        break;
      }
    }
  }

  clear_all_ptr_items(texPack);
  clear_all_ptr_items(grpPack);

  return packageName;
}

static QuietProgressIndicator null_pbar;
static class NullLogWriter : public ILogWriter
{
  bool err;

public:
  NullLogWriter() : err(false) {}

  void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum) override
  {
    if (type == ERROR || type == FATAL)
      err = true;
  }
  bool hasErrors() const override { return err; }

  void startLog() override {}
  void endLog() override {}
} null_log;

class AssetExport : public IDaBuildInterface
{
public:
  AssetExport() : mgr(NULL), pbar(&null_pbar), log(&null_log) {}

  void __stdcall release() override {}

  int __stdcall init(const char *startdir, DagorAssetMgr &m, const DataBlock &appblk, const char *appdir,
    const char *ddsxPluginsPath = nullptr) override
  {

    appBlk.setFrom(&appblk, appblk.resolveFilename());

    simplifyFnameParam(appBlk, "appDir");
    simplifyFnameParam(appBlk, "shadersAbs");
    simplifyFnameParam(appBlk, "dagorCdkDir");
    setup_dxp_grp_write_ver(*appBlk.getBlockByNameEx("assets")->getBlockByNameEx("build"), *log);

    cleanupAppBlk();

    appDir = appdir;
    startDir = startdir;
    mgr = &m;

    // setup asset types allowed for export
    int exp_cnt = make_exp_types_mask(expTypesMask, *mgr, *appBlk.getBlockByNameEx("assets")->getBlockByNameEx("export"), *log);
    texTypeId = m.getTexAssetTypeId();

#if _TARGET_STATIC_LIB
    loadExporterPlugins();
#endif
    // load DDSx export plugins
    const char *ddsxPluginsPathBase = "plugins/ddsx";
    ddsx::load_plugins(String(260, "%s/%s", startDir.str(), ddsxPluginsPath ? ddsxPluginsPath : ddsxPluginsPathBase));

    // init texture conversion cache
    texconvcache::init(m, appblk, startdir, dabuild_dry_run, reqFastConv);
    DEBUG_CTX("texconvcache::init for reqFastConv=%d", reqFastConv);

    return exp_cnt;
  }
  void __stdcall term() override
  {
    texconvcache::term();
    mgr = NULL;
    appBlk.reset();
    appDir = NULL;
  }

  void __stdcall setupReports(ILogWriter *l, IGenericProgressIndicator *pb) override
  {
    log = l ? l : &null_log;
    pbar = pb ? pb : &null_pbar;
  }

  bool __stdcall loadExporterPlugins() override
  {
    // load exporter plugins
    const DataBlock &blk = *appBlk.getBlockByNameEx("assets")->getBlockByNameEx("export")->getBlockByNameEx("plugins");
    int nid_folder = blk.getNameId("folder"), nid_plugin = blk.getNameId("plugin");

    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamType(i) == DataBlock::TYPE_STRING && blk.getParamNameId(i) == nid_folder)
      {
        const char *pfolder = blk.getStr(i);
        bool ret;
        if (stricmp(pfolder, "*common") == 0)
          ret = loadExporterPluginsInFolder(String(260, "%s/plugins/dabuild", startDir.str()));
        else
          ret = loadExporterPluginsInFolder(String(260, "%s/%s", appDir.str(), pfolder));

        if (!ret)
          return false;
      }
      else if (blk.getParamType(i) == DataBlock::TYPE_STRING && blk.getParamNameId(i) == nid_plugin)
      {
        if (!loadSingleExporterPlugin(String(260, "%s/%s", appDir.str(), blk.getStr(i))))
          return false;
      }

    return true;
  }
  bool __stdcall loadExporterPluginsInFolder(const char *dirpath) override
  {
    return ::loadExporterPlugins(appBlk, *mgr, dirpath, expTypesMask, *log);
  }
  bool __stdcall loadSingleExporterPlugin(const char *dll_path) override
  {
    return ::loadSingleExporterPlugin(appBlk, *mgr, dll_path, expTypesMask, *log);
  }
  void __stdcall unloadExporterPlugins() override
  {
    if (mgr)
      ::unloadExporterPlugins(*mgr);
  }

  bool __stdcall exportAll(dag::ConstSpan<unsigned> tc, const char *profile) override
  {
    bool ret = true;
    texconvcache::set_fast_conv(false);
    for (int i = 0; i < tc.size(); i++)
      if (!::exportAssets(*mgr, appDir, tc[i], profile, {}, {}, expTypesMask, appBlk, *pbar, *log))
        ret = false;
    texconvcache::set_fast_conv(reqFastConv);
    return ret;
  }
  bool __stdcall exportPacks(dag::ConstSpan<unsigned> tc, dag::ConstSpan<const char *> packs, const char *profile) override
  {
    bool ret = true;
    texconvcache::set_fast_conv(false);
    for (int i = 0; i < tc.size(); i++)
    {
      if (!::exportAssets(*mgr, appDir, tc[i], profile, packs, {}, expTypesMask, appBlk, *pbar, *log))
        ret = false;
    }
    texconvcache::set_fast_conv(reqFastConv);
    return ret;
  }
  bool __stdcall exportByFolders(dag::ConstSpan<unsigned> tc, dag::ConstSpan<const char *> folders, const char *profile) override
  {
    return false;
  }

  bool __stdcall exportRaw(dag::ConstSpan<unsigned> tc, dag::ConstSpan<const char *> packs, dag::ConstSpan<const char *> packs_re,
    bool export_tex, bool export_res, const char *profile) override
  {
    bool ret = true;
    texconvcache::set_fast_conv(false);
    for (int i = 0; i < tc.size(); i++)
    {
      if (!::exportAssets(*mgr, appDir, tc[i], profile, packs, packs_re, expTypesMask, appBlk, *pbar, *log, export_tex, export_res))
        ret = false;
    }
    texconvcache::set_fast_conv(reqFastConv);
    return ret;
  }

  void __stdcall resetStat() override
  {
    stat_grp_total = stat_grp_built = stat_grp_failed = 0;
    stat_tex_total = stat_tex_built = stat_tex_failed = 0;
    stat_grp_unchanged = stat_tex_unchanged = 0;
    stat_pack_removed = 0;
    stat_grp_sz_diff = stat_tex_sz_diff = 0;
    stat_changed_grp_total_sz = stat_changed_tex_total_sz = 0;
  }
  void __stdcall getStat(bool tex_pack, int &processed, int &built, int &failed) override
  {
    if (tex_pack)
    {
      processed = stat_tex_total;
      built = stat_tex_built;
      failed = stat_tex_failed;
    }
    else
    {
      processed = stat_grp_total;
      built = stat_grp_built;
      failed = stat_grp_failed;
    }
  }
  int __stdcall getRemovedPackCount() override { return stat_pack_removed; }

  const char *__stdcall getPackName(DagorAsset *asset) override
  {
    if (expTypesMask[asset->getType()])
      return asset->getDestPackName();
    else
      return NULL;
  }

  const char *__stdcall getPackNameFromFolder(int fld_idx, bool tex_or_res) override
  {
    return mgr->getFolderPackName(fld_idx, tex_or_res, NULL);
  }

  String __stdcall getPkgName(DagorAsset *asset) override
  {
    return get_name_of_package_containing_asset(appBlk, *mgr, /*target_code = */ _MAKE4C('PC'), /*profile = */ nullptr, *log, asset);
  }

  bool __stdcall checkUpToDate(dag::ConstSpan<unsigned> tc, dag::Span<int> tc_flags, dag::ConstSpan<const char *> packs_to_check,
    const char *profile) override
  {
    bool ret = true;
    for (int i = 0; i < tc.size(); i++)
      if (!::checkUpToDate(*mgr, appDir, tc[i], profile, tc_flags[i], packs_to_check, expTypesMask, appBlk, *pbar, *log))
        ret = false;
    return ret;
  }

  bool __stdcall isAssetExportable(DagorAsset *asset) override { return ::isAssetExportable(*mgr, asset, expTypesMask); }

  void __stdcall destroyCache(dag::ConstSpan<unsigned> tc, const char *profile) override
  {
    if (profile && !*profile)
      profile = NULL;

    const DataBlock &expblk = *appBlk.getBlockByNameEx("assets")->getBlockByNameEx("export");
    String cacheBase(260, profile ? "%s/%s/~%s%s/" : "%s/%s/", appDir.str(), expblk.getStr("cache", "/develop/.cache"), profile,
      dabuild_strip_d3d_res ? "~strip_d3d_res" : "");

    int pcnt = tc.size();
    simplify_fname(cacheBase);

    for (int i = 0; i < pcnt; i++)
    {
      log->addMessage(ILogWriter::NOTE, "clear cache for platform '%c%c%c%c'", _DUMP4C(tc[i]));
      uint64_t tc_storage = 0;
      String mask(128, "*.%s.*", mkbindump::get_target_str(tc[i], tc_storage));
      clearCache(cacheBase, mask);

      log->addMessage(ILogWriter::NOTE, "complete");
    }
  }


  void __stdcall invalidateBuiltRes(const DagorAsset &a, const char *cache_folder) override
  {
    String cacheFname(260, "%s/%s~%s.c4.bin", cache_folder, a.getName(), a.getTypeStr());
    dd_erase(cacheFname);
  }

  bool __stdcall getBuiltRes(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, IDagorAssetExporter *exp, const char *cache_folder,
    String &cache_path, int &data_offset, bool save_all_caches) override
  {
    cache_path = String(260, "%s/%s~%s.c4.bin", cache_folder, a.getName(), a.getTypeStr());

    AssetExportCache c4;
    G_ASSERT(cwr.getSize() == 0); // fixme

    int cacheEndPos = 0;
    bool loaded = c4.load(cache_path, *mgr, &cacheEndPos);

    // cacheEndPos from crd.seekto(cacheEndPos); sizeof(int) from crd.beginBlock();
    data_offset = cacheEndPos + sizeof(int);

    if (loaded && !checkCacheChanged(c4, a, exp))
    {
      FullFileLoadCB crd(cache_path);
      if (crd.fileHandle && (crd.readInt() == _MAKE4C('fC1')) && (crd.readInt() >= 2))
      {
        crd.seekto(cacheEndPos);

        int sz = crd.beginBlock();
        if (sz > 0)
        {
          cwr.copyRaw(crd, sz);
          crd.endBlock();
          return true;
        }
      }
    }

    int64_t ref = ref_time_ticks();
    dag::Vector<String> warnings;
    bool res = is_main_thread() ? warning_checking_build_asset(*exp, a, cwr, *log, warnings) : exp->exportAsset(a, cwr, *log);
    if (!res)
      return false;

    if (!save_all_caches && get_time_usec(ref) < 5 * 1000)
    {
      cache_path.clear();
      data_offset = 0;
      return true;
    }

    if (!loaded)
      checkCacheChanged(c4, a, exp);

    c4.setAssetExpVer(a.getType(), exp->getGameResClassId(), exp->getGameResVersion());
    c4.setWarnings(warnings);
    c4.save(cache_path, *mgr, &cwr, &data_offset);

    data_offset += sizeof(int); // see first data_offset caclulation

    return true;
  }

  bool __stdcall getBuiltResWarnings(DagorAsset &asset, IDagorAssetExporter &exporter, const char *cache_folder,
    dag::Vector<BuildWarning> &warnings, bool &cache_up_to_date) override
  {
    const String cache_path(260, "%s/%s~%s.c4.bin", cache_folder, asset.getName(), asset.getTypeStr());

    AssetExportCache c4;
    if (!c4.load(cache_path, *mgr))
    {
      cache_up_to_date = false;
      return false;
    }

    for (const String &message : c4.getWarnings())
      if (!message.empty())
      {
        BuildWarning warning;
        warning.messageType = WarningCheckingLogWriter::getLogMessageType(message[0]);
        warning.message = message.c_str() + 1;
        warnings.emplace_back(warning);
      }

    cache_up_to_date = !checkCacheChanged(c4, asset, &exporter);
    return true;
  }

  void __stdcall setExpCacheSharedData(void *p) override { AssetExportCache::setSharedDataPtr(p); }

  void __stdcall processSrcHashForDestPacks() override
  {
    SimpleString hash;
    for (DagorAsset *a : mgr->getAssets())
      if (a->props.getBool("useIndividualPackWithSrcHash", false))
        if (IDagorAssetExporter *exp = mgr->getAssetExporter(a->getType()))
          if (exp->getAssetSourceHash(hash, *a, AssetExportCache::getSharedDataPtr(), _MAKE4C('PC')))
          {
            const char *a_name = a->getName();
            const char *suf = strrchr(a_name, '$');

            a->props.setStr(a->getType() == mgr->getTexAssetTypeId() ? "ddsxTexPack" : "gameResPack",
              String(0, "/%.2s/%.*s-%s", hash, suf ? suf - a_name : strlen(a_name), a->getName(), hash));
          }
  }

  void cleanupAppBlk()
  {
    bool had_obsolete_blocks = false;
    if (appBlk.getBlockByName("resources"))
    {
      DataBlock *res_blk = appBlk.getBlockByName("resources");
      bool bad_res_blk = res_blk->blockCount() > 0;
      for (int i = 0; i < res_blk->paramCount(); i++)
        if (strcmp(res_blk->getParamName(i), "save_optimized_render_inst") != 0 && strcmp(res_blk->getParamName(i), "gridSize") != 0 &&
            strcmp(res_blk->getParamName(i), "subGridSize") != 0)
          bad_res_blk = true;
      if (!bad_res_blk)
        goto skip;

      logerr("application.blk has obsolete \"resources\" block; "
             "use assets/build/<asset_type> blocks instead");
      if (!appBlk.getBlockByNameEx("assets")->getBlockByName("build"))
      {
        DataBlock *build_blk = appBlk.addBlock("assets")->addBlock("build");
        if (res_blk->paramExists("anim_states_graph_outdir"))
          build_blk->addBlock("stateGraph")->setStr("cppOutDir", res_blk->getStr("anim_states_graph_outdir", ""));

        if (res_blk->paramExists("useExtraPrepareBillboardMesh"))
        {
          build_blk->addBlock("dynModel")
            ->setBool("useExtraPrepareBillboardMesh", res_blk->getBool("useExtraPrepareBillboardMesh", false));
          build_blk->addBlock("rendInst")
            ->setBool("useExtraPrepareBillboardMesh", res_blk->getBool("useExtraPrepareBillboardMesh", false));
        }
        if (res_blk->paramExists("ignoreMappingInPrepareBillboardMesh"))
        {
          build_blk->addBlock("dynModel")
            ->setBool("ignoreMappingInPrepareBillboardMesh", res_blk->getBool("ignoreMappingInPrepareBillboardMesh", false));
          build_blk->addBlock("rendInst")
            ->setBool("ignoreMappingInPrepareBillboardMesh", res_blk->getBool("ignoreMappingInPrepareBillboardMesh", false));
        }
        if (res_blk->paramExists("dynModel_enableMeshNodeCollapse"))
          build_blk->addBlock("dynModel")
            ->setBool("enableMeshNodeCollapse", res_blk->getBool("dynModel_enableMeshNodeCollapse", false));
        if (res_blk->getBlockByName("rendInstShadersRemap"))
          build_blk->addBlock("rendInst")->addNewBlock(res_blk->getBlockByName("rendInstShadersRemap"), "remapShaders");

        if (res_blk->getBlockByName("PC"))
          build_blk->addBlock("rendInst")->addNewBlock(res_blk->getBlockByName("PC"), "PC");
        if (res_blk->getBlockByName("X360"))
          build_blk->addBlock("rendInst")->addNewBlock(res_blk->getBlockByName("X360"), "X360");
        if (res_blk->getBlockByName("PS3"))
          build_blk->addBlock("rendInst")->addNewBlock(res_blk->getBlockByName("PS3"), "PS3");
      }

      while (appBlk.removeBlock("resources")) {}
      had_obsolete_blocks = true;
    skip:;
    }
    if (appBlk.getBlockByName("resedit_add_out_dir"))
    {
      logerr("application.blk has obsolete \"resedit_add_out_dir\" block, removing");
      while (appBlk.removeBlock("resedit_add_out_dir")) {}
      had_obsolete_blocks = true;
    }
    if (appBlk.getBlockByName("reseditor_disabled_plugins"))
    {
      logerr("application.blk has obsolete \"reseditor_disabled_plugins\" block, removing");
      while (appBlk.removeBlock("reseditor_disabled_plugins")) {}
      had_obsolete_blocks = true;
    }
    if (appBlk.getBlockByNameEx("SDK")->paramExists("lib_folder"))
    {
      logerr("application.blk has obsolete \"SDK/lib_folder\" str, removing");
      while (appBlk.getBlockByName("SDK")->removeParam("lib_folder")) {}
      had_obsolete_blocks = true;
    }
    if (appBlk.getBlockByNameEx("SDK")->paramExists("resource_database_folder"))
    {
      logerr("application.blk has obsolete \"SDK/resource_database_folder\" str, removing");
      while (appBlk.getBlockByName("SDK")->removeParam("resource_database_folder")) {}
      had_obsolete_blocks = true;
    }
    if (appBlk.getBlockByNameEx("SDK")->getBlockByName("additional_tex_folders"))
    {
      logerr("application.blk has obsolete \"SDK/additional_tex_folders\" block, removing");
      while (appBlk.getBlockByName("SDK")->removeBlock("additional_tex_folders")) {}
      had_obsolete_blocks = true;
    }

    if (had_obsolete_blocks)
    {
      bool strip_d3dres = appBlk.getBool("strip_d3dres", false);
      bool collapse_packs = appBlk.getBool("collapse_packs", false);
      appBlk.removeParam("strip_d3dres");
      appBlk.removeParam("collapse_packs");
      appBlk.saveToTextFile("converted_app.blk");
      appBlk.setBool("strip_d3dres", strip_d3dres);
      appBlk.setBool("collapse_packs", collapse_packs);
    }
  }
  static void simplifyFnameParam(DataBlock &b, const char *n)
  {
    const char *s = b.getStr(n);
    if (s)
    {
      SimpleString str(s);
      b.setStr(n, simplify_fname(str));
    }
  }

protected:
  DagorAssetMgr *mgr;
  DataBlock appBlk;
  SimpleString appDir, startDir;
  Tab<bool> expTypesMask;
  int texTypeId = -1;
  ILogWriter *log;
  IGenericProgressIndicator *pbar;
};

static AssetExport daBuildExp;

#if _TARGET_DABUILD_STATIC
#define DABUILD_DLL_API extern "C"
#elif _TARGET_PC_WIN
#define DABUILD_DLL_API extern "C" __declspec(dllexport)
#else
#define DABUILD_DLL_API extern "C" __attribute__((visibility("default")))
#endif

DABUILD_DLL_API IDaBuildInterface *__stdcall get_dabuild_interface()
{
#if !_TARGET_DABUILD_STATIC
  reqFastConv = true;
#endif
  return &daBuildExp;
}
