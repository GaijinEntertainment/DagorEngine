// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "assetBuildCache.h"
#include <assets/assetHlp.h>
#include <assets/daBuildInterface.h>
#include <assets/asset.h>
#include <libTools/dtx/ddsxPlugin.h>

#include "assetUserFlags.h"
#include "av_appwnd.h"
#include <EditorCore/ec_workspace.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_statDrv.h>

static IDaBuildInterface *dabuild = NULL;
static DagorAssetMgr *assetMgr = NULL;
static SimpleString startDir;

class DaBuildCacheChecker : public IDagorAssetBaseChangeNotify
{
public:
  DaBuildCacheChecker() : treeChanged(false) {}

  virtual void onAssetBaseChanged(dag::ConstSpan<DagorAsset *> changed_assets, dag::ConstSpan<DagorAsset *> added_assets,
    dag::ConstSpan<DagorAsset *> removed_assets)
  {
    for (int i = 0; i < changed_assets.size(); i++)
      if (const char *packname = dabuild->getPackName(changed_assets[i]))
        packs.addNameId(packname);
    for (int i = 0; i < added_assets.size(); i++)
      if (const char *packname = dabuild->getPackName(added_assets[i]))
        packs.addNameId(packname);
    for (int i = 0; i < removed_assets.size(); i++)
      if (const char *packname = dabuild->getPackName(removed_assets[i]))
        packs.addNameId(packname);
    treeChanged = added_assets.size() + removed_assets.size() > 0;
  }
  FastNameMapEx packs;
  bool treeChanged;
};
static DaBuildCacheChecker cacheChecker;

static inline void tab_from_namemap(Tab<const char *> &tab, FastNameMap &nm)
{
  tab.reserve(nm.nameCount());
  iterate_names(nm, [&](int, const char *name) { tab.push_back(name); });
}

bool init_dabuild_cache(const char *start_dir)
{
  startDir = start_dir;
  return dabuildcache::init(start_dir, &get_app().getConsole());
}
void term_dabuild_cache()
{
  texconvcache::term();
  dabuildcache::term();
  if (assetMgr)
    assetMgr->unsubscribeBaseUpdateNotify(&cacheChecker);
  assetMgr = NULL;
}

int bind_dabuild_cache_with_mgr(DagorAssetMgr &mgr, DataBlock &appblk, const char *appdir)
{
  int pcount = dabuildcache::bind_with_mgr(mgr, appblk, appdir);
  dabuild = pcount ? dabuildcache::get_dabuild() : NULL;
  assetMgr = pcount ? &mgr : NULL;
  if (texconvcache::init(mgr, appblk, startDir, false, true))
  {
    get_app().getConsole().addMessage(ILogWriter::NOTE, "texture conversion cache inited");
    int pc = ddsx::load_plugins(String(260, "%s/plugins/ddsx", startDir.str()));
    debug("loaded %d DDSx export plugin(s)", pc);
  }
  if (assetMgr)
    assetMgr->subscribeBaseUpdateNotify(&cacheChecker);
  return pcount;
}

void post_base_update_notify_dabuild()
{
  dabuildcache::post_base_update_notify();
  if (cacheChecker.packs.nameCount())
  {
    Tab<const char *> packs;
    tab_from_namemap(packs, cacheChecker.packs);
    debug("will check up-to-date for:");
    for (const char *nm : packs)
      debug("  %s", nm);
    check_assets_base_up_to_date(packs, true, true);
  }
  if (cacheChecker.treeChanged)
  {
    cacheChecker.treeChanged = false;
    get_app().refillTree();
  }
}


static void getPackForFolder(FastNameMap &packs, dag::ConstSpan<int> folders_idx, bool tex, bool res)
{
  if (!assetMgr || !dabuild)
    return;

  int tex_tid = assetMgr->getTexAssetTypeId();
  int fldCnt = folders_idx.size();
  for (int i = 0; i < fldCnt; i++)
  {
    int start_idx, end_idx;
    assetMgr->getFolderAssetIdxRange(folders_idx[i], start_idx, end_idx);
    for (int j = start_idx; j < end_idx; j++)
    {
      DagorAsset &a = assetMgr->getAsset(j);
      if (a.getFileNameId() < 0)
        continue;
      if (a.getType() == tex_tid && !tex)
        continue;
      if (a.getType() != tex_tid && !res)
        continue;
      if (const char *pn = dabuild->getPackName(&a))
        packs.addNameId(pn);
    }
  }
}

const char *get_asset_pack_name(DagorAsset *asset) { return dabuild ? dabuild->getPackName(asset) : NULL; }
String get_asset_pkg_name(DagorAsset *asset) { return dabuild ? dabuild->getPkgName(asset) : String(); }

