// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/assetHlp.h>
#include <assets/daBuildInterface.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/assetMsgPipe.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetExpCache.h>
#include <assets/assetPlugin.h>
#include <gameRes/dag_gameResHooks.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <gameRes/dag_stdGameRes.h>
#include <3d/dag_texMgr.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/appDirRelativePath.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_string.h>
#include <util/dag_texMetaData.h>
#include <util/dag_safeArg.h>
#include <hash/xxh3.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_critSec.h>
#include <ioSys/dag_memIo.h>
#include <debug/dag_debug.h>

static const int ASSET_RESID_BASE = 512 << 10;
static WinCritSec dabuild_cache_cs;

static String startDir;
static IDaBuildInterface *dabuild = NULL;
static DagorAssetMgr *assetMgr = NULL;
static DataBlock bindAssetsBlock; // own copy: caller's appblk may be a scratch object, mutated/reused later
static ILogWriter *buildLog = NULL;
static FastNameMapEx resMap;
static FastNameMap assetMap;
static Tab<unsigned> assetRev(inimem);
static Tab<unsigned> assetTypeToClassId(inimem);
static Tab<int> assetExpTypes(inimem);
static int texTypeId = -1;

static void install_gameres_hooks();
static void reset_gameres_hooks();
static bool resolve_res_handle(const DagorAsset *a, int &out_res_id);

class PassThroughLogWriter : public ILogWriter
{
  bool err = false;
  ILogWriter *log = nullptr;

public:
  PassThroughLogWriter(ILogWriter *l) : log(l) {}

  void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum) override
  {
    if (type == ERROR || type == FATAL)
      err = true;
    if (log)
      log->addMessageFmt(type, fmt, arg, anum);
  }
  bool hasErrors() const override { return err; }

  void startLog() override
  {
    if (log)
      log->startLog();
  }
  void endLog() override
  {
    if (log)
      log->endLog();
  }
};


class DaBuildCacheUpdater : public IDagorAssetBaseChangeNotify, public IDagorAssetChangeNotify
{
public:
  void onAssetBaseChanged(dag::ConstSpan<DagorAsset *> changed_assets, dag::ConstSpan<DagorAsset *> added_assets,
    dag::ConstSpan<DagorAsset *> removed_assets) override
  {
    WinAutoLock lock(dabuild_cache_cs);
    for (int i = 0; i < changed_assets.size(); i++)
    {
      const DagorAsset &a = *changed_assets[i];

      int anid = assetMap.getNameId(a.getNameTypified());
      if (anid != -1)
        assetRev[anid]++;
    }
  }
  void onAssetRemoved(int asset_name_id, int asset_type) override {}
  void onAssetChanged(const DagorAsset &a, int asset_name_id, int asset_type) override
  {
    if (a.getType() == texTypeId)
      ::reload_changed_texture_asset(a);
  }
};
static DaBuildCacheUpdater updater;

bool dabuildcache::init(const char *start_dir, ILogWriter *l, IDaBuildInterface *_dabuild)
{
  startDir = start_dir;
  dabuild = _dabuild;
  buildLog = l;
  DEBUG_CTX("dabuildcache inited with static dabuild lib");
  return true;
}

bool dabuildcache::init(const char *start_dir, ILogWriter *l)
{
  const char *dabuild_bin_fn =
#if _TARGET_PC_LINUX | _TARGET_PC_MACOSX
    "lib"
#endif
    "daBuild" DAGOR_DLL;

  startDir = start_dir;
  dabuild = get_dabuild(startDir + "/" + dabuild_bin_fn);
  if (!dabuild)
  {
    debug("ERR: cannot load %s", dabuild_bin_fn);
    return false;
  }
  DEBUG_CTX("daBuild loaded");
  buildLog = l;
  return true;
}
void dabuildcache::term()
{
  if (!dabuild)
    return;

  AssetExportCache::saveSharedData();
  dabuild->setExpCacheSharedData(NULL);
  if (assetMgr)
  {
    assetMgr->unsubscribeBaseUpdateNotify(&updater);
    assetMgr->unsubscribeUpdateNotify(&updater);
  }
  reset_gameres_hooks();
  assetMgr = NULL;
  bindAssetsBlock.reset();

  dabuild->unloadExporterPlugins();
  dabuild->term();
  release_dabuild(dabuild);
  dabuild = NULL;
  DEBUG_CTX("daBuild unloaded");
}

