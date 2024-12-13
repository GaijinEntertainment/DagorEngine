// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetHlp.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetExporter.h>
#include <assets/assetPlugin.h>
#include <assets/assetExpCache.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/strUtil.h>
#include <3d/ddsxTex.h>
#include <3d/ddsFormat.h>
#include <3d/ddsxTexMipOrder.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_ioUtils.h>
#include <generic/dag_smallTab.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_globalMutex.h>
#include <util/dag_texMetaData.h>
#include <util/dag_string.h>
#include <stdlib.h>
#undef ERROR

static IDagorAssetExporter *texExp = NULL;
static int texActype = -1;
static void *dllHandle = NULL;
static IDaBuildPlugin *texPlugin = NULL;
static String cacheBase;
static String cache2Base;
static bool reqFastConv = false;
static DagorAssetMgr *assetMgr = NULL;
static bool dryRun = false;

#define OVERRIDE_ZLIB_PACKING(a)                               \
  bool def_force_zlib = ddsx::ConvertParams::forceZlibPacking; \
  if ((a).props.getBool("forceZlib", false))                   \
    ddsx::ConvertParams::forceZlibPacking = true;

#define RESTORE_ZLIB_PACKING() ddsx::ConvertParams::forceZlibPacking = def_force_zlib;

class SimpleQuietLogWriter : public ILogWriter
{
public:
  virtual void addMessageFmt(MessageType, const char *, const DagorSafeArg *arg, int anum) {}
  virtual bool hasErrors() const { return false; }
  virtual void startLog() {}
  virtual void endLog() {}
};
static SimpleQuietLogWriter qCon;

static void checkIfMsvcrDllExists()
{
#if _TARGET_PC_WIN
  HMODULE h = LoadLibraryExA("msvcr120.dll", 0, LOAD_LIBRARY_AS_DATAFILE);
  if (h)
    FreeLibrary(h);
  else
    logerr("msvcr120.dll cannot be found. Try installing the Visual C++ Redistributable.");
#endif
}

static IDagorAssetExporter *loadSingleExporterPlugin(const DataBlock &appblk, DagorAssetMgr &mgr, const char *fname, int req_type)
{
  dllHandle = os_dll_load_deep_bind(fname);

  if (dllHandle)
  {
    get_dabuild_plugin_t get_plugin = (get_dabuild_plugin_t)os_dll_get_symbol(dllHandle, GET_DABUILD_PLUGIN_PROC);

    if (get_plugin)
    {
      texPlugin = get_plugin();
      if (texPlugin && texPlugin->init(appblk))
      {
        ::symhlp_load(fname);

        if (dabuild_plugin_install_dds_helper_t install_hlp =
              (dabuild_plugin_install_dds_helper_t)os_dll_get_symbol(dllHandle, DABUILD_PLUGIN_INSTALL_DDS_HLP_PROC))
          install_hlp(&texconvcache::write_built_dds_final);

        for (int i = 0; i < texPlugin->getExpCount(); i++)
          if (mgr.getAssetTypeId(texPlugin->getExpType(i)) == req_type)
            return texPlugin->getExp(i);

        ::symhlp_unload(fname);
        texPlugin->destroy();
      }
      texPlugin = NULL;
    }
    os_dll_close(dllHandle);
    dllHandle = NULL;
  }
  else
    logwarn("failed to load %s\nerror=%s", fname, os_dll_get_last_error_str());
  return NULL;
}

static Tab<void *> build_mutexes(inimem);
static Tab<SimpleString> build_mutex_names(inimem);
static void __cdecl release_build_mutexes()
{
  debug("atexit: release_build_mutexes(%d)", build_mutexes.size());
  while (build_mutexes.size())
  {
    void *m = build_mutexes.back();
    SimpleString nm(build_mutex_names.back());
    build_mutexes.pop_back();
    build_mutex_names.pop_back();

    debug("rel %p %s", m, nm);
    global_mutex_leave(m);
    global_mutex_destroy(m, nm);
  }
}
namespace texconvcache
{
struct BuildMutexAutoAcquire
{
  BuildMutexAutoAcquire(const char *ddsx_c4_bin)
  {
#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX
    String name_stor(ddsx_c4_bin);
    for (char *p = name_stor.str(); *p; p++)
      *p = (*p == ':' || *p == '/' || *p == '\\') ? '_' : *p;
    ddsx_c4_bin = name_stor.str();
#endif
    void *m = dryRun ? NULL : global_mutex_create(ddsx_c4_bin);
    if (!dryRun)
      global_mutex_enter(m);
    build_mutexes.push_back(m);
    build_mutex_names.push_back() = ddsx_c4_bin;
    // debug("acq %p %s (cnt=%d)", m, ddsx_c4_bin, build_mutexes.size());
  }
  ~BuildMutexAutoAcquire()
  {
    if (!build_mutexes.size())
      return;

    void *m = build_mutexes.back();
    SimpleString nm(build_mutex_names.back());
    build_mutexes.pop_back();
    build_mutex_names.pop_back();

    // debug("rel %p %s (cnt=%d)", m, nm, build_mutexes.size());
    if (m)
    {
      global_mutex_leave(m);
      global_mutex_destroy(m, nm);
    }
  }
};
} // namespace texconvcache

bool texconvcache::set_fast_conv(bool fast)
{
  bool prev = reqFastConv;
  reqFastConv = fast && !cache2Base.empty();
  return prev;
}