bool check_assets_base_up_to_date(dag::ConstSpan<const char *> packs, bool tex, bool res)
{
  TIME_PROFILE(check_assets_base_up_to_date);
  if (!dabuild)
    return false;

  int64_t startTime = profile_ref_ticks();

  ILogWriter *log = &::get_app().getConsole();

  log->addMessage(ILogWriter::NOTE, "checking assets up-to-date");

  dabuild->setupReports(log, &::get_app().getConsole());

  Tab<unsigned> platforms(::get_app().getWorkspace().getAdditionalPlatforms(), tmpmem);
  platforms.push_back(_MAKE4C('PC'));

  static Tab<int> platformFlags(inimem);
  platformFlags.clear();

  int pCnt = platforms.size();
  for (int i = 0; i < pCnt; i++)
  {
    if (platforms[i] == _MAKE4C('PC'))
      platformFlags.push_back(ASSET_USER_FLG_UP_TO_DATE_PC);
    else if (platforms[i] == _MAKE4C('PS4'))
      platformFlags.push_back(ASSET_USER_FLG_UP_TO_DATE_PS4);
    else if (platforms[i] == _MAKE4C('iOS'))
      platformFlags.push_back(ASSET_USER_FLG_UP_TO_DATE_iOS);
    else if (platforms[i] == _MAKE4C('and'))
      platformFlags.push_back(ASSET_USER_FLG_UP_TO_DATE_AND);
    else
    {
      log->addMessage(ILogWriter::ERROR, "unsupported platform <%u> '%c%c%c%c'", platforms[i], _DUMP4C(platforms[i]));
      dabuild->setupReports(NULL, NULL);

      log->addMessage(ILogWriter::ERROR, "checking assets up-to-date...failed!");
      return false;
    }
  }

  G_ASSERT(platforms.size() == platformFlags.size());

  bool ret = dabuild->checkUpToDate(platforms, make_span(platformFlags), packs);

  dabuild->setupReports(NULL, NULL);

  log->addMessage(ILogWriter::NOTE, "checking assets up-to-date...complete in %.2fs", ((float)profile_time_usec(startTime)) / 1e6f);

  EDITORCORE->updateViewports();
  EDITORCORE->invalidateViewportCache();
  ;
  ::get_app().afterUpToDateCheck(ret);

  return ret;
}

void rebuild_assets_in_folders_single(unsigned trg_code, dag::ConstSpan<int> folders_idx, bool tex, bool res)
{
  if (!dabuild || !assetMgr)
    return;

  rebuild_assets_in_folders(make_span_const(&trg_code, 1), folders_idx, tex, res);
}

void rebuild_assets_in_folders(dag::ConstSpan<unsigned> tc, dag::ConstSpan<int> folders_idx, bool tex, bool res)
{
  if (!dabuild || !assetMgr)
    return;

  ILogWriter *log = &::get_app().getConsole();

  ::get_app().getConsole().showConsole();

  dabuild->setupReports(log, &::get_app().getConsole());

  FastNameMap _packs;
  getPackForFolder(_packs, folders_idx, tex, res);
  Tab<const char *> packs;
  tab_from_namemap(packs, _packs);

  if (dabuild->exportPacks(tc, packs))
    ::get_app().getConsole().hideConsole();

  dabuild->setupReports(NULL, NULL);

  check_assets_base_up_to_date(packs, tex, res);
}

void rebuild_assets_in_root(dag::ConstSpan<unsigned> tc, bool build_tex, bool build_res)
{
  if (!dabuild || !assetMgr)
    return;

  ILogWriter *log = &::get_app().getConsole();

  ::get_app().getConsole().showConsole();

  dabuild->setupReports(log, &::get_app().getConsole());

  int tex_tid = assetMgr->getTexAssetTypeId();
  FastNameMap _packs;
  for (int j = 0; j < assetMgr->getAssetCount(); j++)
  {
    DagorAsset &a = assetMgr->getAsset(j);
    if (a.getFileNameId() < 0)
      continue;
    if (a.getType() == tex_tid && !build_tex)
      continue;
    if (a.getType() != tex_tid && !build_res)
      continue;
    if (const char *pn = dabuild->getPackName(&a))
      _packs.addNameId(pn);
  }

  Tab<const char *> packs;
  tab_from_namemap(packs, _packs);

  if (!dabuild->exportPacks(tc, packs))
    ::get_app().getConsole().hideConsole();

  dabuild->setupReports(NULL, NULL);

  check_assets_base_up_to_date({}, build_tex, build_res);
}

void rebuild_assets_in_root_single(unsigned trg_code, bool build_tex, bool build_res)
{
  if (!dabuild || !assetMgr)
    return;

  rebuild_assets_in_root(make_span_const(&trg_code, 1), build_tex, build_res);
}

void destroy_assets_cache(dag::ConstSpan<unsigned> tc)
{
  if (!dabuild || !assetMgr)
    return;

  ILogWriter *log = &::get_app().getConsole();

  dabuild->setupReports(log, &::get_app().getConsole());

  dabuild->destroyCache(tc);

  dabuild->setupReports(NULL, NULL);

  check_assets_base_up_to_date({}, true, true);
}

void destroy_assets_cache_single(unsigned trg_code)
{
  if (!dabuild || !assetMgr)
    return;

  destroy_assets_cache(make_span_const(&trg_code, 1));
}

void build_assets(dag::ConstSpan<unsigned> tc, dag::ConstSpan<DagorAsset *> assets)
{
  if (!dabuild || !assetMgr)
    return;

  static Tab<SimpleString> buffer;
  buffer.clear();

  Tab<const char *> packs;

  int cnt = assets.size();
  buffer.resize(cnt);
  packs.resize(cnt);

  ILogWriter *log = &::get_app().getConsole();

  ::get_app().getConsole().showConsole();

  dabuild->setupReports(log, &::get_app().getConsole());

  for (int i = 0; i < cnt; i++)
  {
    buffer[i] = dabuild->getPackName(assets[i]);
    packs[i] = buffer[i];
  }

  if (dabuild->exportPacks(tc, packs))
    ::get_app().getConsole().hideConsole();

  dabuild->setupReports(NULL, NULL);

  check_assets_base_up_to_date(packs, true, true);
}

bool is_asset_exportable(DagorAsset *a)
{
  return a && a->getFileNameId() >= 0 && dabuild->isAssetExportable(a) && ::get_asset_pack_name(a);
}