IDaBuildInterface *dabuildcache::get_dabuild() { return dabuild; }

int dabuildcache::bind_with_mgr(DagorAssetMgr &mgr, DataBlock &appblk, const char *appdir, const char *ddsx_plugins_path)
{
  WinAutoLock lock(dabuild_cache_cs);
  if (!dabuild)
    return -1;

  bindAssetsBlock.setFrom(appblk.getBlockByNameEx("assets"), appblk.resolveFilename());

  if (appblk.getStr("shaders", NULL))
    appblk.setStr("shadersAbs", make_eff_app_relative_path(appblk.getStr("shaders")));

  String cdk_dir(260, "%s/../", startDir.str());
  ::dd_simplify_fname_c(cdk_dir);
  appblk.setStr("dagorCdkDir", cdk_dir);

  int pcount = dabuild->init(startDir, mgr, appblk, appdir, ddsx_plugins_path);
  if (pcount)
  {
    dabuild->loadExporterPlugins();

    resMap.reset();
    assetMgr = &mgr;
    texTypeId = mgr.getTexAssetTypeId();

    assetTypeToClassId.resize(mgr.getAssetTypesCount());
    for (int i = 0; i < assetTypeToClassId.size(); i++)
    {
      IDagorAssetExporter *exp = assetMgr->getAssetExporter(i);
      assetTypeToClassId[i] = exp ? exp->getGameResClassId() : 0;
    }

    assetMgr->subscribeBaseUpdateNotify(&updater);
    assetMgr->subscribeUpdateNotify(&updater, -1, texTypeId);
    install_gameres_hooks();
    assetlocalprops::mkDir("cache");
    AssetExportCache::createSharedData(assetlocalprops::makePath("assets-hash.bin"));
    dabuild->setExpCacheSharedData(AssetExportCache::getSharedDataPtr());
  }

  assetExpTypes.reserve(assetTypeToClassId.size());
  for (int i = 0; i < assetTypeToClassId.size(); i++)
    if (assetTypeToClassId[i])
      assetExpTypes.push_back(i);
  assetExpTypes.push_back(texTypeId);

  return pcount;
}

void dabuildcache::post_base_update_notify() { free_unused_game_resources(); }


static void post_con_error(const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!assetMgr)
    return;
  String tmpConStr;
  tmpConStr.vprintf(512, fmt, arg, anum);
  assetMgr->getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::ERROR, tmpConStr, NULL, NULL);
}
static void post_con_warning(const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!assetMgr)
    return;
  String tmpConStr;
  tmpConStr.vprintf(512, fmt, arg, anum);
  assetMgr->getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::WARNING, tmpConStr, NULL, NULL);
}
static void post_con_note(const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!assetMgr)
    return;
  String tmpConStr;
  tmpConStr.vprintf(512, fmt, arg, anum);
  assetMgr->getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::NOTE, tmpConStr, NULL, NULL);
}
#define DSA_OVERLOADS_PARAM_DECL
#define DSA_OVERLOADS_PARAM_PASS
DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void post_con_error, post_con_error);
DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void post_con_warning, post_con_warning);
DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void post_con_note, post_con_note);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS


static DagorAsset *getAssetByName(const char *_name, int asset_type = -1)
{
  if (!_name || !_name[0])
    return NULL;

  String name(dd_get_fname(_name));
  remove_trailing_string(name, ".res.blk");
  name = DagorAsset::fpath2asset(name);

  const char *type = strchr(name, ':');
  if (type)
  {
    asset_type = assetMgr->getAssetTypeId(type + 1);
    if (asset_type == -1)
    {
      post_con_error("invalid type of asset: %s", name.str());
      return NULL;
    }
    return assetMgr->findAsset(String::mk_sub_str(name, type), asset_type);
  }
  else if (asset_type == -1)
    return assetMgr->findAsset(name);
  else
    return assetMgr->findAsset(name, asset_type);
}

