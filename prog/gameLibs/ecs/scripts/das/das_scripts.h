// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <EASTL/vector.h>
#include <osApiWrappers/dag_watchDir.h>
#include "das_loaded_script.h"
#include "das_file_access_container.h"
#include "das_ecs.h"
#include "das_scripts.h"

namespace bind_dascript
{
#ifndef DAS_HAS_DIRECTORY_WATCH
#if _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0
#define DAS_HAS_DIRECTORY_WATCH 1
#endif
#endif

struct LoadScriptCtx
{
  AotMode aotMode;
  ResolveECS resolveEcs;
  LogAotErrors logAotErrors;
  DasSyntax syntax;
  EnableDebugger enableDebugger;
  void *userData;
};

struct Scripts
{
  das::recursive_mutex mutex;
  ecs::EntityManager *mgr = nullptr;
#if DAS_HAS_DIRECTORY_WATCH
  struct DirWatchHandleDeleter
  {
    void operator()(WatchedFolderMonitorData *ptr) { destroy_folder_monitor(ptr, 10); }
  };
  using DirWatchHandlers = ska::flat_hash_map<eastl::string, eastl::unique_ptr<WatchedFolderMonitorData, DirWatchHandleDeleter>>;
  // for hot reload
  DirWatchHandlers dirsOpened;
  struct FileTrackingInfo
  {
    int64_t lastWriteTime;
    ska::flat_hash_set<eastl::string> scriptsToReload;
  };
  ska::flat_hash_map<eastl::string, FileTrackingInfo> changedFileToScripts;
#endif

  ska::flat_hash_map<eastl::string, LoadedScript> scripts;
  struct ScriptMemory
  {
    uint64_t heapSizeThreshold;
    uint64_t stringHeapSizeThreshold;
    uint64_t nextGcMsec;
  };
  ska::flat_hash_map<eastl::string, ScriptMemory> scriptsMemory;
  enum
  {
    MODULE_FS_ACCESS,
    SANDBOX_FS_ACCESS
  };
  DagFileAccessContainer<MODULE_FS_ACCESS> moduleFileAccess;
  DagFileAccessContainer<SANDBOX_FS_ACCESS> sandboxModuleFileAccess;

  // flags to turn on/off loading of some kind of files
  bool loadDebugCode = true;
  bool loadEvents = true;
  bool profileLoading = false;

  das::atomic<uint32_t> linkAotErrorsCount{0};
  das::atomic<int> compileErrorsCount{0}; //-1 if this value is unknown
  bool sandboxMode = false;
  bool storeTemporaryScripts = false;
  das::vector<eastl::string> temporaryScripts; // contains files loaded in game mode
  das::vector<das::FileInfoPtr> orphanFileInfos;


  DagFileAccess *getFileAccess() { return sandboxMode ? sandboxModuleFileAccess.get() : moduleFileAccess.get(); }

  void init() { debug("daScript: aot lib size=%@", das::getGlobalAotLibrary().size()); }

  void gatherDependencies(const das::string &fname, das::smart_ptr<DagFileAccess> access, das::ModuleGroup &lib_group);

  bool loadScript(const das::string &fname, das::smart_ptr<DagFileAccess> access, LoadScriptCtx ctx);

  bool loadScriptInternal(const das::string &fname, das::smart_ptr<DagFileAccess> access, LoadScriptCtx ctx);

  bool unloadScript(const char *fname, bool strict);

  void postProcessModuleGroups();

  void clearStatistics();


  void setSandboxMode(bool is_sandbox);

  void unloadTemporaryScript();

  ReloadResult reloadAllChanged(AotMode aot_mode, LogAotErrors log_aot_errors);

  void freeSourceData();

  bool postLoadScript(LoadedScript &script, const das::string &fname, uint64_t load_start_time);

  bool mainThreadDone();

  void cleanupMemoryUsage();

  LoadingContainer<das::DebugInfoHelper> helper;
  eastl::vector<QueryData> sharedQueries;
  DasEcsStatistics statistics; // thread safe data
#if DAGOR_DBGLEVEL > 0
  LoadingContainer<DebugArgStrings> argStrings; // To consider: `FastNameMapTS<false, SpinLockReadWriteLock>`
#endif

  void thisThreadDone();

  void processModuleGroupUserData(const das::string &fname, das::ModuleGroup &libGroup);

  int getPendingSystemsNum(das::ModuleGroup &group) const;

  int getPendingQueriesNum(das::ModuleGroup &group) const;

  void storeSharedQueries(das::ModuleGroup &group);

  bool postProcessModuleGroupUserData(const das::string &fname, das::ModuleGroup &group);

  bool keepModuleGroupUserData(const das::string &fname, const das::ModuleGroup &group) const;

  bool processLoadedScript(LoadedScript &script, const das::string &fname, uint64_t load_start_time, das::ModuleGroup &group,
    AotMode aot_mode, AotModeIsRequired aot_mode_is_required);

  void unloadAllScripts(UnloadDebugAgents unload_debug_agents);
  size_t dumpMemory();
};
} // namespace bind_dascript