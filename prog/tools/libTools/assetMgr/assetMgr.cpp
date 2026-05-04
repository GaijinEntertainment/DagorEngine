// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_fileIo.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <assets/assetFolder.h>
#include <assets/assetMsgPipe.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include "assetCreate.h"
#include "vAssetRule.h"
#include "assetMgrPrivate.h"
#include <libTools/util/strUtil.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_ioUtils.h>
#include <osApiWrappers/dag_direct.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_fastStrMap.h>
#include <util/dag_fastIntList.h>
#include <util/dag_globDef.h>
#include <util/dag_jobPool.h>
#include <math/dag_e3dColor.h>
#include <memory/dag_memStat.h>
#include <perfMon/dag_statDrv.h>
// #include <debug/dag_debug.h>

static NullMsgPipe nullPipe;
static bool get_split_not_sep_for_asset(DagorAsset *a);

DagorAssetMgr::DagorAssetMgr(bool use_shared_name_map_for_assets) :
  assets(midmem),
  vaRule(midmem),
  folders(midmem),
  msgPipe(&nullPipe),
  updNotify(midmem),
  perTypeNotify(midmem),
  baseRoots(midmem),
  tracker(NULL),
  updBaseNotify(midmem),
  perTypeExp(midmem),
  perTypeRefProv(midmem),
  texAssetType(-1),
  texRtMipGenAllowed(true),
  sharedAssetNameMapOwner(use_shared_name_map_for_assets ? new DataBlock : nullptr),
  perTypeNameIds(midmem),
  perFolderStartAssetIdx(midmem)
{
  debug("%p.DagorAssetMgr() use_shared_name_map_for_assets=%s", this, sharedAssetNameMapOwner ? "YES" : "no");
}
DagorAssetMgr::~DagorAssetMgr()
{
  clear();
  enableChangesTracker(false);
  WriteGuard guard(mutex);
  del_it(sharedAssetNameMapOwner);
}

void DagorAssetMgr::clear()
{
  WriteGuard guard(mutex);
  for (int i = 0; i < perTypeExp.size(); i++)
    if (perTypeExp[i])
      perTypeExp[i]->onUnregister();
  perTypeExp.clear();

  for (int i = 0; i < perTypeRefProv.size(); i++)
    if (perTypeRefProv[i])
      perTypeRefProv[i]->onUnregister();
  perTypeRefProv.clear();

  clear_all_ptr_items(assets);
  clear_all_ptr_items(vaRule);
  clear_all_ptr_items(folders);
  assetNames.reset();
  assetFileNames.reset();
  typeNames.reset();
  texAssetType = -1;
  nspaceNames.reset();
  perTypeNameIds.clear();
  perFolderStartAssetIdx.clear();
  baseRoots.clear();
  if (tracker)
  {
    enableChangesTracker(false);
    enableChangesTracker(true);
  }

  defDxpPath = NULL;
  defGrpPath = NULL;
  locDefDxp = NULL;
  locDefGrp = NULL;
  basePkg = NULL;
  texRtMipGenAllowed = true;
  if (sharedAssetNameMapOwner)
    sharedAssetNameMapOwner->reset();
}

IDagorAssetMsgPipe *DagorAssetMgr::setMsgPipe(IDagorAssetMsgPipe *pipe)
{
  WriteGuard guard(mutex);
  IDagorAssetMsgPipe *old = (msgPipe == &nullPipe) ? NULL : msgPipe;
  msgPipe = pipe ? pipe : &nullPipe;
  return old;
}

void DagorAssetMgr::setupAllowedTypes(const DataBlock &blk, const DataBlock *exp_blk)
{
  WriteGuard guard(mutex);
  int nid = blk.getNameId("type");
  for (int i = 0; i < blk.paramCount(); i++)
    if (blk.getParamType(i) == DataBlock::TYPE_STRING && blk.getParamNameId(i) == nid)
      typeNames.addNameId(blk.getStr(i));
  texAssetType = typeNames.getNameId("tex");

  perTypeNameIds.resize(typeNames.nameCount());
  perTypePkg.resize(typeNames.nameCount());
  perTypePack.resize(typeNames.nameCount());

  defDxpPath = NULL;
  defGrpPath = NULL;
  locDefDxp = NULL;
  locDefGrp = NULL;
  if (exp_blk && exp_blk->getBlockByName("perTypeDest"))
  {
    const DataBlock *perTypeDest = exp_blk->getBlockByName("perTypeDest")->getBlockByNameEx("pack");
    for (int i = 0; i < typeNames.nameCount(); i++)
      perTypePack[i] = perTypeDest->getStr(typeNames.getName(i), NULL);

    perTypeDest = exp_blk->getBlockByName("perTypeDest")->getBlockByNameEx("package");
    for (int i = 0; i < typeNames.nameCount(); i++)
      perTypePkg[i] = perTypeDest->getStr(typeNames.getName(i), NULL);
  }

  if (exp_blk)
  {
    defDxpPath = exp_blk->getStr("defaultDdsxTexPack", NULL);
    defGrpPath = exp_blk->getStr("defaultGameResPack", NULL);
    locDefDxp = exp_blk->getBlockByNameEx("folderDefaults")->getStr("ddsxTexPack", NULL);
    locDefGrp = exp_blk->getBlockByNameEx("folderDefaults")->getStr("gameResPack", NULL);
    basePkg = exp_blk->getStr("defaultBasePackage", NULL);
  }
  texRtMipGenAllowed = exp_blk ? exp_blk->getBool("rtMipGenAllowed", true) : true;
  post_msg(*msgPipe, msgPipe->NOTE, "registered %d allowed asset types", typeNames.nameCount());
}

static bool asset_name_less(const char *a, const char *b)
{
  int cmp = stricmp(a, b);
  if (cmp == 0)
    cmp = strcmp(a, b);
  return cmp < 0;
}

struct DagorAssetMgr::LoadedFolder
{
  Tab<DagorAssetPrivate *> assets;
  Tab<LoadedFolder *> subFolders;
  LoadedFolder *parent;
  DagorAssetFolder *folder;
  uint32_t subAssetCount = 0;

  LoadedFolder(DagorAssetFolder *f, LoadedFolder *p) : folder(f), parent(p), assets(tmpmem), subFolders(tmpmem) {}

  ~LoadedFolder()
  {
    G_ASSERT(assets.empty());
    clear_all_ptr_items(assets);
    clear_all_ptr_items(subFolders);
    del_it(folder);
  }

  void addSub(LoadedFolder *sub_folder)
  {
    G_ASSERT(sub_folder);
    G_ASSERT(!sub_folder->parent || sub_folder->parent == this);
    sub_folder->parent = this;
    subFolders.push_back(sub_folder);
  }
};

struct DagorAssetMgr::LoadingPool : threadpool::JobPool
{
  Tab<LoadedFolder *> DAG_TS_GUARDED_BY(mutex) folders;

  LoadingPool(int num_workers) : JobPool(num_workers), folders(tmpmem) { folders.reserve(1024); }

  void addFolder(LoadedFolder *loaded)
  {
    OSSpinlockScopedLock guard(mutex);
    folders.push_back(loaded);
  }

  void finish(DagorAssetMgr &mgr)
  {
    OSSpinlockScopedLock guard(mutex);
    G_ASSERT(queue.empty());

    uint32_t numFolders = 1;
    for (int fi = folders.size() - 1; fi >= 0; fi--)
    {
      auto loaded = folders[fi];

      // remove empty folders and count assets
      for (int i = loaded->subFolders.size() - 1; i >= 0; i--)
      {
        LoadedFolder *sub = loaded->subFolders[i];
        if (sub->assets.empty() && sub->subFolders.empty())
        {
          post_msg(*mgr.msgPipe, mgr.msgPipe->REMARK, "remove empty subtree %s", sub->folder->folderPath);
          loaded->subFolders.erase_unsorted(loaded->subFolders.begin() + i);
          delete sub;
        }
        else
        {
          loaded->subAssetCount += sub->assets.size() + sub->subAssetCount;
          numFolders++;
        }
      }

      // sort folders by name
      eastl::sort(loaded->subFolders.begin(), loaded->subFolders.end(),
        [](const LoadedFolder *a, const LoadedFolder *b) { return asset_name_less(a->folder->folderName, b->folder->folderName); });
    }

    WriteGuard mgrGuard(mgr.mutex);
    mgr.folders.reserve(mgr.folders.size() + numFolders);
  }
};

