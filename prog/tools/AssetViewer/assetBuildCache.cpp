// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "assetBuildCache.h"
#include <assets/assetHlp.h>
#include <assets/daBuildInterface.h>
#include <assets/asset.h>
#include <libTools/dtx/ddsxPlugin.h>

#include "assetUserFlags.h"
#include "av_appwnd.h"
#include "av_cm.h"
#include <EditorCore/ec_input.h>
#include <EditorCore/ec_workspace.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_statDrv.h>
#include <de3_interface.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <libTools/util/strUtil.h>
#include <osApiWrappers/dag_sharedMem.h>
#include <osApiWrappers/dag_miscApi.h>
#include <assets/daBuildProgressShm.h>
#include <libTools/util/progressInd.h>
#include <propPanel/colors.h>
#include <propPanel/propPanel.h>
#include <propPanel/imguiHelper.h>
#include <EASTL/hash_map.h>
#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <dag/dag_vector.h>
#if _TARGET_PC_WIN
#include <windows.h>
#undef ERROR
#elif _TARGET_PC_LINUX || _TARGET_APPLE
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#endif

static IDaBuildInterface *dabuild = NULL;
static DagorAssetMgr *assetMgr = NULL;
static SimpleString startDir;
static String buildConfigPath;
extern bool av_perform_uptodate_check;
extern bool dabuildWindowVisible;
extern bool daBuildWindowAutoOpen;
extern bool daBuildWindowAutoClose;
static bool patchBuildMode = false;
static bool wasDabuildRunning = false;
static ImGuiID savedDockNodeId = 0;
static ImGuiID savedPrevTabId = 0;
static bool autoOpenDidAnything = false;
static bool wasVisibleBeforeBuild = false;
static bool pendingBringToFront = false;
static const char daBuildPanelName[] = "daBuild##v2";

struct PendingLogEntry
{
  ILogWriter::MessageType type;
  String text;
};

static WinCritSec logMutex;
static Tab<PendingLogEntry> pendingLogs;

// 0 = idle, 1 = running, 2 = done-success, 3 = done-failed, 4 = done-cancelled
static volatile int dabuildStatus = 0;

#if _TARGET_PC_WIN
static void *volatile dabuildProcess = NULL;
static void *volatile dabuildReadPipe = NULL;
static void *volatile dabuildJobObject = NULL;
#elif _TARGET_PC_LINUX || _TARGET_APPLE
static volatile int dabuildPid = -1;
static volatile int dabuildReadPipeFd = -1;
#endif

static DaBuildProgressShm *progressShm = nullptr;
static intptr_t progressShmHandle = 0;
static String progressShmName;

static volatile bool inprocBuildCancelled = false;
static volatile bool inprocThreadDone = true;

class ThreadedLogWriter : public ILogWriter
{
public:
  void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum) override
  {
    String msg;
    msg.vprintf(0, fmt, arg, anum);
    WinAutoLock lock(logMutex);
    pendingLogs.push_back({type, eastl::move(msg)});
  }
  bool hasErrors() const override { return false; }
  void startLog() override {}
  void endLog() override {}
};

class ThreadedProgressIndicator : public IGenericProgressIndicator
{
  volatile int packDoneVal = 0;
  volatile int packTotalVal = 0;
  volatile int phaseVal = 0; // 1 = tex, 2 = res

public:
  void setPhase(int p) { interlocked_relaxed_store(phaseVal, p); }

  void setActionDescFmt(const char *, const DagorSafeArg *, int) override {}
  void setTotal(int total) override
  {
    interlocked_relaxed_store(packTotalVal, total);
    interlocked_relaxed_store(packDoneVal, 0);
  }
  void setDone(int done) override { interlocked_relaxed_store(packDoneVal, done); }
  void incDone(int inc = 1) override { interlocked_add(packDoneVal, inc); }
  void redrawScreen() override {}
  void destroy() override {}
  void startProgress(IProgressCB *) override {}
  void endProgress() override {}

  DaBuildProgress snap() const
  {
    DaBuildProgress p;
    p.phase = interlocked_relaxed_load(phaseVal);
    p.packDone = interlocked_relaxed_load(packDoneVal);
    p.packTotal = interlocked_relaxed_load(packTotalVal);
    p.assetDone = 0;
    p.assetTotal = 0;
    return p;
  }
};
static ThreadedProgressIndicator inprocPbar;

static void close_progress_shm()
{
  if (!progressShmHandle)
    return;
  close_global_map_shared_mem(progressShmHandle, progressShm, sizeof(DaBuildProgressShm));
  unlink_global_shared_mem(progressShmName);
  progressShmHandle = 0;
  progressShm = nullptr;
}

static DaBuildProgress dabuildProgress;

DaBuildProgress get_dabuild_progress() { return dabuildProgress; }

static int dabuildJobs = 0;
static int maxJobs = 32;
static bool currentBuildUsesJobs = false;

int get_dabuild_jobs() { return dabuildJobs; }
void set_dabuild_jobs(int jobs) { dabuildJobs = jobs < 0 ? 0 : jobs > maxJobs ? maxJobs : jobs; }
int get_dabuild_max_jobs() { return maxJobs; }
bool get_dabuild_current_build_uses_jobs() { return currentBuildUsesJobs; }

struct DaBuildPostParams
{
  Tab<uint64_t> ids;
  Tab<unsigned> tc;
  bool checkTex = true;
  bool checkRes = true;
  bool emptyPacksForUpToDateCheck = false;
  bool consoleWasOpen = false;
  bool hideConsoleOnSuccess = true;
};
static DaBuildPostParams buildPostParams;

static bool launch_build_inproc_async(DaBuildPostParams postParams);
static bool launch_build_async(DaBuildPostParams postParams);
static bool launch_build(DaBuildPostParams postParams);

struct QueueEntry
{
  Tab<unsigned> tcs;
};

static constexpr const char *BASE_PKG_NAME = "MAIN";
static constexpr const char *DABUILD_BASE_PKG_SELECTOR = "*";

struct PkgGroup
{
  SimpleString name;
  Tab<SimpleString> packs; // sorted alphabetically
};

// knownPkgGroups[0] is always BASE_PKG_NAME (MAIN); remaining are sorted alphabetically
static Tab<PkgGroup> knownPkgGroups;

static eastl::hash_map<uint64_t, QueueEntry> queueMap;
static eastl::hash_map<uint64_t, Tab<unsigned>> currentlyBuildingMap;
static Tab<DaBuildPostParams> pendingBuildQueue;

static String packSearchText;
static bool packFilterSelected = false;
static bool packFilterTex = true;
static bool packFilterRes = true;

static PropPanel::IconId searchIconId = PropPanel::IconId::Invalid;
static PropPanel::IconId clearIconId = PropPanel::IconId::Invalid;
static PropPanel::IconId filterDefaultIconId = PropPanel::IconId::Invalid;
static PropPanel::IconId filterActiveIconId = PropPanel::IconId::Invalid;
static bool packFilterPopupOpen = false;
static int packSearchFocusId = 0;

static bool autoJobs = false;
static bool autoJobsActive = false;

static inline uint32_t pack_id_pkg_idx(uint64_t id) { return (uint32_t)(id >> 32); }
static inline uint32_t pack_id_pack_idx(uint64_t id) { return (uint32_t)(id & 0xFFFFFFFF); }

static const char *pack_id_pkg_name(uint64_t id)
{
  uint32_t pi = pack_id_pkg_idx(id);
  return pi < (uint32_t)knownPkgGroups.size() ? knownPkgGroups[pi].name.str() : "";
}

static const char *pack_id_pack_name(uint64_t id)
{
  uint32_t pi = pack_id_pkg_idx(id);
  uint32_t ci = pack_id_pack_idx(id);
  if (pi >= (uint32_t)knownPkgGroups.size())
    return "";
  const PkgGroup &g = knownPkgGroups[pi];
  return ci < (uint32_t)g.packs.size() ? g.packs[ci].str() : "";
}

