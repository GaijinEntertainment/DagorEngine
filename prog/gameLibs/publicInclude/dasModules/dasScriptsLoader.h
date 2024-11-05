//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <daScript/ast/ast_serializer.h>
#include <osApiWrappers/dag_watchDir.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <ecs/scripts/dasEs.h>
#include <dasModules/dasFsFileAccess.h>
#include <startup/dag_globalSettings.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <perfMon/dag_perfTimer.h>
#include <EASTL/vector_set.h>


namespace bind_dascript
{
das::StackAllocator &get_shared_stack();

template <typename TContext>
struct DasLoadedScript
{
  using DagFileAccessPtr = das::smart_ptr<DagFileAccess>;
  das::ProgramPtr program;
  das::shared_ptr<TContext> ctx;
  eastl::unique_ptr<das::ModuleGroup> moduleGroup;
  union
  {
    struct
    {
      bool hasAot : 1;
      bool requireAot : 1;
      bool hasDebugger : 1;
      bool postProcessed : 1;
    };
    uint8_t flags = 0;
  };

  ska::flat_hash_map<eastl::string, int64_t, ska::power_of_two_std_hash<eastl::string>> filesOpened;
  das::smart_ptr<DagFileAccess> access;

  virtual void unload()
  {
    if (program)
      debug("DaScript: unload program");
    G_ASSERT(!ctx || ctx->insideContext == 0);
    ctx.reset();
    program = nullptr;
  }

  virtual ~DasLoadedScript() { unload(); }
  DasLoadedScript() {}
  DasLoadedScript(das::ProgramPtr &&program_, das::shared_ptr<TContext> &&ctx_, DagFileAccessPtr access_, AotMode aot_mode_override,
    AotModeIsRequired aot_mode_is_required, EnableDebugger enable_debugger) :
    program(eastl::move(program_)),
    ctx(eastl::move(ctx_)),
    access(access_),
    hasAot(aot_mode_override == AotMode::AOT),
    requireAot(aot_mode_is_required == AotModeIsRequired::YES),
    hasDebugger(enable_debugger == EnableDebugger::YES),
    postProcessed(false)
  {}
  DasLoadedScript &operator=(const DasLoadedScript &l) = delete;
  DasLoadedScript(const DasLoadedScript &l) = delete;
  DasLoadedScript(DasLoadedScript &&l) :
    ctx(eastl::move(l.ctx)),
    program(eastl::move(l.program)),
    moduleGroup(eastl::move(l.moduleGroup)),
    flags(eastl::move(l.flags)),
    filesOpened(eastl::move(l.filesOpened)),
    access(eastl::move(l.access))
  {}
  DasLoadedScript &operator=(DasLoadedScript &&l)
  {
    ctx = eastl::move(l.ctx);
    program = eastl::move(l.program);
    moduleGroup = eastl::move(l.moduleGroup);
    flags = l.flags;
    filesOpened = eastl::move(l.filesOpened);
    access = eastl::move(l.access);
    return *this;
  }
};


class DebugPrinter final : public das::TextWriter
{
public:
  virtual void output() override
  {
    uint64_t newPos = tellp();
    if (newPos != pos)
    {
      auto len = newPos - pos;
      debug("%.*s", len, data() + pos);
      pos = newPos;
    }
  }

protected:
  int pos = 0;
};

#ifndef DAS_HAS_DIRECTORY_WATCH
#if _TARGET_PC_WIN && DAGOR_DBGLEVEL > 0
#define DAS_HAS_DIRECTORY_WATCH 1
#endif
#endif

inline bool ends_with(const eastl::string_view &str, const char *suffix)
{
  const size_t suffixLen = strlen(suffix);
  return str.size() >= suffixLen && 0 == str.compare(str.size() - suffixLen, suffixLen, suffix);
}

inline bool ends_with_suffix(const das::string &fname, const char **suffixes_begin, const char **suffixes_end)
{
  for (const char **suffix = suffixes_begin; suffix < suffixes_end; ++suffix)
    if (ends_with(fname, *suffix))
      return true;
  return false;
}

template <class T, int ID = 0>
struct Cont
{
  das::mutex static mutex;
  DAS_THREAD_LOCAL static T value;
  eastl::vector<T> cache;

  ~Cont() { clear(); }

  void done()
  {
    das::lock_guard<das::mutex> guard(mutex);
    cache.emplace_back(eastl::move(value));
  }

  void clear() { cache.clear(); }
};

template <int ID = 0>
struct DagFileAccessCont
{
  Cont<das::smart_ptr<DagFileAccess>, ID> access;
  das::string dasProject;
  HotReload hotReloadMode = HotReload::DISABLED;

  void clear()
  {
    access.clear();
    access.value.reset();
  }

  void done()
  {
    access.done();
    access.value.reset();
  }