int DagorAssetMgr::addAssetNameId(const char *nm)
{
  WriteGuard guard(mutex);
  auto prev_sz = assetNames.nameCount();
  int id = assetNames.addNameId(nm);
  if (id < prev_sz)
  {
    if (id >= tmpDupAssetNameMarks.size())
      tmpDupAssetNameMarks.resize(id + 1);
    tmpDupAssetNameMarks.set(id);
  }
  return id;
}
void DagorAssetMgr::addLoadedFolders(int parent_folder_idx, LoadedFolder *loaded)
{
  if (!loaded)
    return;
  WriteGuard guard(mutex);

  int fidx = folders.size();
  folders.push_back(loaded->folder);
  loaded->folder->parentIdx = parent_folder_idx;
  G_ASSERT(perFolderStartAssetIdx.size() == fidx);
  perFolderStartAssetIdx.push_back(assets.size());
  folders[parent_folder_idx]->subFolderIdx.push_back(fidx);

  dag::Span<DagorAssetPrivate *> loaded_assets = make_span((DagorAssetPrivate **)assets.data(), assets.size());
  for (auto &ca : loaded->assets)
    if (ca->getNameId() >= tmpDupAssetNameMarks.size() || !tmpDupAssetNameMarks.get(ca->getNameId()) ||
        checkAssetNotDuplicated(static_cast<DagorAssetPrivate *&>(ca), folders[fidx], folders[fidx]->folderPath,
          assetFileNames.getName(ca->getFileNameId()), ca->getType(), loaded_assets, nullptr))
    {
      ca->setFolder(fidx);
      assets.push_back(ca);
    }
    else if (ca)
      del_it(ca);

  for (auto sub : loaded->subFolders)
    addLoadedFolders(fidx, sub);

  // clear stuff because we moved assets and db folder, and deleted loaded sub-folders
  loaded->assets.clear();
  loaded->subFolders.clear();
  loaded->folder = nullptr;
  delete loaded;
}