uint64_t make_pack_id(const char *pkg, const char *pack)
{
  const char *effectivePkg = (!pkg || !*pkg) ? BASE_PKG_NAME : pkg;
  for (uint32_t pi = 0; pi < (uint32_t)knownPkgGroups.size(); ++pi)
  {
    if (strcmp(knownPkgGroups[pi].name.str(), effectivePkg) != 0)
      continue;
    const PkgGroup &g = knownPkgGroups[pi];
    for (uint32_t ci = 0; ci < (uint32_t)g.packs.size(); ++ci)
      if (strcmp(g.packs[ci].str(), pack) == 0)
        return ((uint64_t)pi << 32) | ci;
    return INVALID_PACK_ID;
  }
  return INVALID_PACK_ID;
}

static int compute_auto_jobs(const DaBuildPostParams &params)
{
  bool anyTex = false;
  int packCount = 0;
  for (uint64_t id : params.ids)
  {
    if (trail_strcmp(pack_id_pack_name(id), ".dxp.bin"))
      anyTex = true;
    ++packCount;
  }
  if (anyTex)
    return maxJobs;
  int count = packCount * (int)params.tc.size();
  return (count <= 1) ? 0 : min(count, maxJobs);
}

static void finish_auto_jobs() { autoJobsActive = false; }

void queue_add_pack(uint64_t pack_id, unsigned tc)
{
  if (pack_id == INVALID_PACK_ID)
    return;
  auto &entry = queueMap[pack_id];
  if (eastl::find(entry.tcs.begin(), entry.tcs.end(), tc) == entry.tcs.end())
    entry.tcs.push_back(tc);
}

void queue_remove_pack(uint64_t pack_id, unsigned tc)
{
  if (pack_id == INVALID_PACK_ID)
    return;
  auto it = queueMap.find(pack_id);
  if (it == queueMap.end())
    return;
  auto &tcs = it->second.tcs;
  tcs.erase(eastl::remove(tcs.begin(), tcs.end(), tc), tcs.end());
}

void queue_toggle_pack(uint64_t pack_id, unsigned tc)
{
  if (pack_id == INVALID_PACK_ID)
    return;
  auto it = queueMap.find(pack_id);
  if (it != queueMap.end())
  {
    auto &tcs = it->second.tcs;
    auto pos = eastl::find(tcs.begin(), tcs.end(), tc);
    if (pos != tcs.end())
    {
      tcs.erase(pos);
      return;
    }
  }
  queue_add_pack(pack_id, tc);
}

void queue_add_pack_all_platforms(uint64_t pack_id)
{
  queue_add_pack(pack_id, _MAKE4C('PC'));
  for (unsigned tc : ::get_app().getWorkspace().getAdditionalPlatforms())
    queue_add_pack(pack_id, tc);
}

void queue_toggle_all_platforms(uint64_t pack_id)
{
  if (!queue_get_pack_tcs(pack_id).empty())
  {
    auto it = queueMap.find(pack_id);
    if (it != queueMap.end())
      it->second.tcs.clear();
  }
  else
    queue_add_pack_all_platforms(pack_id);
}

void queue_remove_all() { queueMap.clear(); }

void queue_select_all_known_packs()
{
  for (uint32_t pi = 0; pi < (uint32_t)knownPkgGroups.size(); ++pi)
    for (uint32_t ci = 0; ci < (uint32_t)knownPkgGroups[pi].packs.size(); ++ci)
      queue_add_pack_all_platforms(((uint64_t)pi << 32) | ci);
}

dag::ConstSpan<unsigned> queue_get_pack_tcs(uint64_t pack_id)
{
  auto it = queueMap.find(pack_id);
  if (it == queueMap.end() || it->second.tcs.empty())
    return {};
  return make_span_const(it->second.tcs);
}

static void queue_save_config()
{
  if (buildConfigPath.empty())
    return;

  DataBlock blk;
  DataBlock *preset = blk.addNewBlock("export_preset");

  if (dabuildJobs != 0)
    preset->setInt("jobs", dabuildJobs);
  if (autoJobs)
    preset->setBool("autoJobs", true);

  for (auto &kv : queueMap)
  {
    if (kv.second.tcs.empty())
      continue;
    const char *pkg = pack_id_pkg_name(kv.first);
    const char *name = pack_id_pack_name(kv.first);
    if (!name || !*name)
      continue;
    DataBlock *entry = preset->addNewBlock("pack");
    entry->setStr("pkg", pkg);
    entry->setStr("name", name);
    for (unsigned tc : kv.second.tcs)
      entry->addInt("tc", (int)tc);
  }

  blk.saveToTextFile(buildConfigPath);
}

static void queue_load_config()
{
  if (buildConfigPath.empty())
    return;

  DataBlock blk;
  if (!blk.load(buildConfigPath))
    return;

  const DataBlock *preset = blk.getBlockByName("export_preset");
  if (!preset)
    return;

  set_dabuild_jobs(preset->getInt("jobs", dabuildJobs));
  autoJobs = preset->getBool("autoJobs", false);

  queueMap.clear();
  int packNid = preset->getNameId("pack");
  for (int i = 0; i < preset->blockCount(); i++)
  {
    const DataBlock *entry = preset->getBlock(i);
    if (entry->getBlockNameId() != packNid)
      continue;
    const char *name = entry->getStr("name", "");
    if (!*name)
      continue;
    const char *pkg = entry->getStr("pkg", nullptr);
    int tcNid = entry->getNameId("tc");
    Tab<unsigned> tcs;
    for (int j = 0; j < entry->paramCount(); j++)
      if (entry->getParamNameId(j) == tcNid && entry->getParamType(j) == DataBlock::TYPE_INT)
        tcs.push_back((unsigned)entry->getInt(j));
    if (pkg)
    {
      uint64_t id = make_pack_id(pkg, name);
      if (id != INVALID_PACK_ID)
      {
        QueueEntry &qe = queueMap[id];
        for (unsigned tc : tcs)
          if (eastl::find(qe.tcs.begin(), qe.tcs.end(), tc) == qe.tcs.end())
            qe.tcs.push_back(tc);
      }
    }
    else
    {
      // fan-out to all groups that contain a pack with this name
      bool matched = false;
      for (uint32_t pi = 0; pi < (uint32_t)knownPkgGroups.size(); ++pi)
        for (uint32_t ci = 0; ci < (uint32_t)knownPkgGroups[pi].packs.size(); ++ci)
          if (strcmp(knownPkgGroups[pi].packs[ci].str(), name) == 0)
          {
            matched = true;
            uint64_t id = ((uint64_t)pi << 32) | ci;
            QueueEntry &qe = queueMap[id];
            for (unsigned tc : tcs)
              if (eastl::find(qe.tcs.begin(), qe.tcs.end(), tc) == qe.tcs.end())
                qe.tcs.push_back(tc);
          }
      if (!matched)
      {
        uint64_t id = make_pack_id(BASE_PKG_NAME, name);
        if (id != INVALID_PACK_ID)
        {
          QueueEntry &qe = queueMap[id];
          for (unsigned tc : tcs)
            if (eastl::find(qe.tcs.begin(), qe.tcs.end(), tc) == qe.tcs.end())
              qe.tcs.push_back(tc);
        }
      }
    }
  }
}

void export_queue()
{
  if (is_dabuild_running())
    return;

  struct Group
  {
    Tab<uint64_t> ids;
    Tab<unsigned> tcs;
  };
  dag::Vector<Group> groups;

  for (auto &kv : queueMap)
  {
    if (kv.second.tcs.empty())
      continue;
    const Tab<unsigned> &tcs = kv.second.tcs;

    G_ASSERT(tcs.size() > 0);

    bool found = false;
    for (Group &g : groups)
    {
      if (g.tcs.size() == tcs.size() && eastl::equal(g.tcs.begin(), g.tcs.end(), tcs.begin()))
      {
        g.ids.push_back(kv.first);
        found = true;
        break;
      }
    }
    if (!found)
    {
      Group g;
      g.tcs.assign(tcs.begin(), tcs.end());
      g.ids.push_back(kv.first);
      groups.push_back(eastl::move(g));
    }
  }

  if (groups.empty())
    return;

  currentlyBuildingMap.clear();
  for (auto &kv : queueMap)
    if (!kv.second.tcs.empty())
      currentlyBuildingMap[kv.first].assign(kv.second.tcs.begin(), kv.second.tcs.end());

  bool consoleWasOpen = ::get_app().getConsole().isVisible();
  pendingBuildQueue.clear();
  for (Group &g : groups)
  {
    DaBuildPostParams params;
    params.consoleWasOpen = consoleWasOpen;
    params.hideConsoleOnSuccess = true;
    params.checkTex = true;
    params.checkRes = true;
    params.emptyPacksForUpToDateCheck = false;
    params.tc = eastl::move(g.tcs);

    params.ids.assign(g.ids.begin(), g.ids.end());

    pendingBuildQueue.push_back(eastl::move(params));
  }

  DaBuildPostParams first = eastl::move(pendingBuildQueue.front());
  pendingBuildQueue.erase(pendingBuildQueue.begin());

  launch_build(eastl::move(first));
}