static dag::ConstSpan<int> findAssetTypes(unsigned class_id)
{
  static int lst[4];
  int num = 0;

  if (!class_id)
    return {};
  for (int i = 0; i < assetTypeToClassId.size(); i++)
    if (assetTypeToClassId[i] == class_id)
    {
      lst[num] = i;
      num++;
      if (num * sizeof(lst[0]) == sizeof(lst))
        break;
    }
  return make_span_const(lst, num);
}
static DagorAsset *getAssetByResId(int res_id)
{
  const char *resName = resMap.getName(res_id);
  if (!resName)
    return nullptr;
  const char *s = strchr(resName, '?');
  return getAssetByName(s ? s + 1 : resName);
}
GameResourceFactory *findFactory(dag::Span<GameResourceFactory *> f, unsigned class_id)
{
  if (!class_id)
    return NULL;
  for (int i = 0; i < f.size(); i++)
    if (f[i]->getResClassId() == class_id)
      return f[i];
  return NULL;
}


static bool abc_on_get_res_name(int res_id, String &out_res_name)
{
  if (res_id < ASSET_RESID_BASE) //== fallback for old resources built separately
    return false;

  WinAutoLock lock(dabuild_cache_cs);
  if (DagorAsset *a = getAssetByResId(res_id - ASSET_RESID_BASE))
  {
    out_res_name = a->getName();
    return true;
  }
  return true;
}
static bool abc_resolve_res_handle(const char *aname, unsigned class_id, int &out_res_id)
{
  if (!aname || !*aname)
    return false;
  WinAutoLock lock(dabuild_cache_cs);
  int a_type = -1;
  DagorAsset *a;

  out_res_id = -1;
  if (class_id && class_id != 0xFFFFFFFFU)
  {
    dag::ConstSpan<int> a_types = findAssetTypes(class_id);
    if (!a_types.size())
    {
      post_con_error("request gameres class=%p is not mapped to any asset type", class_id);
      return true;
    }
    a = assetMgr->findAsset(aname, a_types);
    a_type = a ? a->getType() : a_types[0];
  }
  else
  {
    dag::ConstSpan<int> a_types =
      (class_id != 0xFFFFFFFFU) ? assetExpTypes : make_span_const(assetExpTypes).first(assetExpTypes.size() - 1);
    a = assetMgr->findAsset(aname, a_types);
    if (a && !assetMgr->isAssetNameUnique(*a, a_types))
    {
      post_con_error("cannot use ambiguous asset name <%s> without type qualification", aname);
      return true;
    }
  }
  if (!a)
  {
    if (class_id != 0xFFFFFFFFU)
    {
      gamereshooks::resolve_res_handle = NULL;
      out_res_id = gamereshooks::aux_game_res_handle_to_id(aname, class_id);
      gamereshooks::resolve_res_handle = abc_resolve_res_handle;
      if (out_res_id >= 0)
      {
        debug("cannot find asset <%s> of type <%s>, but can use already built gameRes", aname, assetMgr->getAssetTypeName(a_type));
        return false;
      }
      post_con_error("cannot find asset <%s> of type <%s>", aname, assetMgr->getAssetTypeName(a_type));
    }
    return true;
  }

  return resolve_res_handle(a, out_res_id);
}
static bool resolve_res_handle(const DagorAsset *a, int &out_res_id)
{
  G_ASSERT(a);
  if (a->getFileNameId() == -1) //== fallback for old resources built separately
    return false;

  if (!assetTypeToClassId[a->getType()] && a->getType() != texTypeId)
  {
    post_con_error("asset <%s> is not gameres", a->getNameTypified());
    return false;
  }

  int asset_nid = assetMap.addNameId(a->getNameTypified());
  if (asset_nid == assetRev.size())
    assetRev.push_back(0);
  G_ASSERT(asset_nid < assetRev.size());

  String tmpStr;
  if (assetRev[asset_nid])
    tmpStr.printf(128, "%d?%s:%s", assetRev[asset_nid], a->getName(), a->getTypeStr());
  else
    tmpStr.printf(128, "%s:%s", a->getName(), a->getTypeStr());
  out_res_id = resMap.addNameId(tmpStr) + ASSET_RESID_BASE;

  // post_con_note("%s:%p -> %s:%d -> %d",
  //   aname, class_id, tmpStr.str(), a_type, out_res_id);
  return true;
}
static bool abc_validate_game_res_id(int res_id, int &out_res_id)
{
  if (res_id < ASSET_RESID_BASE) //== fallback for old resources built separately
    return false;

  out_res_id = res_id;
  return true;
}
static bool abc_get_game_res_class_id(int res_id, unsigned &out_class_id)
{
  if (res_id < ASSET_RESID_BASE) //== fallback for old resources built separately
    return false;

  WinAutoLock lock(dabuild_cache_cs);
  DagorAsset *a = getAssetByResId(res_id - ASSET_RESID_BASE);
  if (!a)
  {
    out_class_id = 0;
    return true;
  }

  out_class_id = assetTypeToClassId[a->getType()];
  return true;
}
static bool abc_get_game_resource(int res_id, gameres_rrl_cptr_t rrl, dag::Span<GameResourceFactory *> f, GameResource *&out_res)
{
  if (res_id < ASSET_RESID_BASE) //== fallback for old resources built separately
    return false;

  WinAutoLock lock(dabuild_cache_cs);
  DagorAsset *a = getAssetByResId(res_id - ASSET_RESID_BASE);
  if (!a)
  {
    out_res = NULL;
    return true;
  }

  if (a->getType() == texTypeId)
  {
    TextureMetaData tmd;
    tmd.read(a->props, "PC");

    TEXTUREID tid = ::add_managed_texture(tmd.encode(String(260, "%s*", a->getName())));
    ::acquire_managed_tex(tid);

    out_res = (GameResource *)(uintptr_t) unsigned(tid);
    return true;
  }

  GameResourceFactory *fac = findFactory(f, assetTypeToClassId[a->getType()]);
  if (!fac)
  {
    post_con_error("no factory (classid=%p) to load asset %s", assetTypeToClassId[a->getType()],
      resMap.getName(res_id - ASSET_RESID_BASE));
    out_res = NULL;
    return true;
  }

  lock.unlock();
  out_res = fac->getGameResource(rrl, res_id);
  return true;
}
static bool abc_release_game_resource(int res_id, dag::Span<GameResourceFactory *> f) { return false; }
static bool abc_get_res_refs(int res_id, Tab<int> &out_refs)
{
  if (res_id < ASSET_RESID_BASE) //== fallback for old resources built separately
    return false;

  WinAutoLock lock(dabuild_cache_cs);
  DagorAsset *a = getAssetByResId(res_id - ASSET_RESID_BASE);
  out_refs.clear();
  if (!a)
    return true;

  IDagorAssetExporter *exp = assetMgr->getAssetExporter(a->getType());
  if (!exp)
    return true;

  IDagorAssetRefProvider *refResv = assetMgr->getAssetRefProvider(a->getType());
  if (!refResv)
    return true;

  Tab<IDagorAssetRefProvider::Ref> refList(tmpmem);
  refResv->getAssetRefs(*a, refList);
  for (int i = 0; i < refList.size(); ++i)
  {
    if (refList[i].flags & refResv->RFLG_EXTERNAL)
    {
      if (!refList[i].getAsset())
        out_refs.push_back(-1);
      else
        out_refs.push_back(
          gamereshooks::aux_game_res_handle_to_id(refList[i].refAsset->getName(), assetTypeToClassId[refList[i].refAsset->getType()]));
    }
  }
  return true;
}