bool DagorAssetMgr::loadAssetsBase(const char *assets_folder, const char *name_space)
{
  TIME_PROFILE(loadAssetsBase);
  struct InitThreadPoolOnDemand
  {
    bool cpujobsInitedLocally = false;
    bool threadpoolInitedLocally = false;
    InitThreadPoolOnDemand(bool shared_nm)
    {
      if ((threadpoolInitedLocally = !shared_nm && threadpool::get_num_workers() == 0) != false)
      {
        if ((cpujobsInitedLocally = !cpujobs::is_inited()) != false)
          cpujobs::init();
        threadpool::init(eastl::min(cpujobs::get_core_count(), 16), 1024, 128 << 10);
      }
    }
    ~InitThreadPoolOnDemand()
    {
      if (threadpoolInitedLocally)
        threadpool::shutdown();
      if (cpujobsInitedLocally)
        cpujobs::term(true);
    }
  } init_threadpool_if_needed(sharedAssetNameMapOwner != nullptr);

  int64_t startTime = profile_ref_ticks();
  int nsid;
  {
    WriteGuard guard(mutex);
    nsid = nspaceNames.addNameId(name_space);
  }
  size_t old_ptrs = dagor_memory_stat::get_memchunk_count(true);
  size_t old_mem = dagor_memory_stat::get_memory_allocated(true);

  String tmpPath;
  tmpPath = assets_folder;
  dd_simplify_fname_c(tmpPath);
  tmpPath.resize(strlen(tmpPath) + 1);
  if (tmpPath[tmpPath.length() - 1] == '/')
    erase_items(tmpPath, tmpPath.length() - 1, 1);

  assets_folder = NULL;
  {
    ReadGuard guard(mutex);
    for (int i = baseRoots.size() - 1; i >= 0; i--)
      if (dd_stricmp(baseRoots[i].folder, tmpPath) == 0)
      {
        if (baseRoots[i].nsId != nsid)
        {
          post_error(*msgPipe, "base already contains root <%s> marked as <%s>, not <%s>", baseRoots[i].folder.str(),
            nspaceNames.getName(baseRoots[i].nsId), name_space);
          return false;
        }
        assets_folder = baseRoots[i].folder;
        break;
      }
  }
  if (!assets_folder)
  {
    WriteGuard guard(mutex);
    RootEntryRec &r = baseRoots.push_back();
    r.folder = tmpPath;
    r.nsId = nsid;
    assets_folder = r.folder;
  }

  post_msg(*msgPipe, msgPipe->NOTE, "loading asset base from %s%s...", assets_folder,
    sharedAssetNameMapOwner ? " [using shared namemap]" : "");

  {
    WriteGuard guard(mutex);
    if (!folders.size())
    {
      DagorAssetFolder *rf = new DagorAssetFolder(-1, "Root", NULL);
      folders.push_back(rf);
      perFolderStartAssetIdx.push_back(0);
    }
  }

  int err_count;
  int s_assets;
  {
    ReadGuard guard(mutex);
    err_count = msgPipe->getErrorCount();
    s_assets = assets.size();
  }
  {
    TIME_PROFILE(loadAssets);
    LoadingPool jobPool(sharedAssetNameMapOwner ? 1 : 16);
    auto f = new (midmem) DagorAssetFolder(0, ::dd_get_fname(assets_folder), assets_folder);
    auto loaded = new (tmpmem) LoadedFolder(f, nullptr);
    loadAssets(jobPool, loaded, nsid);
    jobPool.stop();
    jobPool.finish(*this);

    WriteGuard guard(mutex);
    assets.reserve(assets.size() + loaded->assets.size() + loaded->subAssetCount);
    addLoadedFolders(0, loaded);
  }
  // tmpDupAssetNameMarks.clear(); // not needed after addLoadedFolders(), but loadAssetsBase() can be called several times

  // dump loaded asset / folder order
  if (0)
  {
    ReadGuard guard(mutex);
    DataBlock mainBlk;
    auto assetBlk = mainBlk.addBlock("assets");
    for (const auto a : assets)
      assetBlk->addStr("asset", a->getName());
    auto folderBlk = mainBlk.addBlock("folders");
    for (uint32_t i = 0, n = folders.size(); i < n; i++)
      folderBlk->addInt(folders[i]->folderPath, perFolderStartAssetIdx[i]);
    folderBlk->addInt("__end__", assets.size());
    mainBlk.saveToTextFile("assetOrderDump.blk");
  }

  updateGlobUniqueFlags();

  WriteGuard guard(mutex);
  bool loaded_ok = true;
  if (msgPipe->getErrorCount() > err_count)
    loaded_ok = false;

  tmpPath.printf(260, "%s/.tex-hier.blk", assets_folder);
  dd_simplify_fname_c(tmpPath);
  tmpPath.resize(strlen(tmpPath) + 1);

  if (texAssetType >= 0)
  {
    FastStrMap map;
    if (dgs_get_settings && dgs_get_settings())
    {
      const DataBlock &b = *dgs_get_settings()->getBlockByNameEx("texStreaming");
      int nid = b.getNameId("stubTex");
      for (int i = 0; i < b.blockCount(); i++)
        if (b.getBlock(i)->getBlockNameId() == nid)
          if (const char *tag = b.getBlock(i)->getStr("tag", NULL))
          {
            int idx = b.getBlock(i)->getInt("idx", -2);
            if (!b.getBlock(i)->getBool("alias", false))
              for (int j = 0; j < map.getMapRaw().size(); j++)
                if (map.getMapRaw()[j].id == idx)
                {
                  post_error(*msgPipe, "stubTex tag=%s uses the same idx=%d as tag=%s (but alias:b=yes is not specified!)", tag, idx,
                    map.getMapRaw()[j].name);
                  idx = -2;
                  break;
                }
            map.addStrId(tag, idx);
          }
    }

    for (int i = s_assets; i < assets.size(); i++)
      if (assets[i]->getType() == texAssetType)
        if (const char *tag = assets[i]->props.getStr("stubTexTag", NULL))
        {
          intptr_t id = map.getStrId(tag);
          if (id <= 0)
            post_error(*msgPipe, "asset %s uses invalid stubTexTag=%s (id=%d)", assets[i]->getName(), tag, id);
          else
            assets[i]->props.setInt("stubTexIdx", id);
        }
  }
  if (msgPipe->getErrorCount() > err_count)
    loaded_ok = false;

  if (texAssetType >= 0 && dd_file_exist(tmpPath))
  {
    DataBlock blk;
    int cnt_base = 0, cnt_derived = 0;
    if (!blk.load(tmpPath))
    {
      post_error(*msgPipe, "failed to read <%s>", tmpPath.str());
      return false;
    }
    int tex_nid = blk.getNameId("tex");
    int btt_nid = blk.getNameId("baseTexTolerance");
    const E3DCOLOR diff_tolerance_def = blk.getE3dcolor("baseTexTolerance", E3DCOLOR(0, 0, 0, 0));
    FastNameMap excl_4cc;
    for (int i = 0; i < blk.paramCount(); i++)
      if (blk.getParamType(i) == DataBlock::TYPE_BOOL && !blk.getBool(i))
        excl_4cc.addNameId(blk.getParamName(i));

    for (int i = 0; i < blk.blockCount(); i++)
    {
      DagorAsset *btex = findAsset(blk.getBlock(i)->getBlockName(), texAssetType);
      if (!btex)
      {
        post_msg(*msgPipe, msgPipe->WARNING, "base tex <%s> not found", blk.getBlock(i)->getBlockName());
        continue;
      }
      const DataBlock &b2 = *blk.getBlock(i);
      const E3DCOLOR diff_tolerance = b2.getE3dcolor("baseTexTolerance", diff_tolerance_def);
      for (int j = 0; j < b2.paramCount(); j++)
        if (b2.getParamNameId(j) == tex_nid && b2.getParamType(j) == DataBlock::TYPE_STRING)
        {
          DagorAsset *dtex = findAsset(b2.getStr(j), texAssetType);
          if (!dtex)
          {
            post_msg(*msgPipe, msgPipe->WARNING, "paired tex <%s> (for <%s>) not found", b2.getStr(j),
              blk.getBlock(i)->getBlockName());
            continue;
          }
          btex->props.addStr("pairedToTex", dtex->getName());
          dtex->props.setStr("baseTex", btex->getName());
          if (diff_tolerance.u)
            dtex->props.setE3dcolor("baseTexTolerance", diff_tolerance);
          iterate_names(excl_4cc, [&](int, const char *name) {
            btex->props.addBlock(name)->setStr("pairedToTex", "");
            dtex->props.addBlock(name)->setStr("baseTex", "");
          });
          btex->props.compact();
          dtex->props.compact();
          cnt_derived++;
        }
        else if (b2.getParamNameId(j) != btt_nid)
          post_msg(*msgPipe, msgPipe->WARNING, "unexpected %s in %s", b2.getParamName(j), tmpPath.str());
      cnt_base++;
    }
    post_msg(*msgPipe, msgPipe->NOTE, "setup %d base and %d derived tex assets (from %s)", cnt_base, cnt_derived, tmpPath.str());
  }

  for (int i = s_assets; i < assets.size(); i++)
    if (assets[i]->getType() == texAssetType && assets[i]->props.findParam("pairedToTex") >= 0)
    {
      int nid = assets[i]->props.getNameId("pairedToTex");
      for (int j = assets[i]->props.paramCount() - 1; j >= 0; j--)
        if (assets[i]->props.getParamNameId(j) == nid && assets[i]->props.getParamType(j) == DataBlock::TYPE_STRING)
        {
          DagorAsset *dtex = findAsset(assets[i]->props.getStr(j), texAssetType);
          if (!dtex)
            assets[i]->props.removeParam(j);
          else if (stricmp(dtex->props.getStr("baseTex", ""), assets[i]->getName()) != 0)
          {
            // logwarn("%s: remove pairedToTex link %s (since that references baseTex as <%s>)",
            //   assets[i]->getName(), dtex->getName(), dtex->props.getStr("baseTex", ""));
            assets[i]->props.removeParam(j);
          }
          assets[i]->props.compact();
        }
    }

  // process HQ (high quality) split
  int pre_hq_split_assets_count = assets.size();
  tmpPath.printf(260, "%s/.tex-hq-split.blk", assets_folder);
  dd_simplify_fname_c(tmpPath);
  tmpPath.resize(strlen(tmpPath) + 1);

  if (texAssetType >= 0 && dd_file_exist(tmpPath) && DataBlock(tmpPath).getInt("splitAtSize", 0) > 32)
  {
    DataBlock blk;
    if (!blk.load(tmpPath))
    {
      post_error(*msgPipe, "failed to read <%s>", tmpPath.str());
      return false;
    }

    const char *pkg = blk.getStr("hqPkg", NULL);
    const char *pkg_suff = blk.getStr("hqPkgSuffix", NULL);
    int splitAtSize = blk.getInt("splitAtSize", 2048);
    bool allow_genmip_for_split = blk.getBool("allowRtMipGenSplit", false);
    bool force_zlib_for_bq = blk.getBool("forceZlibForBQ", false);
    const DataBlock *platformsBlock = blk.getBlockByName("platforms");

    FastNameMap excl_tex;
    FastNameMap excl_pkg;
    int tex_nid = blk.getNameId("tex");
    int pkg_nid = blk.getNameId("pkg");
    if (const DataBlock *b = blk.getBlockByName("excludeSplit"))
      for (int i = 0; i < b->paramCount(); i++)
      {
        if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == tex_nid)
          excl_tex.addNameId(b->getStr(i));
        else if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == pkg_nid)
          excl_pkg.addNameId(b->getStr(i));
      }

    FastNameMap dont_sep_tex;
    FastNameMap dont_sep_pkg;
    if (const DataBlock *b = blk.getBlockByName("doNotSeparate"))
      for (int i = 0; i < b->paramCount(); i++)
      {
        if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == tex_nid)
          dont_sep_tex.addNameId(b->getStr(i));
        else if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == pkg_nid)
          dont_sep_pkg.addNameId(b->getStr(i));
      }

    int nsid = nspaceNames.addNameId("texHQ");
    int fidx = folders.size();
    folders.push_back(new DagorAssetFolder(-1, "", ""));
    perFolderStartAssetIdx.push_back(assets.size());
    int asset_cnt = assets.size();
    for (int i = 0; i < asset_cnt; i++)
      if (assets[i]->getType() == texAssetType)
      {
        if (!allow_genmip_for_split)
        {
          if (assets[i]->props.getStr("pairedToTex", NULL) || assets[i]->props.getStr("baseTex", NULL))
            continue;
          if (assets[i]->props.getBool("rtMipGen", false) || assets[i]->props.getBlockByNameEx("PC")->getBool("rtMipGen", false))
            continue;
        }
        if (excl_tex.getNameId(assets[i]->getName()) >= 0)
          continue;
        const char *asset_pkg = assets[i]->getCustomPackageName("PC", NULL);
        if (asset_pkg)
          if (excl_pkg.getNameId(asset_pkg) >= 0)
          {
            // debug("skip split %s, pkg=%s", assets[i]->getName(), asset_pkg);
            continue;
          }

        int id = addAssetNameId(String(0, "%s$hq", assets[i]->getName()));
        if (id < 0)
        {
          logerr("can't split %s", assets[i]->getName());
          continue;
        }
        bool split_override = assets[i]->props.getBool("splitAtOverride", false);
        int final_split = splitAtSize;

        if (split_override)
          final_split = assets[i]->props.getInt("splitAt", 0);
        else
          final_split = getFolder(assets[i]->getFolderIndex()).exportProps.getInt("splitAt", final_split);

        // Save platforms parameters blocks from .tex-hq-split.blk and saving to asset as is
        if (platformsBlock)
          for (int block_index = 0; block_index < platformsBlock->blockCount(); block_index++)
          {
            const char *name = platformsBlock->getBlock(block_index)->getBlockName();
            int value =
              platformsBlock->getBlockByNameEx(name)->getInt("splitAtSize", assets[i]->props.getInt("splitAtSize", splitAtSize));
            assets[i]->props.addBlock(name)->setInt("splitAtSize", value);
          }

        if (final_split == 0)
        {
          if (assets[i]->props.getInt("splitAt", 0) != 0)
            assets[i]->props.setInt("splitAt", final_split);
          assets[i]->props.compact();
          continue;
        }

        DagorAssetPrivate *ca = new DagorAssetPrivate(*assets[i], id, nsid);
        if (split_override)
          ca->props.setInt("splitAt", final_split);
        else
        {
          assets[i]->props.setInt("splitAt", final_split);
          ca->props.setInt("splitAt", final_split);
        }

        if (platformsBlock)
          for (int block_index = 0; block_index < platformsBlock->blockCount(); block_index++)
          {
            const char *name = platformsBlock->getBlock(block_index)->getBlockName();
            int value = platformsBlock->getBlockByNameEx(name)->getInt("splitAtSize", ca->props.getInt("splitAtSize", splitAtSize));
            ca->props.addBlock(name)->setInt("splitAtSize", value);
          }

        assets[i]->props.setInt("splitVer", allow_genmip_for_split ? 2 : 1); //== force single rebuild of bq parts of split tex
        if (force_zlib_for_bq)
          assets[i]->props.setBool("forceZlib", true);
        assets[i]->props.compact();

        ca->props.setBool("splitHigh", true);
        ca->props.removeBlock("package");
        ca->props.removeParam("package");
        ca->props.removeParam("rtMipGenBQ");

        if (dont_sep_tex.getNameId(assets[i]->getName()) >= 0 || (asset_pkg && dont_sep_pkg.getNameId(asset_pkg) >= 0) ||
            get_split_not_sep_for_asset(assets[i]))
        {
          ca->props.setStr("forcePackage", asset_pkg ? asset_pkg : "*");
          if (String pk = assets[i]->getDestPackName(false, true, false); !pk.empty())
            ca->props.setStr("ddsxTexPack", String(0, "%s-hq", pk.str()));
        }
        else if ((asset_pkg && strcmp(asset_pkg, basePkg) != 0) && pkg_suff)
          ca->props.setStr("forcePackage", String(0, "%s%s", asset_pkg, pkg_suff));
        else if (pkg)
          ca->props.setStr("forcePackage", pkg);
        else
        {
          ca->props.removeParam("forcePackage");
          if (String pk = assets[i]->getDestPackName(false, true, false); !pk.empty())
            ca->props.setStr("ddsxTexPack", String(0, "%s-hq", pk.str()));
        }
        ca->props.compact();

        assets.push_back(ca);
      }
    post_msg(*msgPipe, msgPipe->NOTE, "added %d hidden HQ tex assets, split at %d (from %s)", assets.size() - asset_cnt, splitAtSize,
      tmpPath.str());
  }

  // process TQ (thumbnail quality) tex packs
  tmpPath.printf(260, "%s/.tex-tq-split.blk", assets_folder);
  dd_simplify_fname_c(tmpPath);
  tmpPath.resize(strlen(tmpPath) + 1);

  if (texAssetType >= 0 && dd_file_exist(tmpPath) && DataBlock(tmpPath).getInt("maxTexDataSize", 4096))
  {
    DataBlock blk;
    if (!blk.load(tmpPath))
    {
      post_error(*msgPipe, "failed to read <%s>", tmpPath.str());
      return false;
    }
    int maxDataSize = blk.getInt("maxTexDataSize", 2048);
    const char *pack_nm = blk.getStr("thumbnailPackName", "_tq_pack");
    int tqBqStrategy = 0;
    if (const char *force_tq = blk.getStr("forceThumbnails", nullptr))
    {
      if (strcmp(force_tq, "no") == 0)
        tqBqStrategy = 0;
      else if (strcmp(force_tq, "use_base") == 0)
        tqBqStrategy = 1;
      else if (strcmp(force_tq, "downsample_base") == 0)
        tqBqStrategy = -1;
      else
        logerr("unknown forceThumbnails:t='%s', ignored; using 'no'", force_tq);
    }

    int nsid = nspaceNames.addNameId("texTQ");
    int fidx = folders.size();
    folders.push_back(new DagorAssetFolder(-1, "", ""));
    perFolderStartAssetIdx.push_back(assets.size());
    int asset_cnt = assets.size();
    for (int i = s_assets; i < pre_hq_split_assets_count; i++)
      if (assets[i]->getType() == texAssetType && !assets[i]->props.getBool("splitHigh", false))
      {
        int id = addAssetNameId(String(0, "%s$tq", assets[i]->getName()));
        if (id < 0)
        {
          logerr("can't split %s", assets[i]->getName());
          continue;
        }

        DagorAssetPrivate *ca = new DagorAssetPrivate(*assets[i], id, nsid);
        ca->props.setBool("convert", true);
        ca->props.setInt("thumbnailMaxDataSize", maxDataSize);
        ca->props.setStr("ddsxTexPack", pack_nm);
        ca->props.setInt("stubTexIdx", -1);
        ca->props.setInt("hqMip", 0);
        ca->props.setInt("mqMip", 0);
        ca->props.setInt("lqMip", 0);
        if (tqBqStrategy)
          ca->props.setInt("forceTQ", tqBqStrategy);

        ca->props.removeParam("pairedToTex");
        ca->props.removeParam("baseTex");
        ca->props.removeParam("rtMipGen");
        ca->props.removeParam("rtMipGenBQ");
        ca->props.compact();

        assets.push_back(ca);
      }
    post_msg(*msgPipe, msgPipe->NOTE, "added %d hidden TQ tex assets, data size <= %dK (from %s)", assets.size() - asset_cnt,
      maxDataSize >> 10, tmpPath.str());
  }

  if (texAssetType >= 0 && !texRtMipGenAllowed)
  {
    int err_cnt = 0;
    for (int i = s_assets; i < assets.size(); i++)
      if (assets[i]->getType() == texAssetType &&
          (assets[i]->props.getBool("rtMipGen", false) || assets[i]->props.getBool("rtMipGenBQ", false)))
      {
        post_msg(*msgPipe, msgPipe->ERROR, "tex <%s> is rtMipGen, prohibited", assets[i]->getName());
        err_cnt++;
      }
    if (err_cnt)
      loaded_ok = false;
  }

  size_t new_ptrs = dagor_memory_stat::get_memchunk_count(true);
  size_t new_mem = dagor_memory_stat::get_memory_allocated(true);
  post_msg(*msgPipe, loaded_ok ? msgPipe->NOTE : msgPipe->ERROR,
    "loaded with%s errors, %d assets  [allocated %dM in %dK chunks] in %.3fs", loaded_ok ? "out" : "", assets.size() - s_assets,
    (new_mem - old_mem + (1 << 19)) >> 20, (new_ptrs - old_ptrs + (1 << 9)) >> 10, ((float)profile_time_usec(startTime)) / 1e6f);
  return loaded_ok;
}

