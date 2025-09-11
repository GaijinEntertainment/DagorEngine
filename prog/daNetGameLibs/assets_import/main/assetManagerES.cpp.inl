// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/animCharEffectors.h>
#include <ecs/phys/animPhys.h>
#include <assets/asset.h>
#include <assets/assetHlp.h>
#include <assets/assetMsgPipe.h>
#include <assets/assetExporter.h>
#include <assets/assetChangeNotify.h>
#include <assets/assetRefs.h>
#include <assets/texAssetBuilderTextureFactory.h>
#include <gameRes/dag_stdGameResId.h>
#include <gameRes/dag_gameResSystem.h>
#include <folders/folders.h>
#include <util/dag_string.h>
#include <util/dag_watchdog.h>
#include <ioSys/dag_dataBlock.h>
#include <libTools/util/iLogWriter.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_direct.h>
#include <gui/dag_visualLog.h>
#include <EASTL/hash_map.h>
#include "assetManager.h"
#include "assetManagerEvents.h"
#include "assetStatus.h"

ECS_REGISTER_BOXED_TYPE(DagorAssetMgr, nullptr);

class MessagePipe final : public NullMsgPipe, public ILogWriter
{
public:
  void onAssetMgrMessage(int msg_type, const char *msg, DagorAsset *, const char *asset_src_fpath) override
  {
    G_STATIC_ASSERT(IDagorAssetMsgPipe::NOTE == (int)ILogWriter::NOTE && IDagorAssetMsgPipe::REMARK == (int)ILogWriter::REMARK &&
                    IDagorAssetMsgPipe::WARNING == (int)ILogWriter::WARNING && IDagorAssetMsgPipe::ERROR == (int)ILogWriter::ERROR &&
                    IDagorAssetMsgPipe::FATAL == (int)ILogWriter::FATAL);
    MessageType msgType = (MessageType)msg_type;
    if (asset_src_fpath)
    {
      const DagorSafeArg args[2] = {msg, asset_src_fpath};
      addMessageFmt(msgType, "AssetManager: %s, file:%s", args, 2);
    }
    else
    {
      const DagorSafeArg args[1] = {msg};
      addMessageFmt(msgType, "AssetManager: %s", args, 1);
    }
  }
  void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum) override
  {
    switch (type)
    {
      case ILogWriter::NOTE:
      case ILogWriter::REMARK: logmessage_fmt(LOGLEVEL_DEBUG, fmt, arg, anum); break;

      case ILogWriter::WARNING: logmessage_fmt(LOGLEVEL_WARN, fmt, arg, anum); break;

      case ILogWriter::ERROR:
      case ILogWriter::FATAL:
        errCount++;
        logmessage_fmt(LOGLEVEL_ERR, fmt, arg, anum);
        break;

      default: break;
    }
  }
  bool hasErrors() const override { return errCount > 0; }
  void startLog() override {}
  void endLog() override {}
};

static MessagePipe messagePipe;
ILogWriter *get_asset_manager_log_writer() { return &messagePipe; }
void invalidate_asset_dependencies(const DagorAsset &asset);

ECS_REGISTER_EVENT(AssetAddedEvent)
ECS_REGISTER_EVENT(AssetChangedEvent)
ECS_REGISTER_EVENT(AssetRemovedEvent)

class AssetManagerWatcher final : public IDagorAssetBaseChangeNotify
{
public:
  void onAssetBaseChanged(dag::ConstSpan<DagorAsset *> changed_assets,
    dag::ConstSpan<DagorAsset *> added_assets,
    dag::ConstSpan<DagorAsset *> removed_assets) override
  {
    for (auto asset : changed_assets)
    {
      visuallog::logmsg(String(0, " %s asset <%d> changed", asset->getName(), asset->getTypeStr()));
      if (asset->getType() == asset->getMgr().getTexAssetTypeId())
        ::reregister_texture_in_build_on_demand_factory(*asset);
      invalidate_asset_dependencies(*asset);
      g_entity_mgr->broadcastEvent(AssetChangedEvent(asset));
    }
    for (auto asset : added_assets)
    {
      visuallog::logmsg(String(0, " %s asset <%d> added", asset->getName(), asset->getTypeStr()));
      g_entity_mgr->broadcastEvent(AssetAddedEvent(asset));
    }
    for (auto asset : removed_assets)
    {
      visuallog::logmsg(String(0, " %s asset <%d> removed", asset->getName(), asset->getTypeStr()));
      g_entity_mgr->broadcastEvent(AssetRemovedEvent(asset));
    }
  }
};
static AssetManagerWatcher assetWatcher;