class DaBuildCacheChecker : public IDagorAssetBaseChangeNotify
{
public:
  DaBuildCacheChecker() : treeChanged(false) {}

  void onAssetBaseChanged(dag::ConstSpan<DagorAsset *> changed_assets, dag::ConstSpan<DagorAsset *> added_assets,
    dag::ConstSpan<DagorAsset *> removed_assets) override
  {
    for (int i = 0; i < changed_assets.size(); i++)
      if (String packname = dabuild->getPackName(changed_assets[i]); !packname.empty())
        packs.addNameId(packname);
    for (int i = 0; i < added_assets.size(); i++)
      if (String packname = dabuild->getPackName(added_assets[i]); !packname.empty())
        packs.addNameId(packname);
    for (int i = 0; i < removed_assets.size(); i++)
      if (String packname = dabuild->getPackName(removed_assets[i]); !packname.empty())
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
  stop_dabuild_background();
  queue_save_config();
  texconvcache::term();
  dabuildcache::term();
  if (assetMgr)
    assetMgr->unsubscribeBaseUpdateNotify(&cacheChecker);
  assetMgr = NULL;
}

int bind_dabuild_cache_with_mgr(DagorAssetMgr &mgr, DataBlock &appblk, const char *appdir)
{
  TIME_PROFILE(bind_dabuild_cache_with_mgr);

  maxJobs = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getInt("maxJobs", 32);
  int pcount = dabuildcache::bind_with_mgr(mgr, appblk, appdir);
  dabuild = pcount ? dabuildcache::get_dabuild() : NULL;
  assetMgr = pcount ? &mgr : NULL;
  patchBuildMode = appblk.getBool("av_patch_build", true);
  if (dabuild)
    dabuild->allowPatchBuild(patchBuildMode);
  if (texconvcache::init(mgr, appblk, startDir, false, true))
  {
    get_app().getConsole().addMessage(ILogWriter::NOTE, "texture conversion cache inited");
    int pc = ddsx::load_plugins(String(260, "%s/plugins/ddsx", startDir.str()));
    debug("loaded %d DDSx export plugin(s)", pc);
  }
  if (assetMgr)
    assetMgr->subscribeBaseUpdateNotify(&cacheChecker);

  if (dabuild && assetMgr)
  {
    eastl::hash_map<eastl::string, FastNameMap> pkgPackMap;
    for (int i = 0; i < assetMgr->getAssetCount(); i++)
    {
      DagorAsset &a = assetMgr->getAsset(i);
      if (a.getFileNameId() < 0)
        continue;
      String pn = dabuild->getPackName(&a);
      if (pn.empty())
        continue;
      String pkg = dabuild->getPkgName(&a);
      const char *pkgKey = pkg.empty() ? BASE_PKG_NAME : pkg.str();
      pkgPackMap[eastl::string(pkgKey)].addNameId(pn);
    }

    knownPkgGroups.clear();

    {
      PkgGroup g;
      g.name = BASE_PKG_NAME;
      auto it = pkgPackMap.find(eastl::string(BASE_PKG_NAME));
      if (it != pkgPackMap.end())
      {
        iterate_names(it->second, [&](int, const char *name) { g.packs.push_back(SimpleString(name)); });
        eastl::sort(g.packs.begin(), g.packs.end(),
          [](const SimpleString &a, const SimpleString &b) { return strcmp(a.str(), b.str()) < 0; });
      }
      knownPkgGroups.push_back(eastl::move(g));
    }

    Tab<SimpleString> otherPkgs;
    for (auto &kv : pkgPackMap)
      if (strcmp(kv.first.c_str(), BASE_PKG_NAME) != 0)
        otherPkgs.push_back(SimpleString(kv.first.c_str()));
    eastl::sort(otherPkgs.begin(), otherPkgs.end(),
      [](const SimpleString &a, const SimpleString &b) { return strcmp(a.str(), b.str()) < 0; });

    for (const SimpleString &pkgName : otherPkgs)
    {
      PkgGroup g;
      g.name = pkgName;
      auto it = pkgPackMap.find(eastl::string(pkgName.str()));
      if (it != pkgPackMap.end())
      {
        iterate_names(it->second, [&](int, const char *name) { g.packs.push_back(SimpleString(name)); });
        eastl::sort(g.packs.begin(), g.packs.end(),
          [](const SimpleString &a, const SimpleString &b) { return strcmp(a.str(), b.str()) < 0; });
      }
      knownPkgGroups.push_back(eastl::move(g));
    }

    searchIconId = PropPanel::load_icon("search");
    clearIconId = PropPanel::load_icon("close_editor");
    filterDefaultIconId = PropPanel::load_icon("filter_default");
    filterActiveIconId = PropPanel::load_icon("filter_active");
  }

  buildConfigPath = assetlocalprops::makePath("build_config.local.blk");
  queue_load_config();

  return pcount;
}

bool is_dabuild_running() { return interlocked_relaxed_load(dabuildStatus) == 1; }

void stop_dabuild_background()
{
  if (!interlocked_relaxed_load(inprocThreadDone))
  {
    interlocked_relaxed_store(inprocBuildCancelled, true);
    for (int i = 0; i < 600 && !interlocked_relaxed_load(inprocThreadDone); i++)
      sleep_msec(100);
  }

#if _TARGET_PC_WIN
  HANDLE proc = (HANDLE)interlocked_exchange_ptr(dabuildProcess, (void *)nullptr);
  if (!proc)
    return;

  HANDLE jobObj = (HANDLE)interlocked_exchange_ptr(dabuildJobObject, (void *)nullptr);
  if (jobObj)
  {
    TerminateJobObject(jobObj, 1);
    CloseHandle(jobObj);
  }
  else
    TerminateProcess(proc, 1);
  WaitForSingleObject(proc, 3000);
  CloseHandle(proc);

  HANDLE rp = (HANDLE)interlocked_exchange_ptr(dabuildReadPipe, (void *)nullptr);
  if (rp)
    CloseHandle(rp);
#elif _TARGET_PC_LINUX || _TARGET_APPLE
  int pid = interlocked_exchange(dabuildPid, -1);
  if (pid <= 0)
    return;

  kill(-(pid_t)pid, SIGTERM);

  int rfd = interlocked_exchange(dabuildReadPipeFd, -1);
  if (rfd >= 0)
    close(rfd);

  int wstatus = 0;
  bool reaped = false;
  for (int i = 0; i < 30; i++)
  {
    if (waitpid((pid_t)pid, &wstatus, WNOHANG) > 0)
    {
      reaped = true;
      break;
    }
    sleep_msec(100);
  }
  if (!reaped)
  {
    kill(-(pid_t)pid, SIGKILL);
    waitpid((pid_t)pid, &wstatus, 0);
  }
#else
  return;
#endif
  close_progress_shm();
}
static void activate_dabuild_window()
{
  ImGuiWindow *win = ImGui::FindWindowByName(daBuildPanelName);
  if (!win)
  {
    pendingBringToFront = true;
    return;
  }

  ImGuiDockNode *node = win->DockNode;
  if (node && node->TabBar)
    node->TabBar->NextSelectedTabId = win->TabId;
  ImGui::FocusWindow(win);
}

static void bring_dabuild_to_front()
{
  ImGuiWindow *win = ImGui::FindWindowByName(daBuildPanelName);
  if (!win)
  {
    pendingBringToFront = true;
    autoOpenDidAnything = true;
    return;
  }

  ImGuiDockNode *node = win->DockNode;
  if (node && node->TabBar && node->TabBar->SelectedTabId != win->TabId)
  {
    savedDockNodeId = node->ID;
    savedPrevTabId = node->TabBar->SelectedTabId;
    node->TabBar->NextSelectedTabId = win->TabId;
    ImGui::FocusWindow(win);
    autoOpenDidAnything = true;
  }
  else if (!(node && node->TabBar))
  {
    ImGuiContext *ctx = ImGui::GetCurrentContext();
    if (ctx->NavWindow != win)
    {
      ImGui::FocusWindow(win);
      autoOpenDidAnything = true;
    }
  }
  // else: already active tab
}

void bring_dabuild_to_front_explicit() { pendingBringToFront = true; }

static void restore_previous_tab()
{
  ImGuiContext *ctx = ImGui::GetCurrentContext();
  ImGuiDockNode *node = ImGui::DockContextFindNodeByID(ctx, savedDockNodeId);
  if (!node || !node->TabBar)
    return;
  ImGuiTabItem *tab = ImGui::TabBarFindTabByID(node->TabBar, savedPrevTabId);
  if (!tab)
    return;
  node->TabBar->NextSelectedTabId = savedPrevTabId;
  if (tab->Window)
    ImGui::FocusWindow(tab->Window);
}

void update_dabuild_background(PropPanel::IMenu *mm)
{
  bool isDabuildRunning = is_dabuild_running();
  if (wasDabuildRunning != isDabuildRunning)
  {
    if (isDabuildRunning)
    {
      autoOpenDidAnything = false;
      wasVisibleBeforeBuild = dabuildWindowVisible;
      savedDockNodeId = 0;
      savedPrevTabId = 0;
      if (daBuildWindowAutoOpen)
      {
        dabuildWindowVisible = true;
        bring_dabuild_to_front();
      }
    }
    else if (pendingBuildQueue.empty())
    {
      if (daBuildWindowAutoClose && autoOpenDidAnything)
      {
        if (savedPrevTabId)
        {
          restore_previous_tab();
          if (!wasVisibleBeforeBuild)
            dabuildWindowVisible = false;
        }
        else
        {
          if (!wasVisibleBeforeBuild)
            dabuildWindowVisible = false;
        }
      }
      savedDockNodeId = 0;
      savedPrevTabId = 0;
      autoOpenDidAnything = false;
    }

    wasDabuildRunning = isDabuildRunning;
  }

  if (mm)
    mm->setCheckById(CM_WINDOW_DABUILD, dabuildWindowVisible);

  {
    WinAutoLock lock(logMutex);
    if (!pendingLogs.empty())
    {
      ILogWriter *log = &::get_app().getConsole();
      for (PendingLogEntry &entry : pendingLogs)
        log->addMessage(entry.type, "%s", entry.text.str());
      pendingLogs.clear();
    }
  }

  if (progressShm)
  {
    DaBuildProgress snap;
    progressShm->readSnap(snap.phase, snap.packDone, snap.packTotal, snap.assetDone, snap.assetTotal);
    dabuildProgress = snap;
  }
  else if (interlocked_relaxed_load(dabuildStatus) == 1)
    dabuildProgress = inprocPbar.snap();

  int status = interlocked_acquire_load(dabuildStatus);
  if (status < 2) // idle or still running
    return;

  bool cancelled = (status == 4);
  interlocked_exchange(dabuildStatus, 0);
  close_progress_shm();

  // the batch run may have changed respacks/patch validity on disk regardless of its outcome
  dabuildcache::invalidate_respack_caches();

  if (cancelled)
  {
    pendingBuildQueue.clear();
    currentlyBuildingMap.clear();
    finish_auto_jobs();
    return;
  }

  if (buildPostParams.emptyPacksForUpToDateCheck)
    check_assets_base_up_to_date({}, buildPostParams.checkTex, buildPostParams.checkRes);
  else
  {
    Tab<const char *> packs;
    for (uint64_t id : buildPostParams.ids)
      packs.push_back(pack_id_pack_name(id));
    check_assets_base_up_to_date(packs, buildPostParams.checkTex, buildPostParams.checkRes);
  }

  if (!pendingBuildQueue.empty())
  {
    DaBuildPostParams next = eastl::move(pendingBuildQueue.front());
    pendingBuildQueue.erase(pendingBuildQueue.begin());
    finish_auto_jobs();
    launch_build(eastl::move(next));
    wasDabuildRunning = true;
    return;
  }

  currentlyBuildingMap.clear();
  finish_auto_jobs();

  bool ok = (status == 2);
  bool shouldHideConsole = (ok == buildPostParams.hideConsoleOnSuccess);
  if (shouldHideConsole && !buildPostParams.consoleWasOpen)
    ::get_app().getConsole().hideConsole();
}

static String dabuildCmdLine;
static Tab<SimpleString> dabuildArgv;

static String target_code_to_str(unsigned code)
{
  char chars[5] = {0};
  int len = 0;
  for (int shift = 24; shift >= 0; shift -= 8)
  {
    char c = (char)((code >> shift) & 0xFF);
    if (c)
      chars[len++] = c;
  }
  for (int i = 0, j = len - 1; i < j; i++, j--)
  {
    char tmp = chars[i];
    chars[i] = chars[j];
    chars[j] = tmp;
  }
  return String(0, "%s", chars);
}

static String find_dabuild_exe()
{
#if _TARGET_PC_WIN
  static const char *names[] = {"daBuild-dev.exe", "daBuild-rel.exe", "daBuild-dbg.exe"};
#else
  static const char *names[] = {"daBuild-dev", "daBuild-rel", "daBuild-dbg"};
#endif
  for (const char *name : names)
  {
    String path(0, "%s/%s", startDir.str(), name);
    if (dd_file_exist(path))
      return path;
  }
  return String();
}

static void classify_output_line(const char *line, ILogWriter::MessageType &out_type, const char *&out_msg)
{
  if (line[0] == '*')
  {
    out_type = ILogWriter::NOTE;
    out_msg = line + 1;
  }
  else if (strncmp(line, "WARNING: ", 9) == 0)
  {
    out_type = ILogWriter::WARNING;
    out_msg = line + 9;
  }
  else if (strncmp(line, "ERROR: ", 7) == 0)
  {
    out_type = ILogWriter::ERROR;
    out_msg = line + 7;
  }
  else if (strncmp(line, "-FATAL-: ", 9) == 0)
  {
    out_type = ILogWriter::FATAL;
    out_msg = line + 9;
  }
  else
  {
    out_type = ILogWriter::NOTE;
    out_msg = line;
  }
}

template <typename ReadFn>
static void drain_pipe_to_log(ReadFn read_fn)
{
  Tab<char> lineBuf;
  char readBuf[4096];
  int n;
  while ((n = read_fn(readBuf, (int)sizeof(readBuf))) > 0)
  {
    for (int i = 0; i < n; i++)
    {
      char c = readBuf[i];
      if (c == '\r')
        continue;
      if (c == '\n')
      {
        lineBuf.push_back('\0');
        const char *line = lineBuf.data();
        if (*line)
        {
          ILogWriter::MessageType type;
          const char *msg;
          classify_output_line(line, type, msg);
          WinAutoLock lock(logMutex);
          pendingLogs.push_back({type, String(0, "%s", msg)});
        }
        lineBuf.clear();
      }
      else
        lineBuf.push_back(c);
    }
  }
  // Flush any partial line without trailing newline
  if (!lineBuf.empty())
  {
    lineBuf.push_back('\0');
    const char *line = lineBuf.data();
    if (*line)
    {
      ILogWriter::MessageType type;
      const char *msg;
      classify_output_line(line, type, msg);
      WinAutoLock lock(logMutex);
      pendingLogs.push_back({type, String(0, "%s", msg)});
    }
  }
}

static bool launch_build(DaBuildPostParams postParams)
{
  int effectiveJobs = autoJobs ? compute_auto_jobs(postParams) : dabuildJobs;
  if (autoJobs)
    autoJobsActive = true;
  if (effectiveJobs == 0)
    return launch_build_inproc_async(eastl::move(postParams));
  int savedJobs = dabuildJobs;
  dabuildJobs = effectiveJobs;
  bool result = launch_build_async(eastl::move(postParams));
  dabuildJobs = savedJobs;
  return result;
}

static bool launch_build_inproc_async(DaBuildPostParams postParams)
{
  if (!dabuild || !assetMgr)
    return false;

  if (interlocked_relaxed_load(dabuildStatus) != 0)
  {
    ::get_app().getConsole().addMessage(ILogWriter::WARNING, "daBuild: build already in progress, request ignored");
    return false;
  }

  buildPostParams = eastl::move(postParams);
  {
    WinAutoLock lock(logMutex);
    pendingLogs.clear();
  }

  if (!buildPostParams.consoleWasOpen)
    ::get_app().getConsole().showConsole();

  ::get_app().getConsole().addMessage(ILogWriter::NOTE, "daBuild: in-process build (0 jobs)");

  dabuildProgress = DaBuildProgress{};
  currentBuildUsesJobs = false;
  interlocked_relaxed_store(inprocBuildCancelled, false);
  interlocked_relaxed_store(inprocThreadDone, false);
  interlocked_exchange(dabuildStatus, 1);

  execute_in_new_thread(
    [](auto) {
      ThreadedLogWriter log;
      dabuild->setupReports(&log, &inprocPbar);

      bool cancelled = false;
      bool ok = true;
      for (uint64_t id : buildPostParams.ids)
      {
        if (interlocked_relaxed_load(inprocBuildCancelled))
        {
          cancelled = true;
          break;
        }
        uint32_t pkgIdx = pack_id_pkg_idx(id);
        String selector(0, "\1%s", (pkgIdx == 0) ? DABUILD_BASE_PKG_SELECTOR : pack_id_pkg_name(id));
        const char *packName = pack_id_pack_name(id);
        const char *packList[] = {selector.c_str(), packName};
        if (!dabuild->exportPacks(buildPostParams.tc, make_span_const(packList, 2)))
          ok = false;
      }

      if (!cancelled)
        cancelled = interlocked_relaxed_load(inprocBuildCancelled);

      dabuild->setupReports(nullptr, nullptr);
      interlocked_relaxed_store(inprocThreadDone, true);

      if (cancelled)
        interlocked_exchange(dabuildStatus, 4);
      else
        interlocked_exchange(dabuildStatus, ok ? 2 : 3);
    },
    "daBuildInprocThread");

  return true;
}

static bool launch_build_async(DaBuildPostParams postParams)
{
  if (!dabuild || !assetMgr)
    return false;

  if (interlocked_relaxed_load(dabuildStatus) != 0)
  {
    ::get_app().getConsole().addMessage(ILogWriter::WARNING, "daBuild: build already in progress, request ignored");
    return false;
  }

  String dabuildExe = find_dabuild_exe();
  if (dabuildExe.empty())
  {
    ::get_app().getConsole().addMessage(ILogWriter::ERROR, "daBuild: cannot find daBuild executable near %s", startDir.str());
    return false;
  }

  dabuildProgress = DaBuildProgress{};
  currentBuildUsesJobs = (dabuildJobs > 0);
  progressShmName.printf(0, "daBuild-progress-%u", get_process_uid());
  progressShm =
    (DaBuildProgressShm *)create_global_map_shared_mem(progressShmName, nullptr, sizeof(DaBuildProgressShm), progressShmHandle);
  if (progressShm)
    memset((void *)progressShm, 0, sizeof(DaBuildProgressShm));

  dabuildArgv.clear();
  dabuildArgv.push_back(SimpleString(dabuildExe.str()));
  dabuildArgv.push_back(SimpleString("-nopbar"));
  dabuildCmdLine.printf(0, "\"%s\" -nopbar", dabuildExe.str());
  if (patchBuildMode)
  {
    dabuildArgv.push_back(SimpleString("-patch_build"));
    dabuildCmdLine.aprintf(0, " %s", dabuildArgv.back());
  }
  if (progressShm)
  {
    String shmArg(0, "-progress_shm:%s", progressShmName.str());
    dabuildArgv.push_back(SimpleString(shmArg));
    dabuildCmdLine.aprintf(0, " %s", shmArg.str());
  }
  if (dabuildJobs > 0)
  {
    String jobsArg(0, "-jobs:%d", dabuildJobs);
    dabuildArgv.push_back(SimpleString(jobsArg));
    dabuildCmdLine.aprintf(0, " %s", jobsArg.str());
  }
  for (unsigned tc : postParams.tc)
  {
    String tcArg(0, "-target:%s", target_code_to_str(tc).str());
    dabuildArgv.push_back(SimpleString(tcArg));
    dabuildCmdLine.aprintf(0, " %s", tcArg.str());
  }
  if (!postParams.checkTex)
  {
    dabuildArgv.push_back(SimpleString("-only_res"));
    dabuildCmdLine.aprintf(0, " -only_res");
  }
  else if (!postParams.checkRes)
  {
    dabuildArgv.push_back(SimpleString("-only_tex"));
    dabuildCmdLine.aprintf(0, " -only_tex");
  }
  dabuildArgv.push_back(SimpleString(::get_app().getWorkspace().getAppBlkPath()));
  dabuildCmdLine.aprintf(0, " \"%s\"", ::get_app().getWorkspace().getAppBlkPath());
  FastNameMap pkgSelectors;
  for (uint64_t id : postParams.ids)
  {
    uint32_t pkgIdx = pack_id_pkg_idx(id);
    pkgSelectors.addNameId(String(0, "\1%s", (pkgIdx == 0) ? DABUILD_BASE_PKG_SELECTOR : pack_id_pkg_name(id)).c_str());
  }
  iterate_names(pkgSelectors, [](int, const char *sel) {
    dabuildArgv.push_back(SimpleString(sel));
    dabuildCmdLine.aprintf(0, " \"%s\"", sel);
  });
  for (uint64_t id : postParams.ids)
  {
    const char *packName = pack_id_pack_name(id);
    dabuildArgv.push_back(SimpleString(packName));
    dabuildCmdLine.aprintf(0, " \"%s\"", packName);
  }

  buildPostParams = eastl::move(postParams);
  {
    WinAutoLock lock(logMutex);
    pendingLogs.clear();
  }

  if (!buildPostParams.consoleWasOpen)
    ::get_app().getConsole().showConsole();

  ::get_app().getConsole().addMessage(ILogWriter::NOTE, "daBuild: %s", dabuildCmdLine.str());

  interlocked_exchange(dabuildStatus, 1);

  execute_in_new_thread(
    [](auto) {
#if _TARGET_PC_WIN
      SECURITY_ATTRIBUTES sa = {};
      sa.nLength = sizeof(SECURITY_ATTRIBUTES);
      sa.bInheritHandle = TRUE;

      HANDLE hReadPipe = NULL, hWritePipe = NULL;
      if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
      {
        WinAutoLock lock(logMutex);
        pendingLogs.push_back({ILogWriter::ERROR, String(0, "%s", "daBuild: CreatePipe failed")});
        interlocked_exchange(dabuildStatus, 3);
        return;
      }
      SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

      STARTUPINFOA si = {};
      si.cb = sizeof(STARTUPINFOA);
      si.hStdOutput = hWritePipe;
      si.hStdError = hWritePipe;
      si.dwFlags = STARTF_USESTDHANDLES;

      Tab<char> cmdBuf;
      int cmdLen = dabuildCmdLine.length();
      cmdBuf.resize(cmdLen + 1);
      memcpy(cmdBuf.data(), dabuildCmdLine.str(), cmdLen + 1);

      PROCESS_INFORMATION pi = {};
      bool launched =
        CreateProcessA(NULL, cmdBuf.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED, NULL, NULL, &si, &pi) != 0;
      CloseHandle(hWritePipe);

      if (!launched)
      {
        CloseHandle(hReadPipe);
        WinAutoLock lock(logMutex);
        pendingLogs.push_back({ILogWriter::ERROR, String(0, "%s", "daBuild: failed to start process")});
        interlocked_exchange(dabuildStatus, 3);
        return;
      }

      HANDLE jobObj = CreateJobObjectA(NULL, NULL);
      if (jobObj)
      {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(jobObj, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
        if (!AssignProcessToJobObject(jobObj, pi.hProcess))
        {
          CloseHandle(jobObj);
          jobObj = NULL;
        }
      }
      interlocked_exchange_ptr(dabuildJobObject, jobObj);
      interlocked_exchange_ptr(dabuildProcess, pi.hProcess);
      interlocked_exchange_ptr(dabuildReadPipe, hReadPipe);
      ResumeThread(pi.hThread);
      CloseHandle(pi.hThread);

      drain_pipe_to_log([&](char *buf, int size) -> int {
        DWORD bytesRead = 0;
        return ReadFile(hReadPipe, buf, (DWORD)size, &bytesRead, NULL) ? (int)bytesRead : 0;
      });

      HANDLE rp = (HANDLE)interlocked_exchange_ptr(dabuildReadPipe, (void *)nullptr);
      if (rp)
        CloseHandle(rp);

      HANDLE proc = (HANDLE)interlocked_exchange_ptr(dabuildProcess, (void *)nullptr);
      DWORD exitCode = 1;
      if (proc)
      {
        GetExitCodeProcess(proc, &exitCode);
        CloseHandle(proc);
      }

      HANDLE doneJobObj = (HANDLE)interlocked_exchange_ptr(dabuildJobObject, (void *)nullptr);
      if (doneJobObj)
        CloseHandle(doneJobObj);

      interlocked_exchange(dabuildStatus, exitCode == 0 ? 2 : 3);
#elif _TARGET_PC_LINUX || _TARGET_APPLE
      Tab<char *> argv;
      argv.reserve(dabuildArgv.size() + 1);
      for (SimpleString &s : dabuildArgv)
        argv.push_back((char *)s.str());
      argv.push_back(nullptr);

      int pipefd[2];
      if (pipe(pipefd) != 0)
      {
        WinAutoLock lock(logMutex);
        pendingLogs.push_back({ILogWriter::ERROR, String(0, "%s", "daBuild: pipe() failed")});
        interlocked_exchange(dabuildStatus, 3);
        return;
      }

      pid_t pid = fork();
      if (pid == -1)
      {
        close(pipefd[0]);
        close(pipefd[1]);
        WinAutoLock lock(logMutex);
        pendingLogs.push_back({ILogWriter::ERROR, String(0, "%s", "daBuild: fork() failed")});
        interlocked_exchange(dabuildStatus, 3);
        return;
      }
      if (pid == 0)
      {
        setpgid(0, 0);
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execvp(argv[0], argv.data());
        _exit(127);
      }

      close(pipefd[1]);
      interlocked_exchange(dabuildPid, (int)pid);
      interlocked_exchange(dabuildReadPipeFd, pipefd[0]);

      drain_pipe_to_log([&](char *buf, int size) -> int { return (int)read(pipefd[0], buf, (size_t)size); });

      int rfd = interlocked_exchange(dabuildReadPipeFd, -1);
      if (rfd >= 0)
        close(rfd);

      int waitedPid = interlocked_exchange(dabuildPid, -1);
      int exitCode = 1;
      if (waitedPid > 0)
      {
        int wstatus = 0;
        if (waitpid((pid_t)waitedPid, &wstatus, 0) >= 0)
          exitCode = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
      }
      interlocked_exchange(dabuildStatus, exitCode == 0 ? 2 : 3);
#else
      interlocked_exchange(dabuildStatus, 3);
#endif
    },
    "daBuildThread");

  return true;
}

void post_base_update_notify_dabuild()
{
  dabuildcache::post_base_update_notify();
  if (cacheChecker.packs.nameCount() && !is_dabuild_running())
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


static void add_asset_id(Tab<uint64_t> &ids, DagorAsset &a)
{
  String pn = dabuild->getPackName(&a);
  if (pn.empty())
    return;
  String pkg = dabuild->getPkgName(&a);
  uint64_t id = make_pack_id(pkg.empty() ? BASE_PKG_NAME : pkg.str(), pn);
  if (id != INVALID_PACK_ID && eastl::find(ids.begin(), ids.end(), id) == ids.end())
    ids.push_back(id);
}

static void getPackForFolder(Tab<uint64_t> &ids, dag::ConstSpan<int> folders_idx, bool tex, bool res)
{
  if (!assetMgr || !dabuild)
    return;

  int tex_tid = assetMgr->getTexAssetTypeId();
  for (int i = 0; i < (int)folders_idx.size(); i++)
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
      add_asset_id(ids, a);
    }
  }
}

String get_asset_pack_name(DagorAsset *asset) { return dabuild ? dabuild->getPackName(asset) : String(); }
String get_asset_pkg_name(DagorAsset *asset) { return dabuild ? dabuild->getPkgName(asset) : String(); }

static bool do_check_assets_base_up_to_date(dag::ConstSpan<const char *> packs, bool quick)
{
  if (!dabuild)
    return false;

  if (interlocked_relaxed_load(dabuildStatus) == 1) // export still running
    return false;

  if (!av_perform_uptodate_check)
    return true;

  int64_t startTime = profile_ref_ticks();

  ILogWriter *log = &::get_app().getConsole();
  const char *quickSuffix = quick ? " (quick)" : "";

  log->addMessage(ILogWriter::NOTE, "checking assets up-to-date%s", quickSuffix);

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

      log->addMessage(ILogWriter::ERROR, "checking assets up-to-date%s...failed!", quickSuffix);
      return false;
    }
  }

  G_ASSERT(platforms.size() == platformFlags.size());

  int readyPacks = 0, totalPacks = 0, removedCacheFiles = 0, workerThreads = 0;
  bool ret = quick ? dabuild->quickCheckUpToDate(platforms, make_span(platformFlags), packs, //
                       readyPacks, totalPacks, removedCacheFiles, workerThreads)
                   : dabuild->checkUpToDate(platforms, make_span(platformFlags), packs);

  dabuild->setupReports(NULL, NULL);

  if (quick)
    log->addMessage(ILogWriter::NOTE,
      "checking assets up-to-date%s...complete in %.2fs using %d threads "
      "(%d up-to-date packs of %d total; %d stale cachefiles removed)",
      quickSuffix, ((float)profile_time_usec(startTime)) / 1e6f, workerThreads, readyPacks, totalPacks, removedCacheFiles);
  else
    log->addMessage(ILogWriter::NOTE, "checking assets up-to-date%s...complete in %.2fs", quickSuffix,
      ((float)profile_time_usec(startTime)) / 1e6f);

  EDITORCORE->updateViewports();
  EDITORCORE->invalidateViewportCache();
  ::get_app().afterUpToDateCheck(ret);

  return ret;
}