bool DagorAssetMgr::startMountAssetsDirect(const char *folder_name, int parent_folder_idx)
{
  WriteGuard guard(mutex);
  if (!folders.size())
  {
    parent_folder_idx = folders.size();
    folders.push_back(new DagorAssetFolder(-1, "Root", NULL));
    perFolderStartAssetIdx.push_back(0);
  }
  if (parent_folder_idx < 0 || parent_folder_idx >= folders.size())
    return false;

  int fidx = folders.size();
  folders.push_back(new DagorAssetFolder(parent_folder_idx, folder_name, "::"));
  folders[parent_folder_idx]->subFolderIdx.push_back(fidx);
  perFolderStartAssetIdx.push_back(assets.size());
  return true;
}

void DagorAssetMgr::updateGlobUniqueFlags()
{
  WriteGuard guard(mutex);
  Tab<int> tempTab(tmpmem);
  tempTab.resize(assetNames.nameCount());
  mem_set_0(tempTab);
  for (int i = 0; i < assets.size(); i++)
    tempTab[assets[i]->getNameId()]++;

  for (int i = 0; i < assets.size(); i++)
    reinterpret_cast<DagorAssetPrivate *>(assets[i])->setGlobUnique(tempTab[assets[i]->getNameId()] == 1);
}

DagorAsset *DagorAssetMgr::loadAssetDirect(const char *folder, const char *file_name)
{
  WriteGuard guard(mutex);
  DagorAssetFolder *f = new DagorAssetFolder(-1, ::dd_get_fname(folder), folder);
  DagorAssetPrivate *ca = NULL;
  DataBlock folders_blk;
  int start_rule_idx = 0;

  // read and apply .folder.blk
  String tmpPath;
  tmpPath.printf(260, "%s/.folder.blk", folder);
  {
    FullFileLoadCB crd(tmpPath, DF_READ | DF_IGNORE_MISSING);
    if (crd.fileHandle)
    {
      if (folders_blk.loadFromStream(crd, tmpPath, crd.getTargetDataSize()))
        readFoldersBlk(*f, nullptr, folders_blk);
      else
        post_error(*msgPipe, "error loading %s", tmpPath.str());
    }
  }

  if (addAsset(folder, file_name, -1, ca, f, nullptr, start_rule_idx, false))
  {
    delete f;
    return ca;
  }

  delete f;
  del_it(ca);
  return NULL;
}

DagorAsset *DagorAssetMgr::makeAssetDirect(const char *asset_name, const DataBlock &props, int atype)
{
  WriteGuard guard(mutex);
  int fidx = folders.size() - 1;
  if (!folders.size())
  {
    fidx = folders.size();
    folders.push_back(new DagorAssetFolder(-1, "", ""));
    perFolderStartAssetIdx.push_back(assets.size());
  }

  DagorAssetPrivate *ca = new DagorAssetPrivate(*this);
  ca->setNames(addAssetNameId(asset_name), -1, true);
  ca->setAssetData(fidx, assetFileNames.addNameId(asset_name), atype);
  ca->props = props;
  assets.push_back(ca);
  if (atype >= 0 && atype < perTypeNameIds.size())
    perTypeNameIds[atype].addInt(ca->getNameId());
  return ca;
}

void DagorAssetMgr::loadAssets(LoadingPool &job_pool, LoadedFolder *loaded, int nspace_id)
{
  job_pool.addFolder(loaded);
  const char *folder_path = loaded->folder->folderPath;
  auto f = loaded->folder;

  // read and apply .folder.blk
  String tmpPath;
  tmpPath.printf(260, "%s/.folder.blk", folder_path);
  {
    FullFileLoadCB crd(tmpPath, DF_READ | DF_IGNORE_MISSING);
    if (crd.fileHandle)
    {
      DataBlock folders_blk;
      if (folders_blk.loadFromStream(crd, tmpPath, crd.getTargetDataSize()))
        readFoldersBlk(*f, loaded->parent ? loaded->parent->folder : nullptr, folders_blk);
      else
        post_error(*msgPipe, "error loading %s", tmpPath.str());
    }
  }

  // scan folder once, building lists, to avoid holding finder resources
  Tab<SimpleString> fileNames(tmpmem);

  if ((f->flags & f->FLG_SCAN_FOLDERS) == 0)
    post_msg(*msgPipe, msgPipe->REMARK, "skip scanning folders in %s", folder_path);
  if ((f->flags & f->FLG_SCAN_ASSETS) == 0)
    post_msg(*msgPipe, msgPipe->REMARK, "skip scanning assets in %s", folder_path);

  if (f->flags & (f->FLG_SCAN_FOLDERS | f->FLG_SCAN_ASSETS))
  {
    tmpPath.printf(260, "%s/*", folder_path);
    for (const alefind_t &ff : dd_find_iterator(tmpPath, DA_FILE | DA_SUBDIR))
    {
      if (ff.attr & DA_SUBDIR)
      {
        if ((f->flags & f->FLG_SCAN_FOLDERS) == 0)
          continue;
        if (dd_stricmp(ff.name, "cvs") == 0 || dd_stricmp(ff.name, ".svn") == 0)
          continue;

        auto path = String(260, "%s/%s", folder_path, ff.name);
        auto sf = new (midmem) DagorAssetFolder(-1, ff.name, path);
        auto sub = new (tmpmem) LoadedFolder(sf, loaded);
        loaded->addSub(sub);
      }
      else
      {
        if ((f->flags & f->FLG_SCAN_ASSETS) == 0)
          continue;
        if (ff.name[0] == '.')
          continue;
        fileNames.emplace_back(ff.name);
      }
    }
  }

  eastl::sort(fileNames.begin(), fileNames.end(), [](const auto &a, const auto &b) { return asset_name_less(a, b); });

  if (f->flags & f->FLG_SCAN_ASSETS)
  {
    // scan for assets (*.res.bljk, *.blk, *.*)
    DagorAssetPrivate *ca = nullptr;
    tmpPath.printf(260, "%s/*", folder_path);
    for (const auto &fileName : fileNames)
    {
      LoadedFolder *va_f = loaded;

    check_virtual_asset_rules:
      int s_rule_idx = 0;
      do
        while (addAsset(folder_path, fileName.c_str(), nspace_id, ca, va_f->folder, loaded, s_rule_idx, true))
        {
          loaded->assets.push_back(ca);
          ca = NULL;
          if (s_rule_idx == -1)
            goto end_check_rules;
        }
      while (s_rule_idx > 0);

      if (s_rule_idx == -2)
        goto end_check_rules;

      while ((va_f->folder->flags & DagorAssetFolder::FLG_INHERIT_RULES) && va_f->parent != nullptr)
      {
        va_f = va_f->parent;
        if (va_f->folder->vaRuleCount)
          goto check_virtual_asset_rules;
      }

    end_check_rules:;
    }
    if (ca)
      delete ca;
  }

  // start sub-folder jobs
  for (uint32_t i = 0, n = loaded->subFolders.size(); i < n; i++)
  {
    auto sub = loaded->subFolders[i];
    job_pool.add([this, sub, nspace_id, &job_pool] { loadAssets(job_pool, sub, nspace_id); });
  }
}