// We should create cached version of ecs components to allow execute hooks from thread, without access to ecs.
// else we will break ecs manager state, because every ecs query should be finished before manager.tick
static const DagorAssetMgr *cachedAssetManager = nullptr; // it's boxed type, so can be saved in pointer
static dag::Vector<int> cachedExportableTypes;

const DagorAssetMgr *get_asset_manager() { return cachedAssetManager; }

dag::ConstSpan<int> get_exportable_types() { return make_span_const(cachedExportableTypes); }

extern "C" IDaBuildInterface *__stdcall get_dabuild_interface();

ECS_ON_EVENT(on_appear)
static void init_asset_manager_es(const ecs::Event &,
  DagorAssetMgr &asset__manager,
  const ecs::string &asset__applicationBlkPath,
  const ecs::string &asset__baseFolder,
  const ecs::string &asset__pluginsFolder,
  ecs::string &asset__baseFolderAbsPath,
  int &asset__rendInstType)
{
  DataBlock applicationBlk;

  if (!applicationBlk.load(asset__applicationBlkPath.c_str()))
  {
    logerr("asset_manager failed to load application.blk at %s", asset__applicationBlkPath);
    return;
  }

  DataBlock::fatalOnMissingFile = false;
  DataBlock::fatalOnLoadFailed = false;
  DataBlock::fatalOnBadVarType = false;
  DataBlock::fatalOnMissingVar = false;

  // asset loading can take a lot of time, better to disable watchdog at all, because it can spam extra fatals
  watchdog_set_option(WATCHDOG_OPTION_TRIG_THRESHOLD, WATCHDOG_DISABLE);

  asset__manager.setMsgPipe(&messagePipe);

  // Most likely we should support only exportable types
  auto exportBlock = applicationBlk.getBlockByNameEx("assets")->getBlockByNameEx("export");
  asset__manager.setupAllowedTypes(*exportBlock->getBlockByNameEx("types"), exportBlock);

  String applicationDirectory = folders::get_game_dir();
  applicationBlk.setStr("appDir", applicationDirectory);
  const char *assetCachePath = exportBlock->getStr("assetCache", "/cache");
  assetlocalprops::init(applicationDirectory, assetCachePath);

  DataBlock::setRootIncludeResolver(applicationDirectory + "/..");
  String assetsBasePath(0, "%s/%s", applicationDirectory, asset__baseFolder);
  if (!dd_dir_exists(assetsBasePath.c_str()))
  {
    debug("Folder with custom assets %s does not exist. Creating empty folder", assetsBasePath.c_str());
    dd_mkdir(assetsBasePath.c_str());
  }
  asset__baseFolderAbsPath = simplify_fname(assetsBasePath);

  asset__manager.loadAssetsBase(assetsBasePath, "global");

  dabuildcache::init(folders::get_exe_dir(), &messagePipe, get_dabuild_interface());

  dabuildcache::bind_with_mgr(asset__manager, applicationBlk, applicationDirectory, asset__pluginsFolder.c_str());
  texconvcache::set_fast_conv(true /*fast*/);

  asset__manager.enableChangesTracker(true);
  asset__manager.subscribeBaseUpdateNotify(&assetWatcher);

  install_asset_manager_hooks();

  texconvcache::init_build_on_demand_tex_factory(asset__manager, &messagePipe);

  for (int i = 0, e = asset__manager.getAssetTypesCount(); i < e; ++i)
  {
    IDagorAssetExporter *exp = asset__manager.getAssetExporter(i);
    if (exp)
    {
      int classId = exp->getGameResClassId();
      cachedExportableTypes.push_back(i);
      if (classId == RendInstGameResClassId)
        asset__rendInstType = i;
    }
  }
  cachedAssetManager = &asset__manager;
}