bool texconvcache::init(DagorAssetMgr &mgr, const DataBlock &appblk, const char *startdir, bool dr, bool fast)
{
  texActype = mgr.getTexAssetTypeId();
  if (texActype < 0)
    return false;

  const DataBlock &build_blk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("build");
  if (build_blk.getBool("preferZSTD", false))
  {
    ddsx::ConvertParams::preferZstdPacking = true;
    ddsx::ConvertParams::zstdMaxWindowLog = build_blk.getInt("zstdMaxWindowLog", 0);
    ddsx::ConvertParams::zstdCompressionLevel = build_blk.getInt("zstdCompressionLevel", 18);
    debug("DDSx prefers ZSTD (compressionLev=%d %s)", ddsx::ConvertParams::zstdCompressionLevel,
      ddsx::ConvertParams::zstdMaxWindowLog ? String(0, "maxWindow=%u", 1 << ddsx::ConvertParams::zstdMaxWindowLog).c_str()
                                            : "defaultWindow");
  }
  if (build_blk.getBool("allowOODLE", false))
  {
    ddsx::ConvertParams::allowOodlePacking = true;
    debug("DDSx allows OODLE");
  }
  if (build_blk.getBool("preferZLIB", false))
  {
    ddsx::ConvertParams::forceZlibPacking = true;
    if (!ddsx::ConvertParams::preferZstdPacking) //-V1051
      debug("DDSx prefers ZLIB");
  }

  texExp = mgr.getAssetExporter(texActype);
  if (!texExp)
  {
#if _TARGET_STATIC_LIB
    return false;
#else
    // no dabuild available, so load tex exporter manually
    alefind_t ff;
    const char *common_dir = "plugins/dabuild";
#if _TARGET_PC_WIN
    const String mask(260, "%s/%s/tex*" DAGOR_OS_DLL_SUFFIX, startdir, common_dir);
#elif _TARGET_PC
    const String mask(260, "%s/%s/libtex*" DAGOR_OS_DLL_SUFFIX, startdir, common_dir);
#endif
    String fname;

    if (::dd_find_first(mask, DA_FILE, &ff))
      do
      {
        fname.printf(260, "%s/%s/%s", startdir, common_dir, ff.name);
        texExp = loadSingleExporterPlugin(appblk, mgr, fname, texActype);
      } while (!texExp && ::dd_find_next(&ff));

    dd_find_close(&ff);
    if (!texExp)
    {
      checkIfMsvcrDllExists();
      return false;
    }
#endif
  }

  const DataBlock &expblk = *appblk.getBlockByNameEx("assets")->getBlockByNameEx("export");
  cacheBase.printf(260, "%s/%s/ddsx~cvt", appblk.getStr("appDir", startdir), expblk.getStr("cache", "/develop/.cache"));
  dd_simplify_fname_c(cacheBase);
  cacheBase.resize(strlen(cacheBase) + 1);

  if (expblk.getBool("useCache2Fast", false))
    cache2Base.printf(0, "%sFast", cacheBase);
  else
    cache2Base = "";
  reqFastConv = fast && !cache2Base.empty();

  AssetExportCache::setSdkRoot(appblk.getStr("appDir", startdir), "develop");
  assetMgr = &mgr;
  dryRun = dr;
  atexit(release_build_mutexes);
  return true;
}
void texconvcache::term()
{
  assetMgr = NULL;
  if (texPlugin)
    texPlugin->destroy();
  texPlugin = NULL;
  if (dllHandle)
    os_dll_close(dllHandle);
  dllHandle = NULL;
  texExp = NULL;
  texActype = -1;
}

bool texconvcache::is_tex_asset_convertible(DagorAsset &a) { return a.getType() == texActype && a.props.getBool("convert", false); }

static bool checkCacheChanged(AssetExportCache &c4, DagorAsset &a, const DataBlock &eff_props, IDagorAssetExporter *exp, bool enum_all)
{
  bool curChanged = false;
  DataBlock p = a.props;
  int processMips_nid = a.props.getNameId("processMips");
  for (int i = a.props.blockCount() - 1; i >= 0; i--)
    if (a.props.getBlock(i) != &eff_props && a.props.getBlock(i)->getBlockNameId() != processMips_nid)
      p.removeBlock(i);
  p.removeParam("ddsxTexPack");
  p.removeParam("ddsxTexPackAllowGrouping");
  p.removeParam("package");
  p.removeParam("forcePackage");
  p.removeParam("export");
  p.removeParam("splitAtOverride");
  p.removeParam("stubTexIdx");
  if (p.getInt("splitAt", 0) <= 0)
    p.removeParam("splitAt");
  p.removeParam("stubTexTag");
  p.removeParam("splitNotSeparate");
  if (p.getBool("splitHigh", false))
    p.removeParam("rtMipGenBQ");

  String nameTypified(a.getNameTypified());
  if (enum_all)
  {
    if (c4.checkDataBlockChanged(nameTypified, p))
      curChanged = true;
  }
  else if (c4.checkDataBlockChanged(nameTypified, p, -1))
  {
    if (c4.checkDataBlockChanged(nameTypified, a.props))
      return true;
    c4.checkDataBlockChanged(nameTypified, p, 1);
  }

  if (c4.checkAssetExpVerChanged(a.getType(), exp->getGameResClassId(), exp->getGameResVersion()))
  {
    if (enum_all)
      curChanged = true;
    else
      return true;
  }


  if (a.isVirtual() && c4.checkFileChanged(a.getTargetFilePath()))
  {
    if (enum_all)
      curChanged = true;
    else
      return true;
  }

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
    {
      if (enum_all)
        curChanged = true;
      else
        return true;
    }

  return curChanged;
}

