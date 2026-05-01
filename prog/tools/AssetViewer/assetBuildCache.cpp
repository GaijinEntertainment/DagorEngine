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
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_sharedMem.h>
#include <osApiWrappers/dag_miscApi.h>
#include <assets/daBuildProgressShm.h>
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
extern bool av_perform_uptodate_check;
extern bool dabuildWindowVisible;
extern bool daBuildWindowAutoOpen;
extern bool daBuildWindowAutoClose;
static bool patchBuildMode = false;
static bool wasDabuildRunning = false; // for detecting changes

struct PendingLogEntry
{
  ILogWriter::MessageType type;
  String text;
};

static WinCritSec logMutex;
static Tab<PendingLogEntry> pendingLogs;

// 0 = idle, 1 = running, 2 = done-success, 3 = done-failed
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
  Tab<SimpleString> packs;
  Tab<unsigned> tc;
  bool checkTex = true;
  bool checkRes = true;
  bool emptyPacksForUpToDateCheck = false;
  bool consoleWasOpen = false;
  bool hideConsoleOnSuccess = true;
};
static DaBuildPostParams buildPostParams;


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
  texconvcache::term();
  dabuildcache::term();
  if (assetMgr)
    assetMgr->unsubscribeBaseUpdateNotify(&cacheChecker);
  assetMgr = NULL;
}

int bind_dabuild_cache_with_mgr(DagorAssetMgr &mgr, DataBlock &appblk, const char *appdir)
{
  maxJobs = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getInt("maxJobs", 32);
  int pcount = dabuildcache::bind_with_mgr(mgr, appblk, appdir);
  dabuild = pcount ? dabuildcache::get_dabuild() : NULL;
  assetMgr = pcount ? &mgr : NULL;
  patchBuildMode = appblk.getBool("av_patch_build", true);
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

bool is_dabuild_running() { return interlocked_relaxed_load(dabuildStatus) == 1; }

void stop_dabuild_background()
{
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

void update_dabuild_background(PropPanel::IMenu *mm)
{
  bool isDabuildRunning = is_dabuild_running();
  if (wasDabuildRunning != isDabuildRunning)
  {
    if (isDabuildRunning)
    {
      if (daBuildWindowAutoOpen)
        dabuildWindowVisible = true;
    }
    else
    {
      if (daBuildWindowAutoClose)
        dabuildWindowVisible = false;
    }

    wasDabuildRunning = isDabuildRunning;
  }

  if (mm)
    mm->setCheckById(CM_WINDOW_DABUILD, dabuildWindowVisible); // catching both auto open/close and [X] closings

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

  int status = interlocked_acquire_load(dabuildStatus);
  if (status < 2) // idle or still running
    return;

  bool ok = (status == 2);
  interlocked_exchange(dabuildStatus, 0);
  close_progress_shm();

  bool shouldHideConsole = (ok == buildPostParams.hideConsoleOnSuccess);
  if (shouldHideConsole && !buildPostParams.consoleWasOpen)
    ::get_app().getConsole().hideConsole();

  if (buildPostParams.emptyPacksForUpToDateCheck)
    check_assets_base_up_to_date({}, buildPostParams.checkTex, buildPostParams.checkRes);
  else
  {
    Tab<const char *> packs;
    for (const SimpleString &s : buildPostParams.packs)
      packs.push_back(s.str());
    check_assets_base_up_to_date(packs, buildPostParams.checkTex, buildPostParams.checkRes);
  }
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
  for (const SimpleString &pack : postParams.packs)
  {
    dabuildArgv.push_back(SimpleString(pack.str()));
    dabuildCmdLine.aprintf(0, " \"%s\"", pack.str());
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
      if (String pn = dabuild->getPackName(&a); !pn.empty())
        packs.addNameId(pn);
    }
  }
}

String get_asset_pack_name(DagorAsset *asset) { return dabuild ? dabuild->getPackName(asset) : String(); }
String get_asset_pkg_name(DagorAsset *asset) { return dabuild ? dabuild->getPkgName(asset) : String(); }

bool check_assets_base_up_to_date(dag::ConstSpan<const char *> packs, [[maybe_unused]] bool tex, [[maybe_unused]] bool res)
{
  TIME_PROFILE(check_assets_base_up_to_date);
  if (!dabuild)
    return false;

  if (interlocked_relaxed_load(dabuildStatus) == 1) // export still running
    return false;

  if (!av_perform_uptodate_check)
    return true;

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

  FastNameMap _packs;
  getPackForFolder(_packs, folders_idx, tex, res);

  DaBuildPostParams params;
  params.consoleWasOpen = ::get_app().getConsole().isVisible();
  params.hideConsoleOnSuccess = true;
  params.checkTex = tex;
  params.checkRes = res;
  params.emptyPacksForUpToDateCheck = false;
  iterate_names(_packs, [&](int, const char *name) { params.packs.push_back(SimpleString(name)); });
  params.tc.assign(tc.begin(), tc.end());

  launch_build_async(eastl::move(params));
}

void rebuild_assets_in_root(dag::ConstSpan<unsigned> tc, bool build_tex, bool build_res)
{
  if (!dabuild || !assetMgr)
    return;

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
    if (String pn = dabuild->getPackName(&a); !pn.empty())
      _packs.addNameId(pn);
  }

  DaBuildPostParams params;
  params.consoleWasOpen = ::get_app().getConsole().isVisible();
  params.hideConsoleOnSuccess = false;
  params.checkTex = build_tex;
  params.checkRes = build_res;
  params.emptyPacksForUpToDateCheck = true;
  iterate_names(_packs, [&](int, const char *name) { params.packs.push_back(SimpleString(name)); });
  params.tc.assign(tc.begin(), tc.end());

  launch_build_async(eastl::move(params));
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
  for (DagorAsset *a : assets)
    params.packs.push_back(SimpleString(dabuild->getPackName(a)));
  params.tc.assign(tc.begin(), tc.end());

  launch_build_async(eastl::move(params));
}

bool is_asset_exportable(DagorAsset *a)
{
  return a && a->getFileNameId() >= 0 && dabuild->isAssetExportable(a) && !::get_asset_pack_name(a).empty();
}

void render_dabuild_imgui()
{
  DAEDITOR3.imguiBegin("daBuild", &dabuildWindowVisible);
  {
    bool running = ::is_dabuild_running();

    if (::get_dabuild_max_jobs() > 0)
    {
      int jobs = ::get_dabuild_jobs();
      const float inputWidth = ImGui::CalcTextSize("999").x + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetFrameHeight() * 2.0f;
      ImGui::SetNextItemWidth(inputWidth);
      if (ImGui::InputInt("Jobs", &jobs, 1, 1))
        ::set_dabuild_jobs(jobs);
      ImGui::SameLine();
    }

    {
      const float stopWidth = ImGui::CalcTextSize("Stop").x + ImGui::GetStyle().FramePadding.x * 2.0f;
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - stopWidth);
      if (running)
      {
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

    ImGui::Checkbox("Auto open on build", &daBuildWindowAutoOpen);
    ImGui::SameLine(0.0f, 10.0f);
    ImGui::Checkbox("Auto close when finished", &daBuildWindowAutoClose);
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