bool DagorAssetMgr::addAsset(const char *folder_path, const char *fname, int nspace_id, DagorAssetPrivate *&ca, DagorAssetFolder *f,
  LoadedFolder *loaded, int &start_rule_idx, bool reg)
{
  bool asset_load_success = false;
  int asset_name_id = -1, asset_type = -1;

  String tmpPath;
  if (trail_stricmp(fname, ".res.blk"))
  {
    start_rule_idx = -1;
    tmpPath = fname;
    erase_items(tmpPath, tmpPath.length() - 8, 8);
    asset_name_id = addAssetNameId(tmpPath);

    if (!ca)
      ca = new DagorAssetPrivate(*this);
    tmpPath.printf(260, "%s/%s", folder_path, fname);
    asset_load_success = ca->loadResBlk(asset_name_id, tmpPath, nspace_id);

    if (asset_load_success)
    {
      ReadGuard guard(mutex);
      const char *type_name = ca->props.getStr("className", NULL);
      asset_type = type_name ? typeNames.getNameId(type_name) : -1;

      if (asset_type < 0)
        post_error_f(*msgPipe, folder_path, fname, "unsupported class <%s> in asset %s::%s", type_name, ca->getNameSpace(),
          ca->getName());
    }
    else
      post_error_f(*msgPipe, folder_path, fname, "failed to load BLK <%s>", tmpPath);
  }
  else if (trail_stricmp(fname, ".blk"))
  {
    const char *p = fname + strlen(fname) - 5;
    while (p > fname && *p != '.')
      p--;
    if (*p != '.')
      goto process_va_rules;

    start_rule_idx = -1;
    tmpPath = fname;
    erase_items(tmpPath, p - fname, fname + tmpPath.length() - p);
    asset_name_id = addAssetNameId(tmpPath);

    if (!ca)
      ca = new DagorAssetPrivate(*this);
    tmpPath.printf(260, "%s/%s", folder_path, fname);
    asset_load_success = ca->loadResBlk(asset_name_id, tmpPath, nspace_id);

    if (asset_load_success)
    {
      ReadGuard guard(mutex);
      tmpPath = p + 1;
      erase_items(tmpPath, tmpPath.length() - 4, 4);
      asset_type = typeNames.getNameId(tmpPath);

      if (asset_type < 0)
        post_error_f(*msgPipe, folder_path, fname, "unsupported class <%s> in asset %s::%s", tmpPath.str(), ca->getNameSpace(),
          ca->getName());
    }
    else
      post_error_f(*msgPipe, folder_path, fname, "failed to load BLK <%s>", tmpPath);
  }
  else
  process_va_rules:
    for (int i = start_rule_idx; i < f->vaRuleCount; i++)
    {
      DagorVirtualAssetRule *rule;
      {
        ReadGuard guard(mutex);
        rule = vaRule[f->startVaRuleIdx + i];
      }
      OSSpinlockScopedLock ruleGuard(rule->mutex);
      if (rule->testRule(fname, folder_path, tmpPath))
      {
        asset_name_id = addAssetNameId(tmpPath);

        if (!ca)
          ca = new DagorAssetPrivate(*this);

        if (rule->shouldAddSrcBlk())
        {
          tmpPath.printf(260, "%s/%s", folder_path, fname);
          asset_load_success = ca->loadResBlk(asset_name_id, tmpPath, nspace_id);
        }
        else
        {
          ca->props.clearData();
          asset_load_success = true;
        }

        if (asset_load_success)
        {
          ca->setNames(asset_name_id, nspace_id, true, i);
          rule->applyRule(ca->props, String(0, "%s/%s", folder_path, fname));
          ca->props.compact();

          const char *type_name = ca->props.getStr("className", NULL);
          const char *type_name2 = rule->getTypeName();
          if (type_name && type_name2)
          {
            if (stricmp(type_name, type_name2) != 0)
            {
              post_error(*msgPipe,
                "conflicting types: <%s> (in %s/%s) and"
                " <%s> in virtual asset rule in %s/.folder.blk",
                type_name, folder_path, fname, type_name2, folder_path);
              type_name = NULL;
            }
          }
          else if (type_name2)
            type_name = type_name2;

          ReadGuard guard(mutex);
          asset_type = type_name ? typeNames.getNameId(type_name) : -1;

          if (asset_type < 0)
            post_error_f(*msgPipe, folder_path, fname, "unsupported class <%s> in asset %s::%s", type_name, ca->getNameSpace(),
              ca->getName());
        }

        start_rule_idx = rule->shouldStopProcessing() ? -1 : i + 1;
        break;
      }
    }

  if (asset_name_id < 0)
    start_rule_idx = -1;

  if (asset_load_success && asset_type >= 0)
  {
    if (!reg)
    {
      WriteGuard guard(mutex);
      ca->setAssetData(-1, assetFileNames.addNameId(fname), asset_type);
      return true;
    }

    bool added;
    {
      WriteGuard guard(mutex);
      added = perTypeNameIds[asset_type].addInt(asset_name_id);
    }
    if (added)
    {
      WriteGuard guard(mutex);
      ca->setAssetData(-1, assetFileNames.addNameId(fname), asset_type);
      return true;
    }
    G_ASSERT(loaded);
    return checkAssetNotDuplicated(ca, f, folder_path, fname, asset_type, make_span(loaded->assets), &start_rule_idx);
  }
  return false;
}
bool DagorAssetMgr::checkAssetNotDuplicated(DagorAssetPrivate *&ca, DagorAssetFolder *f, const char *folder_path, const char *fname,
  int asset_type, dag::Span<DagorAssetPrivate *> loaded_assets, int *start_rule_idx)
{
  int asset_name_id = ca->getNameId();
  int pa_idx = -1;
  for (int i = loaded_assets.size() - 1; i >= 0; i--)
    if (loaded_assets[i]->getNameId() == asset_name_id && loaded_assets[i]->getType() == asset_type)
    {
      pa_idx = i;
      break;
    }

  if (pa_idx == -1)
  {
    WriteGuard guard(mutex);
    ca->setAssetData(-1, assetFileNames.addNameId(fname), asset_type);
    return true;
  }

  if (!loaded_assets[pa_idx]->isVirtual() && ca->isVirtual())
  {
    if (!start_rule_idx)
      del_it(ca);
    else if (*start_rule_idx == -1)
      *start_rule_idx = -2;
    return false;
  }

  if (loaded_assets[pa_idx]->isVirtual() && !ca->isVirtual())
  {
    DagorAssetPrivate *tmp = loaded_assets[pa_idx];
    {
      WriteGuard guard(mutex);
      ca->setAssetData(-1, assetFileNames.addNameId(fname), asset_type);
    }
    /*
    SimpleString tgt1(ca->getTargetFilePath());
    SimpleString tgt2(tmp->getTargetFilePath());
    if (stricmp(tgt1, tgt2) != 0)
      post_msg_a(*msgPipe, msgPipe->WARNING, ca,
        "explicit asset  %s of type <%s> doesn't point to the same target as virtual asset\n"
        "<%s> != <%s>",
        assetNames.getName(asset_name_id), typeNames.getName(asset_type),
        tgt1.str(), tgt2.str());
    */

    loaded_assets[pa_idx] = ca;
    ca = tmp;
    if (!start_rule_idx)
      del_it(ca);
    else if (*start_rule_idx == -1)
      *start_rule_idx = -2;
    return false;
  }
  unsigned rule_idx = ca->getRuleIdx();
  bool ignore_duplicate_asset = false, override_duplicate_asset = false;
  if (ca->isVirtual() && rule_idx < f->vaRuleCount)
  {
    ReadGuard guard(mutex);
    ignore_duplicate_asset = vaRule[f->startVaRuleIdx + rule_idx]->shouldIgnoreDuplicateAsset();
    override_duplicate_asset = vaRule[f->startVaRuleIdx + rule_idx]->shouldOverrideDuplicateAsset();
  }
  if (loaded_assets[pa_idx]->isVirtual() && ca->isVirtual() && ignore_duplicate_asset)
    return false;
  if (loaded_assets[pa_idx]->isVirtual() && ca->isVirtual() && override_duplicate_asset)
  {
    DagorAssetPrivate *tmp = loaded_assets[pa_idx];
    {
      WriteGuard guard(mutex);
      ca->setAssetData(-1, assetFileNames.addNameId(fname), asset_type);
    }

    loaded_assets[pa_idx] = ca;
    ca = tmp;
    if (!start_rule_idx)
      del_it(ca);
    else if (*start_rule_idx == -1)
      *start_rule_idx = -2;
    return false;
  }

  ReadGuard guard(mutex);
  post_error_f(*msgPipe, folder_path, fname, "duplicate %s asset %s of type <%s>, existing is %s asset from %s/%s",
    ca->isVirtual() ? "virtual" : "BLK", assetNames.getName(asset_name_id), typeNames.getName(asset_type),
    loaded_assets[pa_idx]->isVirtual() ? "virtual" : "BLK", folder_path, loaded_assets[pa_idx]->getSrcFileName());
  return false;
}