static bool build_tex(DagorAsset &a, ddsx::Buffer &dest, unsigned target, const char *profile, ILogWriter &log, bool fast)
{
  G_ASSERT(texconvcache::is_tex_asset_convertible(a));

  const char *targetStr = mkbindump::get_target_str(target);
  mkbindump::BinDumpSaveCB tex_cwr(1 << 20, target, false);
  DataBlock old_props;
  if (fast)
  {
    old_props = a.props;
    a.props.setStr("quality", "fastest");
    a.props.setStr("downscaleFilter", "filterBox");
  }
  if (!texExp->exportAsset(a, tex_cwr, log))
  {
    if (fast)
      a.props = old_props;
    log.addMessage(ILogWriter::ERROR, "Can't export(convert) tex asset: %s", a.getName());
    return false;
  }
  if (fast)
    a.props = old_props;

  SmallTab<char, TmpmemAlloc> buf;
  {
    MemoryLoadCB crd(tex_cwr.getMem(), false);
    clear_and_resize(buf, tex_cwr.getSize());
    crd.read(buf.data(), data_size(buf));
  }
  tex_cwr.reset(0);

  TextureMetaData tmd;
  tmd.read(a.props, targetStr);
  if (!dagor_target_code_supports_tex_diff(target))
    tmd.baseTexName = NULL;

  ddsx::ConvertParams cp;
  cp.packSzThres = fast ? (512 << 20) : (16 << 10);
  cp.canPack = !fast;
  cp.addrU = tmd.d3dTexAddr(tmd.addrU);
  cp.addrV = tmd.d3dTexAddr(tmd.addrV);

  //== we assume that tex exporter already stripped unused tmd.hqMip mips (before conversion),
  //   so we don't want skip to them again in DDSx converter
  cp.hQMip = tmd.hqMip - tmd.hqMip; //-V501
  cp.mQMip = tmd.mqMip - tmd.hqMip;
  cp.lQMip = tmd.lqMip - tmd.hqMip;

#define GET_PROP(TYPE, PROP, DEF) props.get##TYPE(PROP, &props != &a.props ? a.props.get##TYPE(PROP, DEF) : DEF)
  const DataBlock &props = a.getProfileTargetProps(target, profile);
  int hq_lev = ((DDSURFACEDESC2 *)&buf[4])->dwMipMapCount >> 16;
  if (GET_PROP(Bool, "rtMipGen", false) && (hq_lev <= 0 || GET_PROP(Bool, "rtMipGenBQ", false)))
  {
    const char *mipFilt = GET_PROP(Str, "mipFilter", "");
    if (stricmp(mipFilt, "") == 0 || stricmp(mipFilt, "filterBox") == 0 || stricmp(mipFilt, "filterTriangle") == 0)
      cp.rtGenMipsBox = true;
    else if (stricmp(mipFilt, "filterKaiser") == 0)
      cp.rtGenMipsKaizer = true;

    cp.kaizerAlpha = GET_PROP(Real, "mipFilterAlpha", 4);
    cp.kaizerStretch = GET_PROP(Real, "mipFilterStretch", 1);
  }
  cp.imgGamma = GET_PROP(Real, "gamma", 2.2);
  cp.splitHigh = GET_PROP(Bool, "splitHigh", false);
  cp.splitAt = GET_PROP(Int, "splitAt", 0);
  if (!GET_PROP(Bool, "splitAtOverride", false))
    cp.splitAt = a.props.getBlockByNameEx(targetStr)->getInt("splitAtSize", cp.splitAt);

  cp.mipOrdRev = GET_PROP(Bool, "mipOrdRev", cp.mipOrdRev);

  if (!tmd.baseTexName.empty() && hq_lev <= 0)
    cp.needBaseTex = true;
  if (const char *paired = GET_PROP(Str, "pairedToTex", NULL))
    if (dagor_target_code_supports_tex_diff(target) && *paired && hq_lev <= 0)
    {
      int nid = a.props.getNameId("pairedToTex");
      for (int i = 0; i < props.paramCount(); i++)
        if (props.getParamNameId(i) == nid && props.getParamType(i) == DataBlock::TYPE_STRING)
          if (a.getMgr().findAsset(props.getStr(i), a.getType()))
          {
            cp.needSysMemCopy = true;
            break;
          }
      if (!cp.needSysMemCopy && &a.props != &props)
        for (int i = 0; i < a.props.paramCount(); i++)
          if (a.props.getParamNameId(i) == nid && a.props.getParamType(i) == DataBlock::TYPE_STRING)
            if (a.getMgr().findAsset(a.props.getStr(i), a.getType()))
            {
              cp.needSysMemCopy = true;
              break;
            }
    }
#undef GET_PROP

  OVERRIDE_ZLIB_PACKING(a);
  bool ret = ddsx::convert_dds(target, dest, buf.data(), data_size(buf), cp);
  RESTORE_ZLIB_PACKING();
  if (!ret)
    log.addMessage(ILogWriter::ERROR, "Can't make DDSx for asset: %s, err=%s", a.getName(), ddsx::get_last_error_text());
  return ret;
}

static void remove_fast_cache_when_final_ready(DagorAsset &a, const char *eff_target)
{
  if (!cache2Base.empty() && !reqFastConv)
  {
    String fn2(0, "%s/%s.ddsx.%s.c4.bin", cache2Base, a.getName(), eff_target);
    if (dd_file_exists(fn2))
      debug("removing %s due to final cache ready", fn2);
    dd_erase(fn2);
  }
}
static bool get_tex_asset_built_ddsx_internal(DagorAsset &a, ddsx::Buffer &dest, unsigned target, const char *profile, ILogWriter *log,
  bool ddsx_hdr_only, const char *use_prebuilt_file = nullptr)
{
  int64_t reft = ref_time_ticks_qpc();
  if (!texExp)
    return false;

  if (!log)
    log = &qCon;

  String eff_target(a.resolveEffProfileTargetStr(mkbindump::get_target_str(target), profile));
  String cacheFname(260, "%s/%s.ddsx.%s.c4.bin", cacheBase.str(), a.getName(), eff_target);

  AssetExportCache c4;
  int cacheEndPos = 0;
  const DataBlock &eff_props = a.getProfileTargetProps(target, profile);
  bool gamma1 = eff_props.getReal("gamma", a.props.getReal("gamma", 2.2)) <= 1.01;

  texconvcache::BuildMutexAutoAcquire bmaa(cacheFname);
  if (c4.load(cacheFname, a.getMgr(), &cacheEndPos) && !checkCacheChanged(c4, a, eff_props, texExp, false) && !use_prebuilt_file)
  {
  read_from_cache:
    FullFileLoadCB crd(cacheFname);

    DAGOR_TRY
    {
      crd.seekto(cacheEndPos);
      dest.len = crd.readInt();
      if (ddsx_hdr_only && dest.len > sizeof(ddsx::Header) && !c4.isTimeChanged())
        dest.len = sizeof(ddsx::Header);
      dest.ptr = tmpmem->alloc(dest.len);
      crd.read(dest.ptr, dest.len);
      crd.close();
    }
    DAGOR_CATCH(const IGenLoad::LoadException &exc)
    {
      logerr("failed to read '%s'", cacheFname);
      debug_flush(false);
      crd.close();
      dd_erase(cacheFname);
      tmpmem->free(dest.ptr);
      dest.zero();
      goto rebuild;
    }

    if (c4.isTimeChanged() && !dryRun)
    {
      c4.removeUntouched();
      c4.save(cacheFname, a.getMgr());

      FullFileSaveCB cwr(cacheFname, DF_WRITE | DF_APPEND);
      cwr.writeInt(dest.len);
      cwr.write(dest.ptr, dest.len);
    }
    if (target == _MAKE4C('PC'))
    {
      bool dump_gamma1 = (((ddsx::Header *)dest.ptr)->flags & ddsx::Header::FLG_GAMMA_EQ_1) ? true : false;
      if (dump_gamma1 != gamma1 && ((ddsx::Header *)dest.ptr)->levels)
      {
        logwarn("%s: inconsistent cache asset.gamma1=%d cache.gamma1=%d", a.getName(), gamma1, dump_gamma1);
        dd_erase(cacheFname);
        tmpmem->free(dest.ptr);
        dest.zero();
        goto rebuild;
      }
    }
    remove_fast_cache_when_final_ready(a, eff_target);
    return true;
  }

rebuild:
  if (use_prebuilt_file)
  {
    FullFileLoadCB crd(use_prebuilt_file);
    if (!crd.fileHandle)
    {
      log->addMessage(ILogWriter::ERROR, "failed to open prebuilt file %s", use_prebuilt_file);
      return false;
    }
    dest.len = df_length(crd.fileHandle);
    dest.ptr = tmpmem->alloc(dest.len);
    crd.read(dest.ptr, dest.len);
    goto after_build;
  }

  if (reqFastConv)
  {
    cacheFname.printf(0, "%s/%s.ddsx.%s.c4.bin", cache2Base, a.getName(), eff_target);

    if (c4.load(cacheFname, a.getMgr(), &cacheEndPos) && !checkCacheChanged(c4, a, eff_props, texExp, false))
      goto read_from_cache;
  }

  if (!build_tex(a, dest, target, profile, *log, reqFastConv))
  {
    dd_erase(cacheFname);
    return false;
  }
after_build:
  if (dryRun)
    return true;

  checkCacheChanged(c4, a, eff_props, texExp, true);

  c4.setAssetExpVer(a.getType(), texExp->getGameResClassId(), texExp->getGameResVersion());
  c4.removeUntouched();

  ::dd_mkpath(cacheFname);
  c4.save(cacheFname, a.getMgr());

  FullFileSaveCB cwr(cacheFname, DF_WRITE | DF_APPEND);
  cwr.writeInt(dest.len);
  cwr.write(dest.ptr, dest.len);

  if (use_prebuilt_file)
    log->addMessage(ILogWriter::NOTE, "recreated tex asset cache <%s>%s from %s", a.getName(),
      dest.len == sizeof(ddsx::Header) ? " - empty" : "", use_prebuilt_file);
  else
    log->addMessage(ILogWriter::NOTE, "converted%s tex asset <%s>%s for %.3f sec", reqFastConv ? " [FAST]" : "", a.getName(),
      dest.len == sizeof(ddsx::Header) ? " - empty" : "", get_time_usec_qpc(reft) / 1e6f);
  debug_flush(false);
  remove_fast_cache_when_final_ready(a, eff_target);
  return true;
}