ECS_NO_ORDER
static void asset_manager_track_changes_es(const ecs::UpdateStageInfoAct &, DagorAssetMgr &asset__manager)
{
  if (asset__manager.trackChangesContinuous(-1))
    dabuildcache::post_base_update_notify();
}

ECS_REQUIRE(DagorAssetMgr asset__manager)
ECS_ON_EVENT(on_disappear)
static void destoy_asset_manager_es(const ecs::Event &) { dabuildcache::term(); }


void get_asset_dependencies(DagorAsset &asset, Tab<IDagorAssetRefProvider::Ref> &refs)
{
  refs.clear();
  if (auto assetMgr = get_asset_manager())
  {
    if (IDagorAssetRefProvider *refProvider = assetMgr->getAssetRefProvider(asset.getType()))
    {
      refProvider->getAssetRefs(asset, refs);
      for (int i = refs.size() - 1; i >= 0; i--)
        if (!refs[i].getAsset() && !refs[i].getBrokenRef())
          erase_items(refs, i, 1);
    }
  }
}

struct AssetRefs
{
  Tab<IDagorAssetRefProvider::Ref> refs;
  bool invalid = true;
};

// the key has name:type format, because we can have different assets with same name a:tex, a:rendInst
static eastl::hash_map<eastl::string, AssetLoadingStatus> assetsStatus;
static eastl::hash_map<eastl::string, eastl::unique_ptr<AssetRefs>> assetsRefs;
// store refs in ptr because we can relocate data on cache update in recursion


void invalidate_asset_dependencies(const DagorAsset &asset)
{
  auto it = assetsRefs.find_as(asset.getNameTypified().c_str());
  if (it != assetsRefs.end())
    it->second->invalid = true;
}

const Tab<IDagorAssetRefProvider::Ref> &get_asset_dependencies(DagorAsset &asset)
{
  String assetName = asset.getNameTypified();
  auto it = assetsRefs.find_as(assetName.c_str());
  if (it == assetsRefs.end())
    it = assetsRefs.emplace(assetName.c_str(), eastl::make_unique<AssetRefs>()).first;

  AssetRefs &refs = *it->second;
  if (refs.invalid)
  {
    get_asset_dependencies(asset, refs.refs);
    refs.invalid = false;
  }
  return refs.refs;
}

AssetLoadingStatus get_asset_status(DagorAsset &asset, bool update_status)
{
  // we cannot cache asset.getNameTypified(), because it works on highly-temporal buffer
  const char *assetName = asset.getName();
  String assetNameType = asset.getNameTypified();

  if (asset.getTypeStr() == eastl::string_view("tex"))
  {
    return get_texture_status(assetName);
  }
  if (!update_status)
    return assetsStatus[assetNameType.c_str()];

  const Tab<IDagorAssetRefProvider::Ref> &refs = get_asset_dependencies(asset);

  AssetLoadingStatus finalStatus = AssetLoadingStatus::NotLoaded;
  if (!refs.empty())
  {
    int notLoaded = 0, loading = 0, loaded = 0, loadedWithErros = 0;
    for (const auto &ref : refs)
    {
      if (DagorAsset *a = ref.getAsset())
      {
        if (a->getName() == eastl::string_view("prefab"))
          continue;
        AssetLoadingStatus status = get_asset_status(*a, update_status);
        switch (status)
        {
          case AssetLoadingStatus::NotLoaded: notLoaded++; break;
          case AssetLoadingStatus::Loaded: loaded++; break;
          case AssetLoadingStatus::Loading: loading++; break;
          case AssetLoadingStatus::LoadedWithErrors: loadedWithErros++; break;
        }
      }
      else
        loadedWithErros++;
    }
    if (loading > 0)
      finalStatus = AssetLoadingStatus::Loading;
    else if (loadedWithErros > 0)
      finalStatus = AssetLoadingStatus::LoadedWithErrors;
    else if (loaded > 0)
      finalStatus = AssetLoadingStatus::Loaded;
  }
  else
  {
    if (IDagorAssetExporter *exp = get_asset_manager()->getAssetExporter(asset.getType()))
    {
      GameResHandle handle = GAMERES_HANDLE_FROM_STRING(assetName);
      bool gameResLoaded = is_game_resource_loaded_nolock(handle, exp->getGameResClassId());
      finalStatus = gameResLoaded ? AssetLoadingStatus::Loaded : AssetLoadingStatus::NotLoaded;
    }
    else
    {
      finalStatus = AssetLoadingStatus::Loaded;
    }
  }
  assetsStatus[assetNameType.c_str()] = finalStatus;
  return finalStatus;
}