// separateModelMatToDescBin strips materials into a "*Desc.bin" sidecar (exp_rendInst/exp_dynModel),
// normally merged only when loading a fully prebuilt game (av_appwnd.cpp/de_engine.cpp) - the respack
// fast path needs it too; a locally-rebuilt asset has materials embedded and doesn't.
// Resolved once, like the game itself does at startup (no live-reload if a batch export changes it later).
static bool matDescsLoaded = false;
static void ensure_mat_descs_loaded()
{
  if (matDescsLoaded || bindAssetsBlock.isEmpty())
    return;
  matDescsLoaded = true;

  const DataBlock &expblk = *bindAssetsBlock.getBlockByNameEx("export");
  const DataBlock &buildblk = *bindAssetsBlock.getBlockByNameEx("build");
  const DataBlock &pkgblk = *expblk.getBlockByNameEx("packages");
  static const char *target_str = "PC"; // on-demand builds are always PC, no profile

  struct TypeDesc
  {
    const char *typeName;
    DataBlock *desc;
  };
  const TypeDesc types[] = {{"rendInst", &gameres_rendinst_desc}, {"dynModel", &gameres_dynmodel_desc}};

  // av_appwnd.cpp/de_engine.cpp lock out further pack/desc loading after their own prebuilt-resource scan;
  // bypass that lockdown for the duration of our own load, same as before it was installed.
  auto savedConfirmHook = gamereshooks::on_gameres_pack_load_confirm;
  gamereshooks::on_gameres_pack_load_confirm = nullptr;

  for (const TypeDesc &t : types)
  {
    const DataBlock &tblk = *buildblk.getBlockByNameEx(t.typeName);
    const char *descListOutPath = tblk.getStr("descListOutPath", nullptr);
    if (!tblk.getBool("separateModelMatToDescBin", false) || !descListOutPath || !*descListOutPath)
      continue;

    // gameres_append_desc() dedups by file path forever (sets a same-named bool once loaded), so a
    // stale invalidate_respack_caches() reload would otherwise silently keep the old data - force a
    // clean reread of every path here.
    t.desc->reset();

    for (int pkid = -1; pkid < (int)pkgblk.blockCount(); pkid++)
    {
      const char *pkname = pkid < 0 ? nullptr : pkgblk.getBlock(pkid)->getBlockName();

      // Merge base first: a resDiff-built patch only carries changed assets, so unchanged ones need base too.
      String destBase, packFnamePrefix;
      assethlp::build_package_dest_strings(destBase, packFnamePrefix, expblk, pkname, "%appDir", target_str, nullptr,
        /*patch_build*/ false);
      String descFname(260, "%s%s.bin", destBase.str(), descListOutPath);
      simplify_fname(descFname);
      gameres_append_desc(*t.desc, descFname, descFname, /*allow_override*/ false);

      if (dabuild->isPatchBuildActive(pkname, target_str))
      {
        String patchDestBase, patchPackFnamePrefix;
        assethlp::build_package_dest_strings(patchDestBase, patchPackFnamePrefix, expblk, pkname, "%appDir", target_str, nullptr,
          /*patch_build*/ true);
        String patchDescFname(260, "%s%s.bin", patchDestBase.str(), descListOutPath);
        simplify_fname(patchDescFname);
        gameres_append_desc(*t.desc, patchDescFname, patchDescFname, /*allow_override*/ true);
      }
    }

    gameres_final_optimize_desc(*t.desc, descListOutPath);
  }

  gamereshooks::on_gameres_pack_load_confirm = savedConfirmHook;
}