bool texconvcache::is_tex_built_fast(DagorAsset &a, unsigned target, const char *profile)
{
  if (!texExp)
    return false;
  if (!texconvcache::is_tex_asset_convertible(a))
    return false;

  String cacheFname(260, "%s/%s.ddsx.%s.c4.bin", cacheBase.str(), a.getName(),
    a.resolveEffProfileTargetStr(mkbindump::get_target_str(target), profile));

  AssetExportCache c4;
  const DataBlock &eff_props = a.getProfileTargetProps(target, profile);

  BuildMutexAutoAcquire bmaa(cacheFname);
  if (c4.load(cacheFname, a.getMgr(), NULL) && !checkCacheChanged(c4, a, eff_props, texExp, false))
    return false;
  cacheFname.printf(0, "%s/%s.ddsx.%s.c4.bin", cache2Base.str(), a.getName(),
    a.resolveEffProfileTargetStr(mkbindump::get_target_str(target), profile));
  if (c4.load(cacheFname, a.getMgr(), NULL) && !checkCacheChanged(c4, a, eff_props, texExp, false))
    return true;
  return false;
}

int texconvcache::validate_tex_asset_cache(DagorAsset &a, unsigned target, const char *profile, ILogWriter *log, bool upd)
{
  if (!texExp)
    return -1;
  G_ASSERT(texconvcache::is_tex_asset_convertible(a));

  String eff_target(a.resolveEffProfileTargetStr(mkbindump::get_target_str(target), profile));
  String cacheFname(260, "%s/%s.ddsx.%s.c4.bin", cacheBase.str(), a.getName(), eff_target);

  AssetExportCache c4;
  int cacheEndPos = 0;
  const DataBlock &eff_props = a.getProfileTargetProps(target, profile);

  BuildMutexAutoAcquire bmaa(cacheFname);
  if (c4.load(cacheFname, a.getMgr(), &cacheEndPos) && !checkCacheChanged(c4, a, eff_props, texExp, false))
  {
    remove_fast_cache_when_final_ready(a, eff_target);
    return 1;
  }

  if (!upd)
    return 0;

  if (!log)
    log = &qCon;
  ddsx::Buffer dest;
  if (!build_tex(a, dest, target, profile, *log, false))
  {
    dd_erase(cacheFname);
    return 0;
  }
  if (dryRun)
  {
    dest.free();
    return 2;
  }

  checkCacheChanged(c4, a, eff_props, texExp, true);

  c4.setAssetExpVer(a.getType(), texExp->getGameResClassId(), texExp->getGameResVersion());
  c4.removeUntouched();

  ::dd_mkdir(cacheBase);
  c4.save(cacheFname, a.getMgr());

  FullFileSaveCB cwr(cacheFname, DF_WRITE | DF_APPEND);
  cwr.writeInt(dest.len);
  cwr.write(dest.ptr, dest.len);
  dest.free();
  remove_fast_cache_when_final_ready(a, eff_target);
  return 2;
}

bool texconvcache::get_tex_asset_built_ddsx(DagorAsset &a, ddsx::Buffer &dest, unsigned target, const char *profile, ILogWriter *log)
{
  return get_tex_asset_built_ddsx_internal(a, dest, target, profile, log, false);
}

namespace texconvcache
{
bool force_ddsx(DagorAsset &a, ddsx::Buffer &dest, unsigned t, const char *p, ILogWriter *log, const char *built_fn)
{
  return get_tex_asset_built_ddsx_internal(a, dest, t, p, log, false, built_fn);
}
} // namespace texconvcache