bool check_assets_base_up_to_date(dag::ConstSpan<const char *> packs, [[maybe_unused]] bool tex, [[maybe_unused]] bool res)
{
  TIME_PROFILE(check_assets_base_up_to_date);
  return do_check_assets_base_up_to_date(packs, false);
}

bool quick_check_assets_base_up_to_date(dag::ConstSpan<const char *> packs, [[maybe_unused]] bool tex, [[maybe_unused]] bool res)
{
  TIME_PROFILE(quick_check_assets_base_up_to_date);
  return do_check_assets_base_up_to_date(packs, true);
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

  DaBuildPostParams params;
  params.consoleWasOpen = ::get_app().getConsole().isVisible();
  params.hideConsoleOnSuccess = true;
  params.checkTex = tex;
  params.checkRes = res;
  params.emptyPacksForUpToDateCheck = false;
  getPackForFolder(params.ids, folders_idx, tex, res);
  params.tc.assign(tc.begin(), tc.end());

  launch_build(eastl::move(params));
}

void rebuild_assets_in_root(dag::ConstSpan<unsigned> tc, bool build_tex, bool build_res)
{
  if (!dabuild || !assetMgr)
    return;

  int tex_tid = assetMgr->getTexAssetTypeId();
  DaBuildPostParams params;
  params.consoleWasOpen = ::get_app().getConsole().isVisible();
  params.hideConsoleOnSuccess = false;
  params.checkTex = build_tex;
  params.checkRes = build_res;
  params.emptyPacksForUpToDateCheck = true;
  for (int j = 0; j < assetMgr->getAssetCount(); j++)
  {
    DagorAsset &a = assetMgr->getAsset(j);
    if (a.getFileNameId() < 0)
      continue;
    if (a.getType() == tex_tid && !build_tex)
      continue;
    if (a.getType() != tex_tid && !build_res)
      continue;
    add_asset_id(params.ids, a);
  }
  params.tc.assign(tc.begin(), tc.end());

  launch_build(eastl::move(params));
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

  DaBuildPostParams params;
  params.consoleWasOpen = ::get_app().getConsole().isVisible();
  params.hideConsoleOnSuccess = true;
  params.checkTex = true;
  params.checkRes = true;
  params.emptyPacksForUpToDateCheck = false;

  currentlyBuildingMap.clear();

  for (DagorAsset *a : assets)
    add_asset_id(params.ids, *a);
  params.tc.assign(tc.begin(), tc.end());

  for (auto &id : params.ids)
    currentlyBuildingMap[id].assign(tc.begin(), tc.end());

  launch_build(eastl::move(params));
}