class AssetCacheLoadCB final : public MemoryLoadCB
{
public:
  String path;
  int cacheHeaderSize;

  AssetCacheLoadCB(const MemoryChainedData *_ch, bool auto_delete, String &&path, int header_size) :
    MemoryLoadCB(_ch, auto_delete), path(path), cacheHeaderSize(header_size)
  {}
  const char *getTargetName() override { return path.empty() ? MemoryLoadCB::getTargetName() : path.c_str(); }
  int tell() override { return MemoryLoadCB::tell() + cacheHeaderSize; }
  void seekto(int position) override { MemoryLoadCB::seekto(position - cacheHeaderSize); }
};

static bool abc_load_game_resource_pack(int res_id, dag::Span<GameResourceFactory *> f)
{
  if (res_id < ASSET_RESID_BASE) //== fallback for old resources built separately
    return false;

  WinAutoLock lock(dabuild_cache_cs);
  DagorAsset *a = getAssetByResId(res_id - ASSET_RESID_BASE);
  if (!a)
    return true;

  int assetClassId = assetTypeToClassId[a->getType()];
  GameResourceFactory *fac = findFactory(f, assetClassId);
  if (!fac)
    return true;

  IDagorAssetExporter *exp = assetMgr->getAssetExporter(a->getType());
  if (!exp)
    return true;

  Tab<int> tmpRefList(tmpmem);
  if (IDagorAssetRefProvider *refResv = assetMgr->getAssetRefProvider(a->getType()))
  {
    Tab<IDagorAssetRefProvider::Ref> refList;
    refResv->getAssetRefs(*a, refList);
    for (int i = 0; i < refList.size(); ++i)
    {
      if (refList[i].flags & refResv->RFLG_BROKEN)
      {
        if (refList[i].flags & refResv->RFLG_OPTIONAL)
          post_con_warning("%s: optional ref <%s> is broken", a->getName(), refList[i].getBrokenRef());
        else
        {
          post_con_error("%s: required ref <%s> is broken", a->getName(), refList[i].getBrokenRef());
          return true;
        }
        continue;
      }

      if (refList[i].flags & refResv->RFLG_EXTERNAL)
      {
        if (!refList[i].getAsset())
          tmpRefList.push_back(-1);
        else
          tmpRefList.push_back(gamereshooks::aux_game_res_handle_to_id(refList[i].refAsset->getName(),
            assetTypeToClassId[refList[i].refAsset->getType()]));
      }
    }
  }

  mkbindump::BinDumpSaveCB cwr(8 << 20, _MAKE4C('PC'), false);
  cwr.setFastBuildFlag(true);
  PassThroughLogWriter lw(buildLog);
  dabuild->setupReports(&lw, NULL);
  String cachePath;
  int dataOffset = 0;

  // save caches for dynModel and rendInst so ShaderMatVdata could setup Sbuffer::IReloadData for case of driver reset
  bool force_cache_store = (assetClassId == DynModelGameResClassId || assetClassId == RendInstGameResClassId);

  if (force_cache_store)
    ensure_mat_descs_loaded();

  if (dabuild->getBuiltRes(*a, cwr, exp, assetlocalprops::makePath("cache"), cachePath, dataOffset, force_cache_store))
  {
    AssetCacheLoadCB crd(cwr.getRawWriter().getMem(), false, eastl::move(cachePath), dataOffset);

    lock.unlock();
    fac->loadGameResourceData(res_id, crd);
    lock.lock();

    if (!tmpRefList.empty())
      fac->createGameResource(nullptr, res_id, tmpRefList.data(), tmpRefList.size());
    else
      fac->createGameResource(nullptr, res_id, NULL, 0);
  }

  dabuild->setupReports(NULL, NULL);
  return true;
}

