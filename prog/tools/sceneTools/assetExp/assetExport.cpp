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
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <stdio.h>
#include <perfMon/dag_cpuFreq.h>
#include <util/dag_string.h>
#include "jobSharedMem.h"

static bool reqFastConv = false;

static inline void clearCache(const String &path, const char *mask)
{
  alefind_t ff;

  String mp(512, "%s%s", path.str(), mask);

  for (bool ok = ::dd_find_first(mp, DA_FILE, &ff); ok; ok = ::dd_find_next(&ff))
    ::dd_erase(path + ff.name);
  ::dd_find_close(&ff);

  mp = path + "*";
  for (bool ok = ::dd_find_first(mp, DA_SUBDIR, &ff); ok; ok = ::dd_find_next(&ff))
    clearCache(path + ff.name + "/", mask);
  ::dd_rmdir(path + ff.name);
  ::dd_find_close(&ff);
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
  int cnt = a_files.size();
  for (int j = 0; j < cnt; j++)
    if (c4.checkFileChanged(a_files[j]))
      curChanged = true;

  return curChanged;
}

static QuietProgressIndicator null_pbar;
static class NullLogWriter : public ILogWriter
{
  bool err;

public:
  NullLogWriter() : err(false) {}

  virtual void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum)
  {
    if (type == ERROR || type == FATAL)
      err = true;
  }
  virtual bool hasErrors() const { return err; }

  virtual void startLog() {}
  virtual void endLog() {}
} null_log;

class AssetExport : public IDaBuildInterface
{
public:
  AssetExport() : mgr(NULL), pbar(&null_pbar), log(&null_log) {}

  virtual void __stdcall release() {}

  virtual int __stdcall init(const char *startdir, DagorAssetMgr &m, const DataBlock &appblk, const char *appdir,
    const char *ddsxPluginsPath = nullptr)
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
#if _TARGET_64BIT
    const char *ddsxPluginsPathBase = "../bin64/plugins/ddsx";
#else
    const char *ddsxPluginsPathBase = "../bin/plugins/ddsx";
#endif
    ddsx::load_plugins(String(260, "%s/%s", startDir.str(), ddsxPluginsPath ? ddsxPluginsPath : ddsxPluginsPathBase));

    // init texture conversion cache
    texconvcache::init(m, appblk, startdir, dabuild_dry_run, reqFastConv);
    debug_ctx("texconvcache::init for reqFastConv=%d", reqFastConv);

    return exp_cnt;
  }
  virtual void __stdcall term()
  {
    texconvcache::term();
    mgr = NULL;
    appBlk.reset();
    appDir = NULL;
  }

  virtual void __stdcall setupReports(ILogWriter *l, IGenericProgressIndicator *pb)
  {
    log = l ? l : &null_log;
    pbar = pb ? pb : &null_pbar;
  }

  virtual bool __stdcall loadExporterPlugins()
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
  virtual bool __stdcall loadExporterPluginsInFolder(const char *dirpath)
  {
    return ::loadExporterPlugins(appBlk, *mgr, dirpath, expTypesMask, *log);
  }
  virtual bool __stdcall loadSingleExporterPlugin(const char *dll_path)
  {
    return ::loadSingleExporterPlugin(appBlk, *mgr, dll_path, expTypesMask, *log);
  }
  virtual void __stdcall unloadExporterPlugins()
  {
    if (mgr)
      ::unloadExporterPlugins(*mgr);
  }

  virtual bool __stdcall exportAll(dag::ConstSpan<unsigned> tc, const char *profile)
  {
    bool ret = true;
    texconvcache::set_fast_conv(false);
    for (int i = 0; i < tc.size(); i++)
      if (!::exportAssets(*mgr, appDir, tc[i], profile, {}, {}, expTypesMask, appBlk, *pbar, *log))
        ret = false;
    texconvcache::set_fast_conv(reqFastConv);
    return ret;
  }
  virtual bool __stdcall exportPacks(dag::ConstSpan<unsigned> tc, dag::ConstSpan<const char *> packs, const char *profile)
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
  virtual bool __stdcall exportByFolders(dag::ConstSpan<unsigned> tc, dag::ConstSpan<const char *> folders, const char *profile)
  {
    return false;
  }

  virtual bool __stdcall exportRaw(dag::ConstSpan<unsigned> tc, dag::ConstSpan<const char *> packs,
    dag::ConstSpan<const char *> packs_re, bool export_tex, bool export_res, const char *profile)
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

  virtual void __stdcall resetStat()
  {
    stat_grp_total = stat_grp_built = stat_grp_failed = 0;
    stat_tex_total = stat_tex_built = stat_tex_failed = 0;
    stat_grp_unchanged = stat_tex_unchanged = 0;
    stat_pack_removed = 0;
    stat_grp_sz_diff = stat_tex_sz_diff = 0;
    stat_changed_grp_total_sz = stat_changed_tex_total_sz = 0;
  }
  virtual void __stdcall getStat(bool tex_pack, int &processed, int &built, int &failed)
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
  virtual int __stdcall getRemovedPackCount() { return stat_pack_removed; }

  virtual const char *__stdcall getPackName(DagorAsset *asset)
  {
    if (expTypesMask[asset->getType()])
      return asset->getDestPackName();
    else
      return NULL;
  }

  virtual const char *__stdcall getPackNameFromFolder(int fld_idx, bool tex_or_res)
  {
    return mgr->getFolderPackName(fld_idx, tex_or_res, NULL);
  }

  virtual bool __stdcall checkUpToDate(dag::ConstSpan<unsigned> tc, dag::Span<int> tc_flags,
    dag::ConstSpan<const char *> packs_to_check, const char *profile)
  {
    bool ret = true;
    for (int i = 0; i < tc.size(); i++)
      if (!::checkUpToDate(*mgr, appDir, tc[i], profile, tc_flags[i], packs_to_check, expTypesMask, appBlk, *pbar, *log))
        ret = false;
    return ret;
  }

  virtual bool __stdcall isAssetExportable(DagorAsset *asset) { return ::isAssetExportable(*mgr, asset, expTypesMask); }

  virtual void __stdcall destroyCache(dag::ConstSpan<unsigned> tc, const char *profile)
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
      String mask(128, "*.%s.*", mkbindump::get_target_str(tc[i]));
      clearCache(cacheBase, mask);

      log->addMessage(ILogWriter::NOTE, "complete");
    }
  }


  virtual void __stdcall invalidateBuiltRes(const DagorAsset &a, const char *cache_folder)
  {
    String cacheFname(260, "%s/%s~%s.c4.bin", cache_folder, a.getName(), a.getTypeStr());
    dd_erase(cacheFname);
  }

  virtual bool __stdcall getBuiltRes(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, IDagorAssetExporter *exp, const char *cache_folder,
    String &cache_path, int &data_offset, bool save_all_caches)
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
      if (crd.fileHandle && (crd.readInt() == _MAKE4C('fC1')) && (crd.readInt() == 2))
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
    if (!exp->buildAssetFast(a, cwr, *log))
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
    c4.save(cache_path, *mgr, &cwr, &data_offset);

    data_offset += sizeof(int); // see first data_offset caclulation

    return true;
  }
  virtual void __stdcall setExpCacheSharedData(void *p) { AssetExportCache::setSharedDataPtr(p); }

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

#ifndef _TARGET_DABUILD_STATIC
extern "C" __declspec(dllexport) IDaBuildInterface *__stdcall get_dabuild_interface()
{
  reqFastConv = true;
  return &daBuildExp;
}
#else
extern "C" IDaBuildInterface *__stdcall get_dabuild_interface() { return &daBuildExp; }
#endif