  DagFileAccess *get()
  {
    if (!access.value && !dasProject.empty())
      access.value = das::make_smart<DagFileAccess>(dasProject.c_str(), hotReloadMode);
    return access.value.get();
  }
  void freeSourceData()
  {
    if (access.value)
      access.value->freeSourceData();
    for (auto fa : access.cache)
      if (fa)
        fa->freeSourceData();
  }
};

template <class T, int id>
das::mutex Cont<T, id>::mutex;

template <class T, int id>
DAS_THREAD_LOCAL T Cont<T, id>::value;

template <typename TLoadedScript, typename TContext>
struct DasScripts
{
  static das::recursive_mutex mutex;
#if DAS_HAS_DIRECTORY_WATCH
  struct DirWatchHandleDeleter
  {
    void operator()(WatchedFolderMonitorData *ptr) { destroy_folder_monitor(ptr, 10); }
  };
  using DirWatchHandlers = ska::flat_hash_map<eastl::string, eastl::unique_ptr<WatchedFolderMonitorData, DirWatchHandleDeleter>>;
  // for hot reload
  DirWatchHandlers dirsOpened;
#endif
  ska::flat_hash_map<eastl::string, TLoadedScript> scripts;
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
  DagFileAccessCont<MODULE_FS_ACCESS> moduleFileAccess;
  DagFileAccessCont<SANDBOX_FS_ACCESS> sandboxModuleFileAccess;

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

  virtual void done()
  {
    moduleFileAccess.done();
    sandboxModuleFileAccess.done();
  }

  DagFileAccess *getFileAccess() { return sandboxMode ? sandboxModuleFileAccess.get() : moduleFileAccess.get(); }

  void init() { debug("daScript: aot lib size=%@", das::getGlobalAotLibrary().size()); }

  virtual void processModuleGroupUserData(const das::string &, das::ModuleGroup &) {}
  virtual bool postProcessModuleGroupUserData(const das::string &, das::ModuleGroup &) { return true; }
  virtual bool keepModuleGroupUserData(const das::string &, const das::ModuleGroup &) const { return false; }
  virtual bool processLoadedScript(TLoadedScript &, const das::string &, uint64_t, das::ModuleGroup &, AotMode, AotModeIsRequired)
  {
    return true;
  }
  virtual void storeSharedQueries(das::ModuleGroup &) {}
  virtual int getPendingSystemsNum(das::ModuleGroup &) const { return 0; }
  virtual int getPendingQueriesNum(das::ModuleGroup &) const { return 0; }

  void fillSharedModules(TLoadedScript &script, const das::string &fname, das::smart_ptr<DagFileAccess> access,
    das::ModuleGroup &lib_group);

  bool loadScript(const das::string &fname, das::smart_ptr<DagFileAccess> access, AotMode aot_mode, ResolveECS resolve_ecs,
    LogAotErrors log_aot_errors, EnableDebugger enable_debugger = EnableDebugger::NO)
  {
#if DAGOR_DBGLEVEL > 0
    if (profileLoading)
    {
      TIME_PROFILE_NAME(loading_profile, fname.c_str());
      return loadScriptInternal(fname, access, aot_mode, resolve_ecs, log_aot_errors, enable_debugger);
    }
    else
#endif
    {
      return loadScriptInternal(fname, access, aot_mode, resolve_ecs, log_aot_errors, enable_debugger);
    }
  }

  virtual bool loadScriptInternal(const das::string &fname, das::smart_ptr<DagFileAccess> access, AotMode aot_mode,
    ResolveECS resolve_ecs, LogAotErrors log_aot_errors, EnableDebugger enable_debugger);

  virtual bool unloadScript(const char *fname, bool strict)
  {
    das::lock_guard<das::recursive_mutex> guard(mutex);
    if (scripts.empty())
      return true; // no more scripts, exiting from game for example
    auto it = scripts.find_as(fname);
    if (it == scripts.end())
    {
      if (strict)
        logerr("Cannot unload '%s': the script not found", fname);
      return false;
    }

    if (it->second.ctx->persistent)
    {
      auto memIt = scriptsMemory.find_as(fname);
      if (memIt != scriptsMemory.end())
        scriptsMemory.erase(memIt);
    }

    debug("daScript: unload %s", fname);
    it->second.unload();
    scripts.erase(it);


    return true;
  }

  void postProcessModuleGroups()
  {
    das::lock_guard<das::recursive_mutex> guard(mutex);
    for (const auto &script : scripts)
    {
      if (script.second.moduleGroup)
        postProcessModuleGroupUserData(script.first, *script.second.moduleGroup);
    }
  }

  virtual void clearStatistics()
  {
    das::lock_guard<das::recursive_mutex> guard(mutex);
    linkAotErrorsCount = 0u;
    compileErrorsCount = 0;
  }

  virtual void unloadAllScripts(UnloadDebugAgents unload_debug_agents)
  {
    das::lock_guard<das::recursive_mutex> guard(mutex);
    scripts.clear();
    das::Module::Reset(/*reset_debug_agent*/ unload_debug_agents == UnloadDebugAgents::YES);
    clearStatistics();
    moduleFileAccess.clear();
    sandboxModuleFileAccess.clear();
    scriptsMemory.clear();
  }

