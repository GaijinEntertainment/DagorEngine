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
#include <3d/dag_texMgr.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/iLogWriter.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_string.h>
#include <util/dag_texMetaData.h>
#include <util/dag_safeArg.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>

static const int ASSET_RESID_BASE = 128 << 10;

static String startDir, tmpStr;
static IDaBuildInterface *dabuild = NULL;
static DagorAssetMgr *assetMgr = NULL;
static ILogWriter *buildLog = NULL;
static FastNameMapEx resMap;
static FastNameMap assetMap;
static Tab<unsigned> assetRev(inimem);
static Tab<unsigned> assetTypeToClassId(inimem);
static Tab<int> assetExpTypes(inimem);
static Tab<int> tmpRefList(inimem);
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
  virtual void onAssetBaseChanged(dag::ConstSpan<DagorAsset *> changed_assets, dag::ConstSpan<DagorAsset *> added_assets,
    dag::ConstSpan<DagorAsset *> removed_assets)
  {
    for (int i = 0; i < changed_assets.size(); i++)
    {
      const DagorAsset &a = *changed_assets[i];

      int anid = assetMap.getNameId(a.getNameTypified());
      if (anid != -1)
        assetRev[anid]++;
    }
  }
  virtual void onAssetRemoved(int asset_name_id, int asset_type) {}
  virtual void onAssetChanged(const DagorAsset &a, int asset_name_id, int asset_type)
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
  debug_ctx("dabuildcache inited with static dabuild lib");
  return true;
}

bool dabuildcache::init(const char *start_dir, ILogWriter *l)
{
  startDir = start_dir;
  dabuild = get_dabuild(startDir + "/daBuild" DAGOR_DLL);
  if (!dabuild)
  {
    debug("ERR: cannot load daBuild" DAGOR_DLL);
    return false;
  }
  debug_ctx("daBuild loaded");
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

  dabuild->unloadExporterPlugins();
  dabuild->term();
  release_dabuild(dabuild);
  dabuild = NULL;
  debug_ctx("daBuild unloaded");
}

IDaBuildInterface *dabuildcache::get_dabuild() { return dabuild; }

int dabuildcache::bind_with_mgr(DagorAssetMgr &mgr, DataBlock &appblk, const char *appdir, const char *ddsx_plugins_path)
{
  if (!dabuild)
    return -1;

  if (appblk.getStr("shaders", NULL))
    appblk.setStr("shadersAbs", String(260, "%s/%s", appdir, appblk.getStr("shaders", NULL)));
  else
    appblk.setStr("shadersAbs", String(260, "%s/../common/compiledShaders/tools", startDir.str()));

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


static String tmpConStr;
static void post_con_error(const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!assetMgr)
    return;
  tmpConStr.vprintf(512, fmt, arg, anum);
  assetMgr->getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::ERROR, tmpConStr, NULL, NULL);
}
static void post_con_warning(const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!assetMgr)
    return;
  tmpConStr.vprintf(512, fmt, arg, anum);
  assetMgr->getMsgPipe().onAssetMgrMessage(IDagorAssetMsgPipe::WARNING, tmpConStr, NULL, NULL);
}
static void post_con_note(const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!assetMgr)
    return;
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
  if (DagorAsset *a = getAssetByResId(res_id - ASSET_RESID_BASE))
  {
    out_res_name = a->getName();
    return true;
  }
  return true;
}
static bool abc_resolve_res_handle(GameResHandle rh, unsigned class_id, int &out_res_id)
{
  const char *aname = (const char *)rh;
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
      out_res_id = gamereshooks::aux_game_res_handle_to_id(rh, class_id);
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

  DagorAsset *a = getAssetByResId(res_id - ASSET_RESID_BASE);
  if (!a)
  {
    out_class_id = 0;
    return true;
  }

  out_class_id = assetTypeToClassId[a->getType()];
  return true;
}
static bool abc_get_game_resource(int res_id, dag::Span<GameResourceFactory *> f, GameResource *&out_res)
{
  if (res_id < ASSET_RESID_BASE) //== fallback for old resources built separately
    return false;

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

  out_res = fac->getGameResource(res_id);
  return true;
}
static bool abc_release_game_resource(int res_id, dag::Span<GameResourceFactory *> f) { return false; }
static bool abc_release_game_res2(GameResHandle rh, dag::Span<GameResourceFactory *> f)
{
  const char *aname = (const char *)rh;
  DagorAsset *a = assetMgr->findAsset(aname, assetExpTypes);

  if (!a)
  {
    post_con_error("try release non-existing asset <%s>", aname);
    return true;
  }
  if (!assetMgr->isAssetNameUnique(*a, assetExpTypes))
  {
    post_con_error("cannot use ambiguous asset name <%s> without type qualification", aname);
    return true;
  }
  if (a->getFolderIndex() == -1) //== fallback for old resources built separately
    return false;

  int asset_nid = assetMap.getNameId(a->getNameTypified());
  if (asset_nid == -1)
  {
    post_con_error("try release not created asset <%s>", aname);
    return true;
  }
  if (assetRev[asset_nid] != 0)
  {
    post_con_error("disabled non-safe release of asset <%s>, leaks may occur", aname);
    return true;
  }
  return false;
}
static bool abc_get_res_refs(int res_id, Tab<int> &out_refs)
{
  if (res_id < ASSET_RESID_BASE) //== fallback for old resources built separately
    return false;

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

  Tab<IDagorAssetRefProvider::Ref> refList(refResv->getAssetRefs(*a), tmpmem);
  for (int i = 0; i < refList.size(); ++i)
  {
    if (refList[i].flags & refResv->RFLG_EXTERNAL)
    {
      if (!refList[i].getAsset())
        out_refs.push_back(-1);
      else
        out_refs.push_back(gamereshooks::aux_game_res_handle_to_id(GAMERES_HANDLE_FROM_STRING(refList[i].refAsset->getName()),
          assetTypeToClassId[refList[i].refAsset->getType()]));
    }
  }
  return true;
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

  IDagorAssetRefProvider *refResv = assetMgr->getAssetRefProvider(a->getType());
  Tab<IDagorAssetRefProvider::Ref> refList;

  int tmp_top = tmpRefList.size();
  if (refResv)
  {
    refList = refResv->getAssetRefs(*a);
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
          tmpRefList.push_back(gamereshooks::aux_game_res_handle_to_id(GAMERES_HANDLE_FROM_STRING(refList[i].refAsset->getName()),
            assetTypeToClassId[refList[i].refAsset->getType()]));
      }
    }
  }

  mkbindump::BinDumpSaveCB cwr(8 << 20, _MAKE4C('PC'), false);
  PassThroughLogWriter lw(buildLog);
  dabuild->setupReports(&lw, NULL);
  String cachePath;
  int dataOffset = 0;

  bool saveAllAssets = false;