int texconvcache::write_tex_contents(IGenSave &cwr, DagorAsset &a, unsigned target, const char *profile, ILogWriter *l)
{
  if (a.getType() != texActype)
  {
    if (l)
      l->addMessage(ILogWriter::ERROR, "%s is not texture asset", a.getNameTypified());
    return -1;
  }

  if (texconvcache::is_tex_asset_convertible(a))
  {
    ddsx::Buffer dest;
    if (get_tex_asset_built_ddsx_internal(a, dest, target, profile, l, false))
    {
      int len = dest.len;
      cwr.write(dest.ptr, dest.len);
      dest.free();
      return len;
    }
    return -1;
  }

  // non-convertible texture; check for DDS
  FullFileLoadCB crd(a.getTargetFilePath());
  if (!crd.fileHandle)
  {
    if (l)
      l->addMessage(ILogWriter::ERROR, "file not found: %s (asset %s)", a.getTargetFilePath(), a.getName());
    return -1;
  }
  unsigned mark = 0;
  int size = df_length(crd.fileHandle);

  crd.read(&mark, 4);
  if (mark != _MAKE4C('DDS '))
  {
    if (l)
      l->addMessage(ILogWriter::ERROR, "not DDS file: %s (asset %s)", a.getTargetFilePath(), a.getName());
    return -1;
  }
  crd.seekto(0);
  copy_stream_to_stream(crd, cwr, size);
  return size;
}