static void install_gameres_hooks()
{
  gamereshooks::resolve_res_handle = abc_resolve_res_handle;
  gamereshooks::get_res_refs = abc_get_res_refs;
  gamereshooks::on_validate_game_res_id = abc_validate_game_res_id;
  gamereshooks::on_get_game_res_class_id = abc_get_game_res_class_id;
  gamereshooks::on_get_game_resource = abc_get_game_resource;
  gamereshooks::on_release_game_resource = abc_release_game_resource;
  gamereshooks::on_load_game_resource_pack = abc_load_game_resource_pack;
  gamereshooks::on_get_res_name = &abc_on_get_res_name;
  post_con_note("assetBuildCache installed hooks to gameres system");
}
static void reset_gameres_hooks()
{
  gamereshooks::resolve_res_handle = 0;
  gamereshooks::get_res_refs = NULL;
  gamereshooks::on_validate_game_res_id = 0;
  gamereshooks::on_get_game_res_class_id = 0;
  gamereshooks::on_get_game_resource = 0;
  gamereshooks::on_release_game_resource = 0;
  gamereshooks::on_load_game_resource_pack = 0;
  gamereshooks::on_get_res_name = 0;
}

int dabuildcache::get_asset_res_id(const DagorAsset &a)
{
  WinAutoLock lock(dabuild_cache_cs);
  int res_id = -1;
  if (resolve_res_handle(&a, res_id))
    return res_id;
  return gamereshooks::aux_game_res_handle_to_id(a.getName(), assetTypeToClassId[a.getType()]);
}