static bool is_asset_depends_on(const DagorAsset &dependent_asset, DagorAsset &main_asset)
{
  const Tab<IDagorAssetRefProvider::Ref> &refs = get_asset_dependencies(main_asset);

  for (const auto &ref : refs)
  {
    if (auto asset = ref.getAsset())
      if (asset->getNameId() == dependent_asset.getNameId())
      {
        return true;
      }
  }
  return false;
}

static void track_a2d_es(const AssetChangedEvent &evt)
{
  const DagorAsset *changedAsset = evt.get<0>();
  if (strcmp("a2d", changedAsset->getTypeStr()) != 0)
    return;
  auto manager = get_asset_manager();
  int animTreeId = manager->getAssetTypeId("animTree");
  auto animTrees = manager->getFilteredAssets(make_span_const(&animTreeId, 1));
  for (int treeId : animTrees)
  {
    DagorAsset &animTree = manager->getAsset(treeId);
    if (is_asset_depends_on(*changedAsset, animTree))
    {
      g_entity_mgr->broadcastEvent(AssetChangedEvent(&animTree));
    }
  }
}

static void track_animchar_assets_es(const AssetChangedEvent &evt,
  ecs::EntityId eid,
  AnimV20::AnimcharBaseComponent &animchar,
  AnimV20::AnimcharRendComponent &animchar_render,
  const ecs::string &animchar__res,
  AnimcharNodesMat44 *animchar_node_wtm,
  bool *animchar__animStateDirty)
{
  const DagorAsset *changedAsset = evt.get<0>();
  const char *assetType = changedAsset->getTypeStr();
  bool animTreeChanged = strcmp("animTree", assetType) == 0 && animchar.getAnimGraph() != nullptr;
  bool dynModelChanged = strcmp("dynModel", assetType) == 0;
  if (!(animTreeChanged || dynModelChanged))
    return;

  DagorAsset *animcharAsset = get_asset_manager()->findAsset(animchar__res.c_str());
  if (animcharAsset == nullptr)
    return;

  if (!is_asset_depends_on(*changedAsset, *animcharAsset))
    return;


  GameResHandle handle = GAMERES_HANDLE_FROM_STRING(animchar__res.c_str());
  AnimV20::IAnimCharacter2 *animCharSource = (AnimV20::IAnimCharacter2 *)::get_game_resource_ex(handle, CharacterGameResClassId);

  if (animTreeChanged)
    animCharSource->baseComp().cloneTo(&animchar, true);
  if (dynModelChanged)
    animCharSource->rendComp().cloneTo(&animchar_render, true, animCharSource->getNodeTree());

  if (animchar_node_wtm && animchar_node_wtm->nwtm.size() != animCharSource->getNodeTree().nodeCount())
    animchar_node_wtm->onLoaded(*g_entity_mgr, eid);

  if (animchar__animStateDirty)
    *animchar__animStateDirty = true;
  g_entity_mgr->sendEventImmediate(eid, InvalidateEffectorData());
  g_entity_mgr->sendEventImmediate(eid, InvalidateAnimatedPhys());

  ::release_game_resource(handle);
}