void DagorAssetMgr::readFoldersBlk(DagorAssetFolder &f, DagorAssetFolder *parent, const DataBlock &blk)
{
  if (blk.getBool("inherit_flags", false))
    if (parent)
      f.flags = parent->flags;

  if (blk.getBool("exported", true))
    f.flags |= f.FLG_EXPORT_ASSETS;
  else
    f.flags &= ~f.FLG_EXPORT_ASSETS;

  if (blk.getBool("scan_assets", true))
    f.flags |= f.FLG_SCAN_ASSETS;
  else
    f.flags &= ~f.FLG_SCAN_ASSETS;

  if (blk.getBool("scan_folders", true))
    f.flags |= f.FLG_SCAN_FOLDERS;
  else
    f.flags &= ~f.FLG_SCAN_FOLDERS;

  if (blk.getBool("inherit_rules", true))
    f.flags |= f.FLG_INHERIT_RULES;
  else
    f.flags &= ~f.FLG_INHERIT_RULES;

  f.exportProps.setFrom(blk.getBlockByNameEx("export"));
  f.exportProps.compact();

  int vrb_nid = blk.getNameId("virtual_res_blk");
  Tab<DagorVirtualAssetRule *> rules(tmpmem);

  for (int i = 0; i < blk.blockCount(); i++)
    if (blk.getBlock(i)->getBlockNameId() == vrb_nid)
    {
      DagorVirtualAssetRule *vr = new (midmem) DagorVirtualAssetRule(getSharedNm());
      if (vr->load(*blk.getBlock(i), *msgPipe))
      {
        rules.push_back(vr);
      }
      else
      {
        delete vr;
        post_error(*msgPipe, "bad VA rule in block %d of %s/.folder.blk", i + 1, f.folderPath.str());
      }
    }

  if (!rules.empty())
  {
    WriteGuard guard(mutex);
    f.startVaRuleIdx = vaRule.size();
    f.vaRuleCount = rules.size();
    vaRule.insert(vaRule.end(), rules.cbegin(), rules.cend());
  }
}

static void save_asset_props_stripped(const DataBlock &blk, IGenSave &cwr, bool is_tex_asset)
{
  if (is_tex_asset &&
      (blk.paramExists("splitVer") || blk.paramExists("splitAt") || blk.paramExists("stubTexTag") || blk.paramExists("splitAtSize")))
  {
    DataBlock b;
    b.setFrom(&blk);
    b.removeParam("splitVer");
    b.removeParam("forceZlib");
    if (blk.paramExists("stubTexTag"))
      b.removeParam("stubTexIdx");
    if (!b.getBool("splitAtOverride", false))
      b.removeParam("splitAt");
    for (int bi = b.blockCount() - 1; bi >= 0; bi--)
    {
      DataBlock &b2 = *b.getBlock(bi);
      if (!b2.getBool("splitAtOverride", false))
        b2.removeParam("splitAt");
      b2.removeParam("splitAtSize");
      if (!b2.paramCount())
        b.removeBlock(bi);
    }
    b.saveToTextStream(cwr);
  }
  else
    blk.saveToTextStream(cwr);
}

bool DagorAssetMgr::saveAssetsMetadata(int folder_idx)
{
  ReadGuard guard(mutex);
  if (folder_idx < -1 || folder_idx >= (int)folders.size())
  {
    post_error(*msgPipe, "invalid folder idx=%d in saveAssetsMetadata", folder_idx);
    return false;
  }

  int start_idx = 0, end_idx = assets.size();
  if (folder_idx >= 0) //-V1051
    getFolderAssetIdxRange(folder_idx, start_idx, end_idx);
  return saveAssetsMetadataImpl(start_idx, end_idx);
}
bool DagorAssetMgr::saveAssetMetadata(DagorAsset &a)
{
  if (a.isVirtual())
    return false;

  ReadGuard guard(mutex);
  int f_idx0 = -1, f_idx1 = -1, idx = -1;
  getFolderAssetIdxRange(a.getFolderIndex(), f_idx0, f_idx1);
  if (f_idx0 >= 0 && f_idx0 < f_idx1)
    idx = find_value_idx(make_span_const(assets).subspan(f_idx0, f_idx1 - f_idx0), &a);
  if (idx < 0)
  {
    post_error(*msgPipe, "invalid asset %s (folder idx %d), not found in base", a.getNameTypified(), a.getFolderIndex());
    return false;
  }
  idx += f_idx0;
  return saveAssetsMetadataImpl(idx, idx + 1);
}
bool DagorAssetMgr::saveAssetsMetadataImpl(int start_idx, int end_idx)
{
  ReadGuard guard(mutex);
  DynamicMemGeneralSaveCB cwr_new(tmpmem, 0, 4 << 10), cwr_old(tmpmem, 0, 4 << 10);
  DataBlock blk;

  int nsid1 = nspaceNames.getNameId("texHQ"), nsid2 = nspaceNames.addNameId("texTQ");
  for (int i = start_idx; i < end_idx; i++)
    if (!assets[i]->isVirtual())
    {
      if ((nsid1 >= 0 && assets[i]->getNameSpaceId() == nsid1) || (nsid2 >= 0 && assets[i]->getNameSpaceId() == nsid2))
        continue;

      cwr_new.seekto(0);
      cwr_new.resize(0);
      cwr_old.seekto(0);
      cwr_old.resize(0);

      bool is_tex_asset = assets[i]->getType() == texAssetType;
      if (DataBlock *src_blk = assets[i]->props.getBlockByName("__src"))
      {
        blk.parseIncludesAsParams = true;
        blk.parseCommentsAsParams = true;
        blk.load(assets[i]->getSrcFilePath());
        blk.parseIncludesAsParams = false;
        save_asset_props_stripped(*src_blk, cwr_new, is_tex_asset);
      }
      else
      {
        blk.load(assets[i]->getSrcFilePath());
        save_asset_props_stripped(assets[i]->props, cwr_new, is_tex_asset);
      }

      blk.saveToTextStream(cwr_old);
      blk.parseCommentsAsParams = false;

      if (cwr_new.size() != cwr_old.size() || memcmp(cwr_new.data(), cwr_old.data(), cwr_new.size()) != 0)
      {
        post_msg(*msgPipe, msgPipe->NOTE, "writing changed asset <%s>", assets[i]->getName());
        FullFileSaveCB fcwr(assets[i]->getSrcFilePath());
        fcwr.write(cwr_new.data(), cwr_new.size());
      }
    }
    else
    {
      //== handle saving of virtual assets properly
    }

  return true;
}

static bool check_asset_changed(const DagorAsset &a, DynamicMemGeneralSaveCB &cwr_new, DynamicMemGeneralSaveCB &cwr_old,
  int texAssetType, int nsid)
{
  DataBlock blk;
  if (a.isVirtual())
  {
    //== handle saving of virtual assets properly
    return false;
  }
  if (nsid >= 0 && a.getNameSpaceId() == nsid)
    return false;

  cwr_new.seekto(0);
  cwr_new.resize(0);
  cwr_old.seekto(0);
  cwr_old.resize(0);

  bool is_tex_asset = a.getType() == texAssetType;
  if (const DataBlock *src_blk = a.props.getBlockByName("__src"))
  {
    blk.parseIncludesAsParams = true;
    blk.parseCommentsAsParams = true;
    blk.load(a.getSrcFilePath());
    blk.parseIncludesAsParams = false;
    save_asset_props_stripped(*src_blk, cwr_new, is_tex_asset);
  }
  else
  {
    blk.load(a.getSrcFilePath());
    save_asset_props_stripped(a.props, cwr_new, is_tex_asset);
  }

  blk.saveToTextStream(cwr_old);
  blk.parseCommentsAsParams = false;

  if (cwr_new.size() != cwr_old.size() || memcmp(cwr_new.data(), cwr_old.data(), cwr_new.size()) != 0)
    return true;
  return false;
}