bool dabuildcache::invalidate_asset(const DagorAsset &a, bool free_unused_resources)
{
  WinAutoLock lock(dabuild_cache_cs);
  if (a.getType() == texTypeId)
    ::reload_changed_texture_asset(a);

  int anid = assetMap.getNameId(a.getNameTypified());
  if (anid != -1)
  {
    if (free_unused_resources)
    {
      GameResourceFactory *f = find_gameres_factory(assetTypeToClassId[a.getType()]);
      if (f)
        while (f->freeUnusedResources(nullptr, false)) {}
    }
    dabuild->invalidateBuiltRes(a, assetlocalprops::makePath("cache"));
    assetRev[anid]++;
  }
  return false;
}

bool dabuildcache::get_built_res_hash_xxh3(DagorAsset &asset, unsigned char out_hash_xxh3_128[16])
{
  IDagorAssetExporter *exp = asset.getMgr().getAssetExporter(asset.getType());
  memset(out_hash_xxh3_128, 0, 16);
  if (!exp || !dabuild)
    return false;
  unsigned classId = assetTypeToClassId[asset.getType()];
  mkbindump::BinDumpSaveCB cwr(1 << 20, _MAKE4C('PC'), false);
  cwr.setFastBuildFlag(true);
  bool build_result = false, from_grp = false;

  {
    WinAutoLock lock(dabuild_cache_cs);
    String cachePath;
    int dataOffset = 0;
    build_result = dabuild->getBuiltRes(asset, cwr, exp, assetlocalprops::makePath("cache"), cachePath, dataOffset, true, &from_grp);
    if (build_result && from_grp && (classId == RendInstGameResClassId || classId == DynModelGameResClassId))
      ensure_mat_descs_loaded();
  }

  if (!build_result)
    return false;

  XXH3_state_t hashState;
  ::XXH3_128bits_reset(&hashState);
  for (const auto *mem = cwr.getRawWriter().getMem(); mem && mem->used; mem = mem->next)
    ::XXH3_128bits_update(&hashState, mem->data, (int)mem->used);

  if (from_grp && (classId == RendInstGameResClassId || classId == DynModelGameResClassId))
  {
    // Only a respack-served result can have materials split into *Desc.bin (a fallback rebuild above always
    // embeds them via setFastBuildFlag). Safe to read gameres_*_desc unlocked here.
    DynamicMemGeneralSaveCB matCwr(tmpmem, 4 << 10, 4 << 10);
    const DataBlock *descRoot = (classId == RendInstGameResClassId) ? &gameres_rendinst_desc : &gameres_dynmodel_desc;
    if (const DataBlock *assetDesc = descRoot->getBlockByName(asset.getName()))
    {
      if (const DataBlock *texBlk = assetDesc->getBlockByName("tex"))
        texBlk->saveToTextStream(matCwr);
      if (const DataBlock *matBlk = assetDesc->getBlockByName("mat"))
        matBlk->saveToTextStream(matCwr);
    }
    if (matCwr.size())
      ::XXH3_128bits_update(&hashState, matCwr.data(), matCwr.size());
  }

  ::XXH128_canonicalFromHash((XXH128_canonical_t *)out_hash_xxh3_128, ::XXH3_128bits_digest(&hashState));
  return true;
}

void dabuildcache::invalidate_respack_caches()
{
  WinAutoLock lock(dabuild_cache_cs);
  matDescsLoaded = false;
  if (dabuild)
    dabuild->invalidateRespackCaches();
}