  void setSandboxMode(bool is_sandbox)
  {
    das::lock_guard<das::recursive_mutex> guard(mutex);
    if (sandboxModuleFileAccess.dasProject.empty())
    {
      logerr("daScript: sandbox mode without restrictions! Not set das_project file. see init_sandbox");
      return;
    }
    sandboxMode = is_sandbox;
    storeTemporaryScripts = is_sandbox;
    unloadTemporaryScript();
  }

  void unloadTemporaryScript()
  {
    das::lock_guard<das::recursive_mutex> guard(mutex);
    for (auto &script : temporaryScripts)
      unloadScript(script.c_str(), /*strict*/ false);
    temporaryScripts.clear();
  }

  virtual ReloadResult reloadAllChanged(AotMode aot_mode, LogAotErrors log_aot_errors)
  {
    das::lock_guard<das::recursive_mutex> guard(mutex);
    DagFileAccess *globalFileAccess = !sandboxMode ? moduleFileAccess.get() : nullptr;
#if DAS_HAS_DIRECTORY_WATCH
    if (!eastl::any_of(dirsOpened.begin(), dirsOpened.end(), [&](auto &dir) { return could_be_changed_folder(dir.second.get()); }))
      return NO_CHANGES;
#endif
    ska::flat_hash_map<eastl::string, EnableDebugger, ska::power_of_two_std_hash<eastl::string>> changed;
    for (auto &script : scripts)
    {
      if (script.second.filesOpened.empty())
        continue;
      if (strstr(script.first.c_str(), "inplace:"))
        continue;
      bool shouldReload = false;
      for (auto &f : script.second.filesOpened)
      {
        if (f.second == -1) // if it is -1, than it is loaded from vrom, so we can't re-load it
          continue;
        DagorStat buf;
        memset(&buf, 0, sizeof(buf));
        if (df_stat(f.first.c_str(), &buf) == -1) // file removed?
        {
          debug("%s: file %s: removed", script.first.c_str(), f.first.c_str());
          script.second.filesOpened.erase(f.first);
          shouldReload = true;
        }
        else if (buf.mtime > f.second && f.second != -1) // if it is -1, than it is loaded from vrom, so we can't re-load it
        {
          debug("%s: file %s: mtime = %d was %d", script.first.c_str(), f.first.c_str(), buf.mtime, f.second);
          f.second = buf.mtime;
          shouldReload = true;
          if (globalFileAccess)
          {
            auto globalFile = globalFileAccess->filesOpened.find_as(f.first.c_str());
            if (globalFile != globalFileAccess->filesOpened.end())
              globalFile->second = f.second;
          }
        }
        if (shouldReload)
          break;
      }
      if (shouldReload)
        changed.insert_or_assign(script.first, script.second.hasDebugger ? EnableDebugger::YES : EnableDebugger::NO);
    }
    ReloadResult ret = changed.empty() ? NO_CHANGES : RELOADED;
    if (!changed.empty())
    {
      freeSourceData();
      das::Module::ClearSharedModules();
    }
    for (const eastl::pair<eastl::string, EnableDebugger> &c : changed)
    {
      if (!loadScript(c.first.c_str(), das::make_smart<DagFileAccess>(getFileAccess(), HotReload::ENABLED), aot_mode, ResolveECS::YES,
            log_aot_errors, c.second))
        ret = ERRORS;
    }
    compileErrorsCount = -1; // Can't count errors because not all files have been processed.
    return ret;
  }

  void freeSourceData()
  {
    moduleFileAccess.freeSourceData();
    sandboxModuleFileAccess.freeSourceData();
  }

  bool postLoadScript(TLoadedScript &script, const das::string &fname, uint64_t load_start_time);

  bool mainThreadDone()
  {
    G_ASSERT(is_main_thread());
    uint64_t startTime = profile_ref_ticks();
    bool res = true;
    for (auto &script : scripts)
    {
      res = postLoadScript(script.second, script.first, profile_ref_ticks()) && res;
    }
    debug("daScript: post load from main thread in %d ms", profile_time_usec(startTime) / 1000);
    return res;
  }

  void cleanupMemoryUsage()
  {
    freeSourceData();
    das::clearGlobalAotLibrary();
    das::Module::ClearSharedModules();
  }
};

template <typename TLoadedScript, typename TContext>
das::recursive_mutex DasScripts<TLoadedScript, TContext>::mutex;

struct FileSerializationStorage : das::SerializationStorage
{
  file_ptr_t file;
  das::string fileName;

  FileSerializationStorage(file_ptr_t file_, const das::string &name) : file(file_), fileName(name) {}
  virtual size_t writingSize() const override;
  virtual bool readOverflow(void *data, size_t size) override;
  virtual void write(const void *data, size_t size) override;
  virtual ~FileSerializationStorage() override;
};

das::SerializationStorage *create_file_read_serialization_storage(file_ptr_t file, const das::string &name);
das::SerializationStorage *create_file_write_serialization_storage(file_ptr_t file, const das::string &name);

} // namespace bind_dascript