int __stdcall texconvcache::write_built_dds_final(IGenSave &cwr, DagorAsset &a, unsigned target, const char *profile, ILogWriter *l)
{
#define GET_PROP(TYPE, PROP, DEF) props.get##TYPE(PROP, &props != &a.props ? a.props.get##TYPE(PROP, DEF) : DEF)
  const DataBlock &props = a.getProfileTargetProps(target, profile);
  const char *basetex_nm = dagor_target_code_supports_tex_diff(target) ? (GET_PROP(Str, "baseTex", NULL)) : NULL;
  String basetex_hq_stor;

  if (!a.props.getBool("convert", false))
  {
    String targetFilePath(a.getTargetFilePath());
    if (const char *ext = dd_get_fname_ext(targetFilePath))
      if (dd_stricmp(ext, ".dds") == 0)
      {
        FullFileLoadCB crd(targetFilePath);
        if (!crd.fileHandle)
          return -1;
        copy_stream_to_stream(crd, cwr, crd.getTargetDataSize());
        return crd.getTargetDataSize();
      }
  }

  ddsx::Buffer ddsx_hdr_buf;
  if (!get_tex_asset_built_ddsx_internal(a, ddsx_hdr_buf, target, profile, l, true))
    return -1;

  bool is_diff_tex = ((ddsx::Header *)ddsx_hdr_buf.ptr)->flags & ddsx::Header::FLG_NEED_PAIRED_BASETEX;
  ddsx_hdr_buf.free();

  if (basetex_nm && *basetex_nm && is_diff_tex)
  {
    if (GET_PROP(Bool, "splitHigh", false))
    {
      basetex_hq_stor.printf(0, "%s$hq", basetex_nm);
      basetex_nm = basetex_hq_stor;
    }
    DagorAsset *a_base = a.getMgr().findAsset(basetex_nm, a.getType());
    if (!a_base)
    {
      if (l)
        l->addMessage(ILogWriter::ERROR, "asset <%s> refers to missing baseTex <%s>", a.getName(), basetex_nm);
      return -1;
    }

    if (a_base == &a)
    {
      if (l)
        l->addMessage(ILogWriter::ERROR, "asset <%s> refers to itself as baseTex <%s>", a.getName(), basetex_nm);
      return -1;
    }

    MemorySaveCB cwr_base(1 << 20), cwr_diff(1 << 20);
    if (write_built_dds_final(cwr_base, *a_base, target, profile, l) < 0)
    {
      if (l)
        l->addMessage(ILogWriter::ERROR, "asset <%s> failed to get DDSx for baseTex <%s>", a.getName(), basetex_nm);
      return -1;
    }
    if (write_tex_contents(cwr_diff, a, target, profile, l) < 0)
    {
      if (l)
        l->addMessage(ILogWriter::ERROR, "asset <%s> failed to get DDSx for tex difference", a.getName());
      return -1;
    }

    DDSURFACEDESC2 hb;
    ddsx::Header hd;
    MemoryLoadCB crd_base(cwr_base.getMem(), false), crd_diff(cwr_diff.getMem(), false);

    crd_base.readInt();
    crd_base.read(&hb, sizeof(hb));
    crd_diff.read(&hd, sizeof(hd));

    if (dagor_target_code_be(target))
    {
      hd.w = mkbindump::le2be16(hd.w);
      hd.h = mkbindump::le2be16(hd.h);
      hd.depth = mkbindump::le2be16(hd.depth);
      hd.flags = mkbindump::le2be32(hd.flags);
    }

    if (hb.dwWidth != hd.w || hb.dwHeight != hd.h || hb.dwDepth || hd.depth || !hd.checkLabel() ||
        !(hd.flags & hd.FLG_NEED_PAIRED_BASETEX))
    {
      if (l)
        l->addMessage(ILogWriter::ERROR, "asset <%s> mismatch: %dx%dx%d != <%s> %dx%dx%d", a.getName(), hd.w, hd.h, hd.depth,
          basetex_nm, hb.dwWidth, hb.dwHeight, hb.dwDepth);
      return -1;
    }
    if ((hb.ddpfPixelFormat.dwFourCC != _MAKE4C('DXT1') && hb.ddpfPixelFormat.dwFourCC != _MAKE4C('DXT5')) ||
        (hd.d3dFormat != _MAKE4C('DXT1') && hd.d3dFormat != _MAKE4C('DXT5')))
    {
      if (l)
        l->addMessage(ILogWriter::ERROR, "asset <%s> mismatch: %c%c%c%c != <%s> %c%c%c%c", a.getName(), _DUMP4C(hd.d3dFormat),
          basetex_nm, _DUMP4C(hb.ddpfPixelFormat.dwFourCC));
      return -1;
    }

    MemorySaveCB cwr_unpack(1 << 20);
    if (hd.isCompressionZSTD())
    {
      zstd_decompress_data(cwr_unpack, crd_diff, hd.packedSz);
      crd_diff.setMem(cwr_unpack.getMem(), false);
    }
    else if (hd.isCompressionOODLE())
    {
      oodle_decompress_data(cwr_unpack, crd_diff, hd.packedSz, hd.memSz);
      crd_diff.setMem(cwr_unpack.getMem(), false);
    }
    else if (hd.isCompressionZLIB())
    {
      ZlibLoadCB zlib_crd(crd_diff, hd.packedSz);
      copy_stream_to_stream(zlib_crd, cwr_unpack, hd.memSz);
      crd_diff.setMem(cwr_unpack.getMem(), false);
    }
    else if (hd.isCompression7ZIP())
    {
      LzmaLoadCB lzma_crd(crd_diff, hd.packedSz);
      copy_stream_to_stream(lzma_crd, cwr_unpack, hd.memSz);
      lzma_crd.ceaseReading();
      crd_diff.setMem(cwr_unpack.getMem(), false);
    }

    crd_diff.seekrel(3 * sizeof(float)); // skip kaizer params

    // patch base texture with difference to build final DXT* data
    struct Dxt1ColorBlock
    {
      unsigned c0r : 5, c0g : 6, c0b : 5;
      unsigned c1r : 5, c1g : 6, c1b : 5;
      unsigned idx;
    };
    struct Dxt5AlphaBlock
    {
      unsigned char a0, a1;
      unsigned short idx[3];
    };

    int blk_total = (hd.w / 4) * (hd.h / 4);
    int blk_empty = crd_diff.readInt();
    Tab<unsigned> map(tmpmem);
    map.resize((blk_total + 31) / 32);
    if (blk_empty == 0)
      mem_set_ff(map);
    else if (blk_empty == blk_total)
      mem_set_0(map);
    else
      crd_diff.read(map.data(), data_size(map));

    int dxt_blk_sz = (hd.d3dFormat == _MAKE4C('DXT1')) ? 8 : 16;
    Tab<char> dxtData(tmpmem);
    dxtData.resize(blk_total * dxt_blk_sz);

    if (hb.ddpfPixelFormat.dwFourCC == hd.d3dFormat)
      crd_base.read(dxtData.data(), blk_total * dxt_blk_sz);
    else if (hb.ddpfPixelFormat.dwFourCC == _MAKE4C('DXT1'))
      for (int i = 0; i < blk_total; i++)
      {
        memset(&dxtData[i * 16], 0xFF, 2);
        memset(&dxtData[i * 16 + 2], 0, 6);
        crd_base.read(&dxtData[i * 16 + 8], 8);
      }
    else if (hb.ddpfPixelFormat.dwFourCC == _MAKE4C('DXT5'))
      for (int i = 0; i < blk_total; i++)
      {
        crd_base.seekrel(8);
        crd_base.read(&dxtData[i * 8], 8);
      }


    if (hd.d3dFormat == _MAKE4C('DXT5'))
      for (int i = 0; i < map.size(); i++)
      {
        if (!map[i])
          continue;
        for (unsigned j = 0, w = map[i]; w; j++, w >>= 1)
        {
          if (!(w & 1))
            continue;

          Dxt5AlphaBlock pa, &ba = *(Dxt5AlphaBlock *)(&dxtData[(i * 32 + j) * dxt_blk_sz]);
          crd_diff.read(&pa, sizeof(pa));

          if (hb.ddpfPixelFormat.dwFourCC == _MAKE4C('DXT5'))
          {
            pa.a0 += ba.a0;
            pa.a1 += ba.a1;
            pa.idx[0] = ba.idx[0] ^ pa.idx[0];
            pa.idx[1] = ba.idx[1] ^ pa.idx[1];
            pa.idx[2] = ba.idx[2] ^ pa.idx[2];
          }
          memcpy(&ba, &pa, sizeof(pa));
        }
      }
    for (int i = 0; i < map.size(); i++)
    {
      if (!map[i])
        continue;
      for (unsigned j = 0, w = map[i]; w; j++, w >>= 1)
      {
        if (!(w & 1))
          continue;

        Dxt1ColorBlock pc, &bc = *(Dxt1ColorBlock *)(&dxtData[(i * 32 + j) * dxt_blk_sz + (hd.d3dFormat == _MAKE4C('DXT5') ? 8 : 0)]);
        crd_diff.read(&pc, sizeof(pc));

        pc.c0r += bc.c0r;
        pc.c0g += bc.c0g;
        pc.c0b += bc.c0b;
        pc.c1r += bc.c1r;
        pc.c1g += bc.c1g;
        pc.c1b += bc.c1b;
        pc.idx = bc.idx ^ pc.idx;
        memcpy(&bc, &pc, sizeof(pc));
      }
    }
    hb.dwMipMapCount = 1;
    hb.ddpfPixelFormat.dwFourCC = hd.d3dFormat;

    cwr.writeInt(_MAKE4C('DDS '));
    cwr.write(&hb, sizeof(hb));
    cwr.write(dxtData.data(), data_size(dxtData));
    return int(sizeof(hb) + data_size(dxtData));
  }
  int pos = cwr.tell();
  if (!get_tex_asset_built_raw_dds(a, cwr, target, profile, l))
    return -1;
  return cwr.tell() - pos;
#undef GET_PROP
}

int texconvcache::get_tex_size(DagorAsset &a, unsigned target, const char *profile)
{
  if (a.getType() != texActype)
    return -1;

  if (texconvcache::is_tex_asset_convertible(a))
  {
    ddsx::Buffer dest;
    if (get_tex_asset_built_ddsx_internal(a, dest, target, profile, NULL, true))
    {
      ddsx::Header *hdr = (ddsx::Header *)dest.ptr;
      int memSz = mkbindump::le2be32_cond(hdr->memSz, dagor_target_code_be(target));
      dest.free();

      if (DagorAsset *hq_tex = a.getMgr().findAsset(String(0, "%s$hq", a.getName(), a.getType())))
      {
        G_ASSERT(texconvcache::is_tex_asset_convertible(*hq_tex));
        if (get_tex_asset_built_ddsx_internal(*hq_tex, dest, target, profile, NULL, true))
        {
          ddsx::Header *hdr = (ddsx::Header *)dest.ptr;
          memSz += mkbindump::le2be32_cond(hdr->memSz, dagor_target_code_be(target));
          dest.free();
        }
      }
      return memSz;
    }
    return -1;
  }

  // non-convertible texture; check for DDS
  FullFileLoadCB crd(a.getTargetFilePath());
  if (!crd.fileHandle)
    return -1;
  unsigned mark = 0;
  int size = df_length(crd.fileHandle);

  crd.read(&mark, 4);
  if (mark != _MAKE4C('DDS '))
    return -1;
  return size;
}