bool is_asset_exportable(DagorAsset *a)
{
  return a && a->getFileNameId() >= 0 && dabuild->isAssetExportable(a) && !::get_asset_pack_name(a).empty();
}

static dag::ConstSpan<unsigned> get_pack_building_tcs(uint64_t pack_id)
{
  auto it = currentlyBuildingMap.find(pack_id);
  if (it == currentlyBuildingMap.end())
    return {};
  return make_span_const(it->second);
}

static bool is_tc_building(dag::ConstSpan<unsigned> buildingTcs, unsigned tc)
{
  for (unsigned b : buildingTcs)
    if (b == tc)
      return true;
  return false;
}

void render_dabuild_imgui()
{
  if (pendingBringToFront)
  {
    pendingBringToFront = false;
    ImGuiWindow *win = ImGui::FindWindowByName(daBuildPanelName);
    if (win)
      activate_dabuild_window();
    else
      ImGui::SetNextWindowFocus();
  }

  DAEDITOR3.imguiBegin(daBuildPanelName, &dabuildWindowVisible, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
  {
    bool running = ::is_dabuild_running();

    if (::get_dabuild_max_jobs() > 0)
    {
      const float inputWidth = ImGui::CalcTextSize("999").x + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetFrameHeight() * 2.0f;
      ImGui::SetNextItemWidth(inputWidth);
      if (autoJobs)
      {
        ImGui::BeginDisabled();
        int displayJobs = ::get_dabuild_jobs();
        ImGui::InputInt("Jobs", &displayJobs, 1, 1);
        ImGui::EndDisabled();
      }
      else
      {
        int jobs = ::get_dabuild_jobs();
        if (ImGui::InputInt("Jobs", &jobs, 1, 1))
          ::set_dabuild_jobs(jobs);
      }
      ImGui::SameLine();
      if (autoJobsActive)
        ImGui::BeginDisabled();
      ImGui::Checkbox("Auto", &autoJobs);
      ImGui::SetItemTooltip("Auto-optimizing pack exports");
      if (autoJobsActive)
        ImGui::EndDisabled();
      ImGui::SameLine();
    }

    {
      const float exportWidth = ImGui::CalcTextSize("Export").x + ImGui::GetStyle().FramePadding.x * 2.0f;
      const float stopWidth = ImGui::CalcTextSize("Stop").x + ImGui::GetStyle().FramePadding.x * 2.0f;
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - stopWidth - exportWidth - 10.f);

      if (running)
      {
        ImGui::BeginDisabled();
        ImGui::Button("Export");
        ImGui::EndDisabled();

        ImGui::SameLine(0.0f, 10.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.08f, 0.08f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.40f, 0.05f, 0.05f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        if (ImGui::Button("Stop"))
          ::stop_dabuild_background();
        ImGui::PopStyleColor(4);
      }
      else
      {
        if (ImGui::Button("Export"))
          export_queue();

        ImGui::SameLine(0.0f, 10.0f);

        ImGui::BeginDisabled();
        ImGui::Button("Stop");
        ImGui::EndDisabled();
      }
    }

    if (!running)
      ImGui::BeginDisabled();

    DaBuildProgress progress = running ? ::get_dabuild_progress() : DaBuildProgress{};
    bool jobMode = running && ::get_dabuild_current_build_uses_jobs();
    const char *phaseLabel = progress.phase == 1 ? "Tex" : progress.phase == 2 ? "Res" : "Pack";
    static const char spinChars[] = "|/-\\";
    char spin = running ? spinChars[(int)(ImGui::GetTime() * 6.0f) % 4] : ' ';
    if (jobMode)
      ImGui::Text("%s %d / %d (dispatched) %c", phaseLabel, progress.packDone, progress.packTotal, spin);
    else
      ImGui::Text("%s %d / %d   %c", phaseLabel, progress.packDone, progress.packTotal, spin);
    float packFrac = progress.packTotal > 0 ? (float)progress.packDone / (float)progress.packTotal : 0.0f;
    ImGui::ProgressBar(packFrac, ImVec2(-1.0f, 0.0f));
    if (jobMode)
      ImGui::TextDisabled("no per-asset tracking in job mode");
    else
    {
      float assetFrac = progress.assetTotal > 0 ? (float)progress.assetDone / (float)progress.assetTotal : 0.0f;
      ImGui::ProgressBar(assetFrac, ImVec2(-1.0f, 0.0f));
    }

    if (!running)
      ImGui::EndDisabled();

    ImGui::Checkbox("Bring to front on build", &daBuildWindowAutoOpen);
    ImGui::SetItemTooltip("Brings the panel to the front when build starts.");
    ImGui::SameLine(0.0f, 10.0f);
    ImGui::Checkbox("Restore on finish", &daBuildWindowAutoClose);
    ImGui::SetItemTooltip("Restore the panel's previous state.\nHave no effect if bringing to front is not set.");

    ImGui::Separator();

    {
      const ImVec2 fontSizedIconSize = PropPanel::ImguiHelper::getFontSizedIconSize();
      const ImVec2 filterButtonSize = PropPanel::ImguiHelper::getImageButtonWithDownArrowSize(fontSizedIconSize);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - filterButtonSize.x);
      PropPanel::ImguiHelper::searchInput(&packSearchFocusId, "##packSearch", "Filter packs", packSearchText, searchIconId,
        clearIconId);
      ImGui::SameLine();
      const bool anyFilterActive = !packFilterTex || !packFilterRes || packFilterSelected;
      const PropPanel::IconId filterIcon = anyFilterActive ? filterActiveIconId : filterDefaultIconId;
      if (PropPanel::ImguiHelper::imageButtonWithArrow("packFilter", filterIcon, fontSizedIconSize, packFilterPopupOpen))
      {
        ImGui::OpenPopup("##packFilterPopup");
        packFilterPopupOpen = true;
      }
      ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
      if (ImGui::BeginPopup("##packFilterPopup", ImGuiWindowFlags_NoMove))
      {
        ImGui::TextUnformatted("Select filter");
        PropPanel::ImguiHelper::checkboxWithDragSelection("tex", &packFilterTex);
        PropPanel::ImguiHelper::checkboxWithDragSelection("res", &packFilterRes);
        PropPanel::ImguiHelper::checkboxWithDragSelection("selected", &packFilterSelected);
        ImGui::EndPopup();
      }
      else
        packFilterPopupOpen = false;
      ImGui::PopStyleColor();
    }

    if (running)
      ImGui::BeginDisabled();

    float deselectAllWidth = ImGui::CalcTextSize("Deselect all").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    float selectAllWidth = ImGui::CalcTextSize("Select all").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    float rightButtonsWidth = deselectAllWidth + ImGui::GetStyle().ItemSpacing.x + selectAllWidth;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - rightButtonsWidth);

    if (ImGui::Button("Deselect all"))
      queue_remove_all();
    ImGui::SameLine();
    if (ImGui::Button("Select all"))
      queue_select_all_known_packs();

    dag::ConstSpan<unsigned> additionalPlatforms = ::get_app().getWorkspace().getAdditionalPlatforms();
    const bool multiPlatform = !additionalPlatforms.empty();

    static constexpr float LIST_PAD = 2.0f;
    static constexpr float ITEM_PAD = 4.0f;
    static constexpr float PKG_PAD = 4.0f;

    Tab<unsigned> displayPlatforms;
    if (multiPlatform)
    {
      displayPlatforms.push_back(_MAKE4C('PC'));
      for (unsigned ptc : additionalPlatforms)
        displayPlatforms.push_back(ptc);
    }
    const int numPlatSeg = (int)displayPlatforms.size();

    float platformSegW = 0.0f;
    if (multiPlatform)
    {
      for (unsigned ptc : displayPlatforms)
      {
        char buf[8];
        snprintf(buf, sizeof(buf), "[%c%c%c%c]", _DUMP4C(ptc));
        float w = ImGui::CalcTextSize(buf).x + 2.0f * ITEM_PAD;
        if (w > platformSegW)
          platformSegW = w;
      }
    }

    const ImVec4 colSelected = PropPanel::getOverriddenColor(PropPanel::ColorOverride::LISTBOX_SELECTION_BACKGROUND);
    const ImVec4 colHovered = PropPanel::getOverriddenColor(PropPanel::ColorOverride::LISTBOX_HIGHLIGHT_BACKGROUND_HOVERED);
    const ImVec4 colBuilding = ImVec4(0.85f, 0.55f, 0.10f, 1.0f);

    const float lineH = ImGui::GetFrameHeight();

    String searchLower = packSearchText;
    dd_strlwr(searchLower);

    auto passesFilter = [&](uint64_t pack_id) -> bool {
      const char *packName = pack_id_pack_name(pack_id);
      if (packFilterSelected && queue_get_pack_tcs(pack_id).empty())
        return false;
      bool isTex = trail_strcmp(packName, ".dxp.bin");
      if (isTex && !packFilterTex)
        return false;
      if (!isTex && !packFilterRes)
        return false;
      if (searchLower.empty())
        return true;
      String packLower = String(packName);
      dd_strlwr(packLower);
      return strstr(packLower.str(), searchLower.str()) != nullptr;
    };

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(LIST_PAD, LIST_PAD));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    bool childOpen = ImGui::BeginChild("##packList", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    if (childOpen)
    {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

      const float availW = ImGui::GetContentRegionAvail().x;
      const float bodyW = availW - numPlatSeg * platformSegW;

      ImDrawList *dl = ImGui::GetWindowDrawList();


      int rowIdx = 0;
      auto drawPackRow = [&](uint64_t pack_id) {
        const char *displayName = pack_id_pack_name(pack_id);
        dag::ConstSpan<unsigned> selectedTcs = queue_get_pack_tcs(pack_id);
        dag::ConstSpan<unsigned> buildingTcs = get_pack_building_tcs(pack_id);
        bool isQueued = !selectedTcs.empty();
        bool anyBuilding = !buildingTcs.empty();

        ImVec2 rowMin = ImGui::GetCursorScreenPos();
        ImVec2 rowMax = ImVec2(rowMin.x + availW, rowMin.y + lineH);

        int hoveredSeg = -1;
        if (!running)
        {
          float relX = ImGui::GetIO().MousePos.x - rowMin.x;
          if (ImGui::IsMouseHoveringRect(rowMin, rowMax))
          {
            if (!multiPlatform || relX < bodyW)
              hoveredSeg = 0;
            else
            {
              int ci = (int)((relX - bodyW) / platformSegW);
              if (ci >= 0 && ci < numPlatSeg)
                hoveredSeg = ci + 1;
            }
          }
        }

        {
          float bxEnd = multiPlatform ? rowMin.x + bodyW : rowMax.x;
          if (anyBuilding)
            dl->AddRectFilled(rowMin, ImVec2(bxEnd, rowMax.y), ImGui::GetColorU32(colBuilding));
          else if (isQueued)
            dl->AddRectFilled(rowMin, ImVec2(bxEnd, rowMax.y), ImGui::GetColorU32(colSelected));
          if (hoveredSeg == 0)
            dl->AddRectFilled(rowMin, ImVec2(bxEnd, rowMax.y), ImGui::GetColorU32(colHovered));
        }

        if (multiPlatform)
        {
          for (int ti = 0; ti < numPlatSeg; ++ti)
          {
            unsigned ptc = displayPlatforms[ti];
            bool tcSel = false;
            for (unsigned s : selectedTcs)
              if (s == ptc)
              {
                tcSel = true;
                break;
              }

            float cx = rowMin.x + bodyW + ti * platformSegW;
            float cxE = cx + platformSegW;

            if (is_tc_building(buildingTcs, ptc))
              dl->AddRectFilled(ImVec2(cx, rowMin.y), ImVec2(cxE, rowMax.y), ImGui::GetColorU32(colBuilding));
            else if (tcSel)
              dl->AddRectFilled(ImVec2(cx, rowMin.y), ImVec2(cxE, rowMax.y), ImGui::GetColorU32(colSelected));

            if (hoveredSeg == ti + 1)
              dl->AddRectFilled(ImVec2(cx, rowMin.y), ImVec2(cxE, rowMax.y), ImGui::GetColorU32(colHovered));
          }
        }

        ImGui::SetCursorScreenPos(rowMin);
        char btnId[64];
        snprintf(btnId, sizeof(btnId), "##qrow%d", rowIdx++);
        bool clicked = false;
        if (!running)
          clicked = ImGui::InvisibleButton(btnId, ImVec2(availW, lineH));
        else
          ImGui::Dummy(ImVec2(availW, lineH));

        if (clicked)
        {
          if (!multiPlatform)
          {
            queue_toggle_pack(pack_id, _MAKE4C('PC'));
          }
          else if (hoveredSeg >= 1)
          {
            queue_toggle_pack(pack_id, displayPlatforms[hoveredSeg - 1]);
          }
          else
          {
            if (isQueued)
            {
              auto it = queueMap.find(pack_id);
              if (it != queueMap.end())
                it->second.tcs.clear();
            }
            else
              queue_add_pack_all_platforms(pack_id);
          }
        }

        const float textY = rowMin.y + (lineH - ImGui::GetTextLineHeight()) * 0.5f;
        const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);

        {
          float clipRight = (multiPlatform ? rowMin.x + bodyW : rowMax.x) - ITEM_PAD;
          dl->PushClipRect(rowMin, ImVec2(clipRight, rowMax.y), true);
          dl->AddText(ImVec2(rowMin.x + ITEM_PAD, textY), textCol, displayName);
          dl->PopClipRect();
        }

        if (multiPlatform)
        {
          for (int ti = 0; ti < numPlatSeg; ++ti)
          {
            char chipLabel[8];
            snprintf(chipLabel, sizeof(chipLabel), "[%c%c%c%c]", _DUMP4C(displayPlatforms[ti]));
            float cx = rowMin.x + bodyW + ti * platformSegW + ITEM_PAD;
            dl->AddText(ImVec2(cx, textY), textCol, chipLabel);
          }
        }
      };

      for (uint32_t pi = 0; pi < (uint32_t)knownPkgGroups.size(); ++pi)
      {
        const PkgGroup &pkg = knownPkgGroups[pi];
        int visible = 0;
        for (uint32_t ci = 0; ci < (uint32_t)pkg.packs.size(); ++ci)
          if (passesFilter(((uint64_t)pi << 32) | ci))
            ++visible;
        if (visible == 0)
          continue;

        ImGui::Dummy(ImVec2(0, PKG_PAD));
        ImGui::BeginDisabled();
        const ImVec2 item_spacing = ImGui::GetStyle().ItemSpacing;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.f, item_spacing.y));

        ImGui::SeparatorText(pkg.name.str());
        ImGui::PopStyleVar();
        ImGui::EndDisabled();
        ImGui::Dummy(ImVec2(0, PKG_PAD));

        for (uint32_t ci = 0; ci < (uint32_t)pkg.packs.size(); ++ci)
        {
          uint64_t pack_id = ((uint64_t)pi << 32) | ci;
          if (passesFilter(pack_id))
            drawPackRow(pack_id);
        }
      }

      ImGui::PopStyleVar();
    }
    ImGui::EndChild();

    if (running)
      ImGui::EndDisabled();
  }
  DAEDITOR3.imguiEnd();

  if (::is_dabuild_running())
  {
    static const float PAD = 10.0f;
    const ImGuiViewport *ov = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(ov->WorkPos.x + PAD, ov->WorkPos.y + ov->WorkSize.y - PAD), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.75f);
    constexpr ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                              ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                              ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    if (ImGui::Begin("##dabuildOverlay", nullptr, overlayFlags))
    {
      static const char *spinnerFrames[] = {"|", "/", "-", "\\"};
      int frame = (int)(ImGui::GetTime() * 8.0) & 3;
      ImGui::TextUnformatted(spinnerFrames[frame]);
      ImGui::SameLine();
      ImGui::TextUnformatted("daBuild in progress...");
    }
    ImGui::End();
  }
}