bool DagorAssetMgr::isAssetMetadataChanged(const DagorAsset &a)
{
  ReadGuard guard(mutex);
  DynamicMemGeneralSaveCB cwr_new(tmpmem, 0, 4 << 10), cwr_old(tmpmem, 0, 4 << 10);
  return check_asset_changed(a, cwr_new, cwr_old, texAssetType, nspaceNames.getNameId("texHQ"));
}
bool DagorAsset::isMetadataChanged() const { return mgr.isAssetMetadataChanged(*this); }

bool DagorAssetMgr::isAssetsMetadataChanged()
{
  ReadGuard guard(mutex);
  DynamicMemGeneralSaveCB cwr_new(tmpmem, 0, 4 << 10), cwr_old(tmpmem, 0, 4 << 10);
  DataBlock blk;

  int nsid = nspaceNames.getNameId("texHQ");
  for (int i = 0; i < assets.size(); i++)
    if (check_asset_changed(*assets[i], cwr_new, cwr_old, texAssetType, nsid))
      return true;
  return false;
}

int DagorAssetMgr::getFolderIndex(const DagorAssetFolder &folder) const
{
  ReadGuard guard(mutex);
  for (int i = 0; i < folders.size(); ++i)
    if (folders[i] == &folder)
      return i;
  return -1;
}

Tab<int> DagorAssetMgr::getFilteredFolders(dag::ConstSpan<int> types)
{
  ReadGuard guard(mutex);
  FastIntList tempList;
  for (int i = 0; i < assets.size(); i++)
  {
    int t = assets[i]->getType();
    for (int j = 0; j < types.size(); j++)
      if (types[j] == t)
      {
        tempList.addInt(assets[i]->getFolderIndex());
        break;
      }
  }

  bool more_iter;
  do
  {
    more_iter = false;

    for (int i = tempList.getList().size() - 1; i >= 0; i--)
      if (folders[tempList.getList()[i]]->parentIdx != -1)
        more_iter |= tempList.addInt(folders[tempList.getList()[i]]->parentIdx);
  } while (more_iter);

  return Tab<int>(tempList.getList());
}

Tab<int> DagorAssetMgr::getFilteredAssets(dag::ConstSpan<int> types, int folder_idx) const
{
  ReadGuard guard(mutex);
  Tab<int> tempTab(tmpmem);
  if (types.size() == 0)
    return tempTab;
  if (folder_idx >= 0 && folder_idx >= folders.size())
    return tempTab;

  if (folder_idx == -1)
  {
    if (types.size() == 1)
    {
      int t = types[0];
      ;
      for (int i = 0; i < assets.size(); i++)
        if (assets[i]->getType() == t)
          tempTab.push_back(i);
    }
    else
      for (int i = 0; i < assets.size(); i++)
      {
        int t = assets[i]->getType();
        for (int j = 0; j < types.size(); j++)
          if (types[j] == t)
          {
            tempTab.push_back(i);
            break;
          }
      }
  }
  else
  {
    int s_idx, e_idx;
    getFolderAssetIdxRange(folder_idx, s_idx, e_idx);

    if (types.size() == 1)
    {
      int t = types[0];
      ;
      for (int i = s_idx; i < e_idx; i++)
        if (assets[i]->getType() == t)
          tempTab.push_back(i);
    }
    else
      for (int i = s_idx; i < e_idx; i++)
        if (assets[i]->getFolderIndex() == folder_idx)
        {
          int t = assets[i]->getType();
          for (int j = 0; j < types.size(); j++)
            if (types[j] == t)
            {
              tempTab.push_back(i);
              break;
            }
        }
  }
  return tempTab;
}

bool DagorAssetMgr::registerAssetExporter(IDagorAssetExporter *exp)
{
  WriteGuard guard(mutex);
  int type = typeNames.getNameId(exp->getAssetType());
  if (type == -1)
  {
    post_error(*msgPipe, "exporter <%s> cannot be registered for non-allowed asset type: %s", exp->getExporterIdStr(),
      exp->getAssetType());
    return false;
  }
  while (type >= perTypeExp.size())
    perTypeExp.push_back(nullptr);
  if (perTypeExp[type])
  {
    post_error(*msgPipe,
      "exporter <%s> cannot be registered for asset type: %s - it has "
      "already registered exporter <%s>",
      exp->getExporterIdStr(), exp->getAssetType(), perTypeExp[type]->getExporterIdStr());
    return false;
  }
  perTypeExp[type] = exp;
  exp->onRegister();
  return true;
}

bool DagorAssetMgr::unregisterAssetExporter(IDagorAssetExporter *exp)
{
  WriteGuard guard(mutex);
  int type = typeNames.getNameId(exp->getAssetType());
  if (type == -1)
  {
    post_error(*msgPipe, "exporter <%s> cannot be unregistered for non-allowed asset type: %s", exp->getExporterIdStr(),
      exp->getAssetType());
    return false;
  }
  while (type >= perTypeExp.size())
    perTypeExp.push_back(nullptr);
  if (type >= perTypeExp.size() || !perTypeExp[type])
  {
    post_error(*msgPipe,
      "exporter <%s> cannot be unregistered for asset type: %s - it was "
      "not registered yet",
      exp->getExporterIdStr(), exp->getAssetType());
    return false;
  }
  if (perTypeExp[type] != exp)
  {
    post_error(*msgPipe,
      "exporter <%s> cannot be unregistered for asset type: %s - it was "
      "registered for exporter <%s>",
      exp->getExporterIdStr(), exp->getAssetType(), perTypeExp[type]->getExporterIdStr());
    return false;
  }
  perTypeExp[type] = NULL;
  exp->onUnregister();
  return true;
}

bool DagorAssetMgr::registerAssetRefProvider(IDagorAssetRefProvider *exp)
{
  WriteGuard guard(mutex);
  int type = typeNames.getNameId(exp->getAssetType());
  if (type == -1)
  {
    post_error(*msgPipe, "exporter <%s> cannot be registered for non-allowed asset type: %s", exp->getRefProviderIdStr(),
      exp->getAssetType());
    return false;
  }
  while (type >= perTypeRefProv.size())
    perTypeRefProv.push_back(nullptr);
  if (perTypeRefProv[type])
  {
    post_error(*msgPipe,
      "exporter <%s> cannot be registered for asset type: %s - it has "
      "already registered exporter <%s>",
      exp->getRefProviderIdStr(), exp->getAssetType(), perTypeRefProv[type]->getRefProviderIdStr());
    return false;
  }
  perTypeRefProv[type] = exp;
  exp->onRegister();
  return true;
}

bool DagorAssetMgr::unregisterAssetRefProvider(IDagorAssetRefProvider *exp)
{
  WriteGuard guard(mutex);
  int type = typeNames.getNameId(exp->getAssetType());
  if (type == -1)
  {
    post_error(*msgPipe, "exporter <%s> cannot be unregistered for non-allowed asset type: %s", exp->getRefProviderIdStr(),
      exp->getAssetType());
    return false;
  }
  while (type >= perTypeRefProv.size())
    perTypeRefProv.push_back(nullptr);
  if (type >= perTypeRefProv.size() || !perTypeRefProv[type])
  {
    post_error(*msgPipe,
      "exporter <%s> cannot be unregistered for asset type: %s - it was "
      "not registered yet",
      exp->getRefProviderIdStr(), exp->getAssetType());
    return false;
  }
  if (perTypeRefProv[type] != exp)
  {
    post_error(*msgPipe,
      "exporter <%s> cannot be unregistered for asset type: %s - it was "
      "registered for exporter <%s>",
      exp->getRefProviderIdStr(), exp->getAssetType(), perTypeRefProv[type]->getRefProviderIdStr());
    return false;
  }
  perTypeRefProv[type] = NULL;
  exp->onUnregister();
  return true;
}

// package names resolution
static const char *get_custom_package_name1(DagorAsset *a, const char *pName, const char *target, const char *profile,
  const char *_pname)
{
  const DataBlock *b = a->props.getBlockByName(_pname);
  const char *p = a->props.getStr(_pname, NULL);
  if (b && target)
  {
    p = b->getStr(String(64, "%s_%s", pName, target), b->getStr(target, p));
    if (profile && *profile)
      p = b->getStr(String(64, "%s_%s~%s", pName, target, profile), p);
  }
  return p;
}

static const char *get_package_name1(DagorAssetMgr &mgr, int fidx, const char *paramName, const char *target_str, const char *profile,
  const char *_pname)
{
  while (auto *folder = mgr.getFolderPtr(fidx))
  {
    const char *p = folder->exportProps.getStr(_pname, NULL);
    if (p)
      return p;
    const DataBlock *b = folder->exportProps.getBlockByName(_pname);
    if (b)
    {
      if (target_str)
        if (const DataBlock *b2 = b->getBlockByName(target_str))
          b = b2;
      if (target_str && profile && *profile)
        if (const DataBlock *b2 = b->getBlockByName(String(128, "%s~%s", target_str, profile)))
          b = b2;
      p = b->getStr(paramName, NULL);
      if (p)
        return p;
    }
    fidx = folder->parentIdx;
  }
  return NULL;
}