bool texconvcache::get_tex_asset_built_raw_dds(DagorAsset &a, IGenSave &cwr, unsigned target, const char *profile, ILogWriter *l)
{
  if (!texconvcache::is_tex_asset_convertible(a))
  {
    SimpleString fn(a.getTargetFilePath());
    if (trail_stricmp(fn, ".dds"))
    {
      FullFileLoadCB crd(fn);
      if (!crd.fileHandle)
      {
        l->addMessage(ILogWriter::ERROR, "asset <%s>: failed to open source <%s> file", a.getName(), fn);
        return false;
      }
      copy_stream_to_stream(crd, cwr, df_length(crd.fileHandle));
      return true;
    }
    l->addMessage(ILogWriter::WARNING, "non-convertible tex asset <%s> refers to usupported <%s> file", a.getName(), fn);
    return false;
  }

  struct BitMaskFormat
  {
    uint32_t bitCount;
    uint32_t alphaMask;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint32_t format;
    uint32_t flags;
  };
  static BitMaskFormat bitMaskFormat[] = {
    {24, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_R8G8B8, DDPF_RGB},
    {32, 0xff000000, 0xff0000, 0xff00, 0xff, D3DFMT_A8R8G8B8, DDPF_RGB | DDPF_ALPHA},
    {32, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_X8R8G8B8, DDPF_RGB},
    {16, 0x00000000, 0x00f800, 0x07e0, 0x1f, D3DFMT_R5G6B5, DDPF_RGB},
    {16, 0x00008000, 0x007c00, 0x03e0, 0x1f, D3DFMT_A1R5G5B5, DDPF_RGB | DDPF_ALPHA},
    {16, 0x00000000, 0x007c00, 0x03e0, 0x1f, D3DFMT_X1R5G5B5, DDPF_RGB},
    {16, 0x0000f000, 0x000f00, 0x00f0, 0x0f, D3DFMT_A4R4G4B4, DDPF_RGB | DDPF_ALPHA},
    {8, 0x00000000, 0x0000e0, 0x001c, 0x03, D3DFMT_R3G3B2, DDPF_RGB},
    {8, 0x000000ff, 0x000000, 0x0000, 0x00, D3DFMT_A8, DDPF_ALPHA},
    {8, 0x00000000, 0x0000FF, 0x0000, 0x00, D3DFMT_L8, DDPF_LUMINANCE},
    {16, 0x0000ff00, 0x0000e0, 0x001c, 0x03, D3DFMT_A8R3G3B2, DDPF_RGB | DDPF_ALPHA},
    {16, 0x00000000, 0x000f00, 0x00f0, 0x0f, D3DFMT_X4R4G4B4, DDPF_RGB},
    {32, 0xff000000, 0xff0000, 0xff00, 0xff, D3DFMT_A8B8G8R8, DDPF_RGB | DDPF_ALPHA},
    {32, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_X8B8G8R8, DDPF_RGB},
    {16, 0x00000000, 0x0000ff, 0xff00, 0x00, D3DFMT_V8U8, DDPF_BUMPDUDV},
    {16, 0x0000ff00, 0x0000ff, 0x0000, 0x00, D3DFMT_A8L8, DDPF_LUMINANCE | DDPF_ALPHA},
    {16, 0x00000000, 0xFFFF, 0x00000000, 0, D3DFMT_L16, DDPF_LUMINANCE},
  };

  // getting ddsx form texture
  ddsx::Buffer b;
  if (!texconvcache::get_tex_asset_built_ddsx(a, b, target, profile, l))
    return false;

  // getting ddsx header
  InPlaceMemLoadCB crd(b.ptr, b.len);
  ddsx::Header hdr;
  crd.read(&hdr, sizeof(hdr));
  if (!hdr.checkLabel())
  {
    b.free();
    l->addMessage(ILogWriter::NOTE, "invalid DDSx format");
    return false;
  }

  // dds header preparing
  DDSURFACEDESC2 targetHeader;
  memset(&targetHeader, 0, sizeof(DDSURFACEDESC2));
  targetHeader.dwSize = sizeof(DDSURFACEDESC2);
  targetHeader.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
  targetHeader.dwHeight = hdr.h;
  targetHeader.dwWidth = hdr.w;
  targetHeader.dwDepth = hdr.depth;
  //== we could report levels count for GENMIP if receiver process it properly
  targetHeader.dwMipMapCount = hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER) ? 1 : hdr.levels;
  targetHeader.ddsCaps.dwCaps = 0;
  targetHeader.ddsCaps.dwCaps2 = 0;

  if (hdr.flags & ddsx::Header::FLG_CUBTEX)
    targetHeader.ddsCaps.dwCaps2 |= DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES;
  else if (hdr.flags & ddsx::Header::FLG_VOLTEX)
  {
    targetHeader.dwFlags |= DDSD_DEPTH;
    targetHeader.ddsCaps.dwCaps2 |= DDSCAPS2_VOLUME;
  }
  else
    targetHeader.ddsCaps.dwCaps |= DDSCAPS_TEXTURE;

  DDPIXELFORMAT &pf = targetHeader.ddpfPixelFormat;
  pf.dwSize = sizeof(DDPIXELFORMAT);

  // pixel format search
  int i;
  for (i = 0; i < sizeof(bitMaskFormat) / sizeof(BitMaskFormat); i++)
    if (hdr.d3dFormat == bitMaskFormat[i].format)
      break;

  if (i < sizeof(bitMaskFormat) / sizeof(BitMaskFormat))
  {
    pf.dwFlags = bitMaskFormat[i].flags;
    pf.dwRGBBitCount = bitMaskFormat[i].bitCount;
    pf.dwRBitMask = bitMaskFormat[i].redMask;
    pf.dwGBitMask = bitMaskFormat[i].greenMask;
    pf.dwBBitMask = bitMaskFormat[i].blueMask;
    pf.dwRGBAlphaBitMask = bitMaskFormat[i].alphaMask;
  }
  else
  {
    pf.dwFlags = DDPF_FOURCC;
    pf.dwFourCC = hdr.d3dFormat;
  }


  // dds output
  uint32_t FourCC = MAKEFOURCC('D', 'D', 'S', ' ');
  cwr.write(&FourCC, sizeof(FourCC));
  cwr.write(&targetHeader, sizeof(targetHeader));

  DynamicMemGeneralSaveCB mcwr(tmpmem, hdr.memSz);
  if (hdr.isCompressionZSTD())
    zstd_decompress_data(mcwr, crd, hdr.packedSz);
  else if (hdr.isCompressionOODLE())
    oodle_decompress_data(mcwr, crd, hdr.packedSz, hdr.memSz);
  else if (hdr.isCompressionZLIB())
  {
    ZlibLoadCB zlib_crd(crd, hdr.packedSz);
    copy_stream_to_stream(zlib_crd, mcwr, hdr.memSz);
  }
  else if (hdr.isCompression7ZIP())
  {
    LzmaLoadCB lzma_crd(crd, hdr.packedSz);
    copy_stream_to_stream(lzma_crd, mcwr, hdr.memSz);
    lzma_crd.ceaseReading();
  }
  else
    copy_stream_to_stream(crd, mcwr, hdr.memSz);

  ddsx_forward_mips_inplace(hdr, (char *)mcwr.data(), mcwr.size());
  cwr.write(mcwr.data(), mcwr.size());
  b.free();

  return true;
}