// case of usage dabuildCache in game
#ifdef _TARGET_EXPORTERS_STATIC
  if (assetClassId == DynModelGameResClassId || assetClassId == RendInstGameResClassId)
  {
    saveAllAssets = true;
  }
#endif


  if (dabuild->getBuiltRes(*a, cwr, exp, assetlocalprops::makePath("cache"), cachePath, dataOffset, saveAllAssets))
  {
#ifndef _TARGET_EXPORTERS_STATIC
    cachePath.clear();
    dataOffset = 0;
#endif
    AssetCacheLoadCB crd(cwr.getRawWriter().getMem(), false, eastl::move(cachePath), dataOffset);

    fac->loadGameResourceData(res_id, crd);
    if (tmpRefList.size() > tmp_top)
    {
      Tab<int> refList;
      refList = make_span_const(tmpRefList).subspan(tmp_top);
      fac->createGameResource(res_id, refList.data(), refList.size());
    }
    else
      fac->createGameResource(res_id, NULL, 0);
  }

  dabuild->setupReports(NULL, NULL);
  tmpRefList.resize(tmp_top);
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
  gamereshooks::on_release_game_res2 = abc_release_game_res2;
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
  gamereshooks::on_release_game_res2 = 0;
  gamereshooks::on_load_game_resource_pack = 0;
  gamereshooks::on_get_res_name = 0;
}

int dabuildcache::get_asset_res_id(const DagorAsset &a)
{
  int res_id = -1;
  if (resolve_res_handle(&a, res_id))
    return res_id;
  return gamereshooks::aux_game_res_handle_to_id(GAMERES_HANDLE_FROM_STRING(a.getName()), assetTypeToClassId[a.getType()]);
}

bool dabuildcache::invalidate_asset(const DagorAsset &a, bool free_unused_resources)
{
  if (a.getType() == texTypeId)
    ::reload_changed_texture_asset(a);

  int anid = assetMap.getNameId(a.getNameTypified());
  if (anid != -1)
  {
    if (free_unused_resources)
    {
      GameResourceFactory *f = find_gameres_factory(assetTypeToClassId[a.getType()]);
      if (f)
        while (f->freeUnusedResources(false)) {}
    }
    dabuild->invalidateBuiltRes(a, assetlocalprops::makePath("cache"));
    assetRev[anid]++;
  }
  return false;
}