static const char *resolve_pkname(const char *p, bool full) { return full ? ((p && strcmp(p, "*") == 0) ? NULL : p) : p; }

const char *DagorAsset::getCustomPackageName(const char *target, const char *profile, bool full)
{
  const char *pName = assetType == mgr.getTexAssetTypeId() ? "ddsxTex" : "gameRes";
  const char *pkg_name = nullptr;
  if (const char *p = mgr.getPkgNameForType(assetType))
    pkg_name = resolve_pkname(p, full);
  else if (const char *p = get_custom_package_name1(this, pName, target, profile, "forcePackage"))
    pkg_name = resolve_pkname(p, full);
  else if (const char *p = get_package_name1(mgr, folderIdx, pName, target, profile, "forcePackage"))
    pkg_name = resolve_pkname(p, full);
  else if (const char *p = get_custom_package_name1(this, pName, target, profile, "package"))
    pkg_name = resolve_pkname(p, full);
  else if (full)
    pkg_name = mgr.getPackageName(folderIdx, pName, target, profile);
  if (full && !pkg_name)
    return mgr.getBasePkg();
  return pkg_name;
}

const char *DagorAssetMgr::getPackageName(int fidx, const char *pName, const char *target, const char *profile)
{
  ReadGuard guard(mutex);
  return resolve_pkname(get_package_name1(*this, fidx, pName, target, profile, "package"), true);
}

// pack name resolution
static void makeRelFolderPath(String &tmpFolderPath, DagorAssetMgr &mgr, int fidx)
{
  Tab<int> p(tmpmem);
  while (fidx != 0)
  {
    p.push_back(fidx);
    fidx = mgr.getFolder(fidx).parentIdx;
  }

  tmpFolderPath.clear();
  for (int i = p.size() - 1; i >= 1; i--)
    tmpFolderPath.aprintf(64, "%s/", mgr.getFolder(p[i]).folderName.str());
  tmpFolderPath.aprintf(64, "%s", mgr.getFolder(p[0]).folderName.str());
}

static String get_pack_name_internal(DagorAssetMgr &mgr, int fidx, const char *paramName, const char *prefixParamName,
  const char *pack_suffix, const char *def_val, const char *root_def_val, const char *asset_pack_override, IDagorAssetMsgPipe &pipe)
{
  const char *prefix = "";
  const char *p;
  int src_fidx = fidx;

  if (asset_pack_override && *asset_pack_override == '/')
  {
    prefixParamName = nullptr;
    asset_pack_override++;
    if (!*asset_pack_override)
      asset_pack_override = nullptr;
  }

  if (asset_pack_override && (asset_pack_override[0] == '*' || !prefixParamName))
    p = asset_pack_override;
  else
    p = mgr.getFolder(fidx).exportProps.getStr(paramName, def_val);
  for (;;) // infinite loop with explicit break
  {
    if (!p)
    {
      post_error(pipe, "'%s' is NULL in %s/.folder.blk", paramName, mgr.getFolder(fidx).folderPath.str());
      return {};
    }
    if (!p[0])
      return {};
    if (!prefix[0] && prefixParamName)
      prefix = mgr.getFolder(fidx).exportProps.getStr(prefixParamName, "");

    if (asset_pack_override && asset_pack_override[0] != '*')
      if (p[0] != '*' || stricmp(p, "*name") == 0 || stricmp(p, "*name_src") == 0 || stricmp(p, "*path") == 0 ||
          stricmp(p, "*path_src") == 0)
        p = asset_pack_override;

    if (p[0] != '*')
    {
      if (p[0] == '/')
        return String(p + 1);
      if (!prefix[0])
        return String(p);
      return String::mk_str_cat(prefix, p);
    }

    if (stricmp(p, "*parent") == 0)
    {
      fidx = mgr.getFolder(fidx).parentIdx;
      if (fidx != -1)
      {
        p = mgr.getFolder(fidx).exportProps.getStr(paramName, def_val);
        continue;
      }
      p = NULL;
      break;
    }
    else if (stricmp(p, "*name") == 0)
      return String(0, "%s%s%s", prefix, mgr.getFolder(fidx).folderName.str(), pack_suffix);
    else if (stricmp(p, "*name_src") == 0)
      return String(0, "%s%s%s", prefix, mgr.getFolder(src_fidx).folderName.str(), pack_suffix);
    else if (stricmp(p, "*path") == 0)
    {
      String buf;
      makeRelFolderPath(buf, mgr, fidx);
      return String(0, "%s%s%s", prefix, buf.str(), pack_suffix);
    }
    else if (stricmp(p, "*path_src") == 0)
    {
      String buf;
      makeRelFolderPath(buf, mgr, src_fidx);
      return String(0, "%s%s%s", prefix, buf.str(), pack_suffix);
    }

    post_error(pipe, "'%s' is undefined '%s' in %s/.folder.blk", paramName, p, mgr.getFolder(fidx).folderPath.str());
    return {};
  }

  return String(root_def_val);
}

static inline String get_pack_name(DagorAssetMgr &mgr, int fidx, const char *paramName, const char *prefixParamName,
  const char *pack_suffix, const char *def_val, const char *root_def_val, const char *asset_pack_override, bool dabuild_collapse_packs,
  IDagorAssetMsgPipe &pipe)
{
  String nm =
    get_pack_name_internal(mgr, fidx, paramName, prefixParamName, pack_suffix, def_val, root_def_val, asset_pack_override, pipe);
  if (nm.empty() || !dabuild_collapse_packs)
    return nm;
  return String::mk_str_cat("allres", pack_suffix);
}

String DagorAsset::getDestPackName(bool dabuild_collapse_packs, bool pure_name, bool allow_grouping)
{
  bool is_tex = (assetType == mgr.getTexAssetTypeId());
  const char *explicit_nm = mgr.getPackNameForType(assetType, props.getStr(is_tex ? "ddsxTexPack" : "gameResPack", NULL));
  String grp_nm;
  if (allow_grouping && !mgr.getPackNameForType(assetType) &&
      props.getBool(is_tex ? "ddsxTexPackAllowGrouping" : "gameResPackAllowGrouping", true))
  {
    const char *suffix = is_tex ? ".dxp.bin" : ".grp";
    const char *tex_suf = is_tex ? strchr(getName(), '$') : nullptr;
    const char *texGrpParam = "ddsxTexPackGroupBQ";
    if (tex_suf)
    {
      if (strcmp(tex_suf, "$hq") == 0 || strcmp(tex_suf, "$uhq") == 0)
        texGrpParam = "ddsxTexPackGroupHQ";
      else if (strcmp(tex_suf, "$tq") == 0)
        texGrpParam = "ddsxTexPackGroupTQ";
    }

    grp_nm = get_pack_name(mgr, folderIdx, is_tex ? texGrpParam : "gameResPackGroup", NULL, suffix, "*parent", NULL, NULL,
      dabuild_collapse_packs, mgr.getMsgPipe());
    if (!grp_nm.empty() && grp_nm[0] != '*')
    {
      if (trail_strcmp(grp_nm, suffix))
        grp_nm.chop(strlen(suffix));
      explicit_nm = grp_nm;
    }
  }

  return mgr.getFolderPackName(folderIdx, is_tex, explicit_nm, dabuild_collapse_packs, pure_name, assetType);
}

String DagorAssetMgr::getFolderPackName(int folder_idx, bool for_tex, const char *asset_pack_override, bool dabuild_collapse_packs,
  bool pure_name, int a_type)
{
  const char *suffix = for_tex ? ".dxp.bin" : ".grp";
  String nm = get_pack_name(*this, folder_idx, for_tex ? "ddsxTexPack" : "gameResPack",
    (pure_name || getPackNameForType(a_type)) ? NULL : (for_tex ? "ddsxTexPackPrefix" : "gameResPackPrefix"), suffix,
    for_tex ? getLocDefDxp() : getLocDefGrp(), for_tex ? getDefDxpPath() : getDefGrpPath(), asset_pack_override,
    dabuild_collapse_packs, getMsgPipe());
  if (nm.empty())
    return nm;

  bool has_suffix = trail_strcmp(nm, suffix);
  if (has_suffix && pure_name)
    nm.chop(strlen(suffix));
  else if (!has_suffix && !pure_name)
    nm.append(suffix);
  return nm;
}

// splitNoSeparate resolution
static bool get_split_not_sep_for_asset(DagorAsset *a)
{
  if (a->props.paramExists("splitNotSeparate"))
    return a->props.getBool("splitNotSeparate");

  DagorAssetMgr &mgr = a->getMgr();
  int fidx = a->getFolderIndex();
  while (auto *folder = mgr.getFolderPtr(fidx))
  {
    if (folder->exportProps.paramExists("splitNotSeparate"))
      return folder->exportProps.getBool("splitNotSeparate");
    fidx = folder->parentIdx;
  }
  return false;
}

bool DagorAssetMgr::mountFmodEvents(const char *mount_folder_name)
{
  G_ASSERTF(0, "%s(%s) - support dropped on 18.09.2019", __FUNCTION__, mount_folder_name);
  return false;
}