bool texconvcache::get_tex_asset_built_raw_dds_by_name(const char *texname, IGenSave &cwr, unsigned target, const char *profile,
  ILogWriter *l)
{
  if (!assetMgr)
    return false;

  // search of asset
  String tmp_stor;
  String out_str(DagorAsset::fpath2asset(TextureMetaData::decodeFileName(texname, &tmp_stor)));
  if (out_str.length() > 0 && out_str[out_str.length() - 1] == '*')
    erase_items(out_str, out_str.length() - 1, 1);
  DagorAsset *tex_a = assetMgr->findAsset(out_str, assetMgr->getTexAssetTypeId());

  if (!tex_a)
    return false;

  return texconvcache::get_tex_asset_built_raw_dds(*tex_a, cwr, target, profile, l);
}

bool texconvcache::convert_dds(ddsx::Buffer &b, const char *tex_path, const DagorAsset *a, unsigned target,
  const ddsx::ConvertParams &_cp)
{
  const char *targetStr = mkbindump::get_target_str(target);
  String cacheFname(260, "%s/%s.ddsx.%s.ct.bin", cacheBase.str(), a->getName(), targetStr);

  AssetExportCache c4;
  int cacheEndPos = 0;
  ddsx::ConvertParams cp = _cp;

#define GET_PROP(TYPE, PROP, DEF) props.get##TYPE(PROP, &props != &a->props ? a->props.get##TYPE(PROP, DEF) : DEF)
  const DataBlock &props = a->getProfileTargetProps(target, NULL);
  if (GET_PROP(Bool, "rtMipGen", false))
  {
    const char *mipFilt = GET_PROP(Str, "mipFilter", "");
    if (stricmp(mipFilt, "") == 0 || stricmp(mipFilt, "filterBox") == 0 || stricmp(mipFilt, "filterTriangle") == 0)
      cp.rtGenMipsBox = true;
    else if (stricmp(mipFilt, "filterKaiser") == 0)
      cp.rtGenMipsKaizer = true;

    cp.kaizerAlpha = GET_PROP(Real, "mipFilterAlpha", 4);
    cp.kaizerStretch = GET_PROP(Real, "mipFilterStretch", 1);
    cp.imgGamma = GET_PROP(Real, "gamma", 2.2);
  }
  cp.splitHigh = GET_PROP(Bool, "splitHigh", false);
  cp.splitAt = GET_PROP(Int, "splitAt", 0);
  if (!GET_PROP(Bool, "splitAtOverride", false))
    cp.splitAt = a->props.getBlockByNameEx(targetStr)->getInt("splitAtSize", cp.splitAt);

  cp.mipOrdRev = GET_PROP(Bool, "mipOrdRev", cp.mipOrdRev);
#undef GET_PROP

  BuildMutexAutoAcquire bmaa(cacheFname);
  if (c4.load(cacheFname, a->getMgr(), &cacheEndPos) && !c4.checkFileChanged(tex_path) &&
      !c4.checkDataBlockChanged(a->getNameTypified(), const_cast<DataBlock &>(a->props)))
  {
    {
      FullFileLoadCB crd(cacheFname);

      DAGOR_TRY
      {
        crd.seekto(cacheEndPos);
        b.len = crd.readInt();
        b.ptr = tmpmem->alloc(b.len);
        crd.read(b.ptr, b.len);
        crd.close();
      }
      DAGOR_CATCH(const IGenLoad::LoadException &exc)
      {
        logerr("failed to read '%s'", cacheFname);
        debug_flush(false);
        crd.close();
        tmpmem->free(b.ptr);
        b.zero();
        goto rebuild;
      }
    }

    if (c4.isTimeChanged() && !dryRun)
    {
    store_cache:
      c4.removeUntouched();
      c4.save(cacheFname, a->getMgr());

      ::dd_mkpath(cacheFname);
      FullFileSaveCB cwr(cacheFname, DF_WRITE | DF_APPEND);
      cwr.writeInt(b.len);
      cwr.write(b.ptr, b.len);
    }
    return true;
  }

rebuild:
  SmallTab<char, TmpmemAlloc> buf;
  file_ptr_t handle = df_open(tex_path, DF_READ);
  int len = df_length(handle);
  clear_and_resize(buf, len);
  df_read(handle, buf.data(), len);
  df_close(handle);

  int t0 = get_time_msec_qpc();
  OVERRIDE_ZLIB_PACKING(*a);
  bool ret = ddsx::convert_dds(target, b, buf.data(), len, cp);
  RESTORE_ZLIB_PACKING();
  t0 = get_time_msec_qpc() - t0;
  if (t0 > 50 && len > (16 << 10) + 128 && !dryRun)
  {
    c4.checkFileChanged(tex_path);
    c4.checkDataBlockChanged(a->getNameTypified(), const_cast<DataBlock &>(a->props));
    goto store_cache;
  }
  debug("%s* DDS(%d)->DDSx(%d)= %+d b", a->getName(), len, b.len, b.len - len);
  return ret;
}
