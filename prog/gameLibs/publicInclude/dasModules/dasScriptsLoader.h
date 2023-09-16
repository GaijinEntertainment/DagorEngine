//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daScript/daScript.h>
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
    const int newPos = tellp();
    if (newPos != pos)
    {
      int len = newPos - pos;
      debug("%.*s", len, data.data() + pos);
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
    uint64_t heapSize;
    uint64_t stringHeapSize;
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

  bool warnOnPersistentHeap = true;

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
    das::ModuleGroup &lib_group)
  {
    if (!access->storeOpenedFiles)
      return;
    DagFileAccess *globalFileAccess = !sandboxMode ? moduleFileAccess.get() : nullptr;
    das::vector<das::ModuleInfo> req;
    das::vector<das::string> missing, circular;
    das::vector<das::string> notAllowed;
    das::das_set<das::string> dependencies;
    if (das::getPrerequisits(fname, access, req, missing, circular, notAllowed, dependencies, lib_group, nullptr, 1, false))
    {
      for (das::ModuleInfo &moduleInfo : req)
      {
        if (eastl::find_if(script.filesOpened.begin(), script.filesOpened.end(),
              [&](auto it) { return it.first == moduleInfo.fileName; }) == script.filesOpened.end())
        {
          if (globalFileAccess)
          {
            auto globalFile = globalFileAccess->filesOpened.find_as(moduleInfo.fileName);
            if (globalFile != globalFileAccess->filesOpened.end())
            {
              script.filesOpened.emplace(moduleInfo.fileName, globalFile->second);
              continue;
            }
          }
          DagorStat buf;
          df_stat(moduleInfo.fileName.c_str(), &buf);
          script.filesOpened.emplace(moduleInfo.fileName, buf.mtime);
        }
      }
    }
  }

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
    ResolveECS resolve_ecs, LogAotErrors log_aot_errors, EnableDebugger enable_debugger)
  {
    static const char *alwaysSkipSuffixes[] = {"_common.das", "_macro.das"};
    static const char *debugFiles[] = {"_console.das", "_debug.das"};
    static const char *eventFiles[] = {"_events.das"};
    if (ends_with_suffix(fname, eastl::begin(alwaysSkipSuffixes), eastl::end(alwaysSkipSuffixes)))
    {
      debug("daScript: skip module/macro file '%s'", fname.c_str());
      return true;
    }
    const bool debugScript = ends_with_suffix(fname, eastl::begin(debugFiles), eastl::end(debugFiles));
    if (!(das::is_in_aot() || das::is_in_completion()))
    {
      if (!loadDebugCode && debugScript)
      {
        debug("daScript: skip debug file '%s'", fname.c_str());
        return true;
      }
      if (!loadEvents && ends_with_suffix(fname, eastl::begin(eventFiles), eastl::end(eventFiles)))
      {
        debug("daScript: skip event file '%s'", fname.c_str());
        return true;
      }
    }

    const uint64_t loadStartTime = profile_ref_ticks();
    DebugPrinter tout;
    auto dummyLibGroup = eastl::make_unique<das::ModuleGroup>();
    processModuleGroupUserData(fname, *dummyLibGroup);
    debug("daScript: load script <%s>", fname.c_str());
    das::CodeOfPolicies policies;
    policies.aot = aot_mode == AotMode::AOT;
    const bool noLinter = (bool)::dgs_get_argv("das-no-linter");
    policies.no_unused_function_arguments = !noLinter;
    policies.no_unused_block_arguments = !noLinter;
    policies.fail_on_lack_of_aot_export = true;
    policies.no_global_variables = true;
    policies.fail_on_no_aot = false;
    policies.debugger = enable_debugger == EnableDebugger::YES;
    policies.no_unsafe = sandboxMode;
    policies.no_aliasing = true;
    policies.strict_unsafe_delete = true;
    policies.strict_smart_pointers = true;
    policies.stack = 4096;
    if (das::ProgramPtr program = das::compileDaScript(fname, access, tout, *dummyLibGroup, policies))
    {
      if (program->failed())
      {
        logerr("failed to compile <%s>\n", fname.c_str());
        for (auto &err : program->errors)
        {
          G_UNUSED(err);
          logerr(das::reportError(err.at, err.what, err.extra, err.fixme, err.cerr).c_str());
          compileErrorsCount++;
        }
        das::lock_guard<das::recursive_mutex> guard(mutex);
        auto it = scripts.find(fname);
        if (it == scripts.end())
        {
          TLoadedScript script;
          script.filesOpened = eastl::move(access->filesOpened);
          fillSharedModules(script, fname, access, *dummyLibGroup);
          scripts[fname] = eastl::move(script);
        }
        else
        {
          it->second.filesOpened = eastl::move(access->filesOpened);
          fillSharedModules(it->second, fname, access, *dummyLibGroup);
        }
        storeSharedQueries(*dummyLibGroup);
        return false;
      }
      {
        das::lock_guard<das::recursive_mutex> guard(mutex);
        if (!postProcessModuleGroupUserData(fname, *dummyLibGroup))
        {
          logerr("daScript: failed to post process<%s>, internal error, use %d\n", fname.c_str(), program->use_count());
          return false;
        }
      }

      das::shared_ptr<TContext> ctx = das::make_shared<TContext>(program->unsafe ? program->getContextStackSize() : 0);
#if DAS_HAS_DIRECTORY_WATCH
      ctx->name = fname;
#endif
      bool simulateRes = false;
      if (!program->unsafe)
      {
        // das::SharedStackGuard guard(*ctx, get_shared_stack());
        // simulateRes = program->simulate(*ctx, tout, &get_shared_stack());
        simulateRes = program->simulate(*ctx, tout);
      }
      else if (sandboxMode)
      {
        logerr("daScript: unsafe context <%s>", fname.c_str());
      }
      else
      {
        logwarn("daScript: unsafe context <%s>", fname.c_str());
        simulateRes = program->simulate(*ctx, tout);
      }
      if (!simulateRes)
      {
        if (ctx->getException())
          logerr("daScript: failed to compile, exception <%s>\n", ctx->getException());
        else
        {
          logerr("daScript: failed to compile<%s>, internal error\n", fname.c_str());
          for (auto &err : program->errors)
          {
            G_UNUSED(err);
            logerr(das::reportError(err.at, err.what, err.extra, err.fixme, err.cerr).c_str());
          }
        }
        return false;
      }
      // this should only be here when AOT is enabled
      if (aot_mode == AotMode::AOT && !program->aotErrors.empty())
      {
        linkAotErrorsCount += uint32_t(program->aotErrors.size());
        logwarn("daScript: failed to link cpp aot <%s>\n", fname.c_str());
        if (log_aot_errors == LogAotErrors::YES)
          for (auto &err : program->aotErrors)
          {
            logwarn(das::reportError(err.at, err.what, err.extra, err.fixme, err.cerr).c_str());
          }
      }
      const AotMode aotModeOverride = program->options.getBoolOption("no_aot", false) ? AotMode::NO_AOT : AotMode::AOT;
      if (debugScript)
      {
        if (aotModeOverride != AotMode::NO_AOT)
          logerr("%s debug file was loaded with enabled aot. Please disable aot using `options no_aot`"
                 " or remove this error if you really need this",
            fname.c_str());
      }
      else
      {
        if (program->options.getBoolOption("gc", false))
          logerr("`%s`: options gc - is allowed only for debug/console scripts.", fname.c_str());
        if (warnOnPersistentHeap && ctx->persistent)
          logerr("`%s`: options persistent_heap - is allowed only for debug/console scripts.", fname.c_str());
        if (program->options.getBoolOption("rtti", false))
          logerr("`%s`: options rtti - is allowed only for debug/console/macro scripts.", fname.c_str());
      }

      AotModeIsRequired aotModeIsRequired = !debugScript ? AotModeIsRequired::YES : AotModeIsRequired::NO;
      TLoadedScript script(eastl::move(program), eastl::move(ctx), access, aotModeOverride, aotModeIsRequired, enable_debugger);
      script.filesOpened = eastl::move(access->filesOpened);
      fillSharedModules(script, fname, access, *dummyLibGroup);

#if DAS_HAS_DIRECTORY_WATCH
      for (auto &f : script.filesOpened)
      {
        const char *realName = df_get_real_name(f.first.c_str());
        if (!realName)
          continue;
        const char *fnameExt = dd_get_fname(realName);
        char buf[512];
        if (!fnameExt || fnameExt <= realName || fnameExt - realName >= sizeof(buf))
          continue;
        memcpy(buf, realName, fnameExt - realName - 1);
        buf[fnameExt - realName - 1] = 0;
        dd_simplify_fname_c(buf);
        das::lock_guard<das::recursive_mutex> guard(mutex);
        if (dirsOpened.find_as((const char *)buf) != dirsOpened.end())
          continue;
        bool hasUpperPath = false;
        const int bufLen = strlen(buf);
        for (auto &d : dirsOpened)
        {
          if (d.first.length() >= bufLen)
            continue;
          if (strncmp(d.first.c_str(), buf, d.first.length()) == 0)
          {
            hasUpperPath = true;
            break;
          }
        }
        if (hasUpperPath)
          continue;
        WatchedFolderMonitorData *handle = add_folder_monitor(buf, 100);
        if (handle)
        {
          dirsOpened.emplace(buf, eastl::unique_ptr<WatchedFolderMonitorData, DirWatchHandleDeleter>(handle));
          debug("watch dir = %s", buf);
        }
      }
#endif

      if (script.program->thisModule->isModule)
      {
        if (!ends_with(fname, "events.das")) // we still load/unload events files to perform annotations macro
          logerr("daScript: '%s' is module. If it's file with events, rename it to <fileName>_events.das. Otherwise rename it to "
                 "<fileName>_common.das or <fileName>_macro.das (macro in case it contains macroses)",
            fname.c_str());
        const int num = getPendingSystemsNum(*script.program->thisModuleGroup);
        if (num == 0)
        {
          if (!das::is_in_aot())
          {
            G_ASSERT(script.systems.empty() && script.queries.empty());
            debug("das: unload module `%s`", fname.c_str());
            script.ctx.reset();
            script.program.reset();
            return true;
          }
        }
        else
        {
          logwarn("das: unable to unload potentially unused module %s", fname.c_str());
        }
      }

      if (!script.ctx->persistent)
      {
        auto memIt = scriptsMemory.find_as(fname);
        if (memIt != scriptsMemory.end())
          scriptsMemory.erase(memIt);
        script.program->library.foreach(
          [&fname](das::Module *mod) {
            mod->globals.foreach([&fname](auto globVar) {
              if (globVar && globVar->used && !globVar->type->isNoHeapType())
              {
                const auto ignoreMark = globVar->annotation.find("ignore_heap_usage", das::Type::tBool);
                if (ignoreMark == nullptr || !ignoreMark->bValue)
                  logerr("global variable '%s' in script <%s> requires heap memory, but heap memory is regularly cleared."
                         "Only value types/fixed arrays are supported in this case",
                    globVar->name, fname.c_str());
              }
            });
            return true;
          },
          "*");
      }
      else
      {
        // persistent
        scriptsMemory[fname] =
          ScriptMemory{(uint64_t)script.ctx->heap->getInitialSize(), (uint64_t)script.ctx->stringHeap->getInitialSize(), 0};
      }

      das::lock_guard<das::recursive_mutex> guard(mutex);
      if (storeTemporaryScripts)
        temporaryScripts.push_back(fname);

      auto it = scripts.find(fname);
      if (it != scripts.end())
      {
        debug("daScript: unload %s", fname.c_str());
        it->second.unload();
      }

      dummyLibGroup->reset(); // cleanup submodules to reduce memory usage
      script.moduleGroup = eastl::move(dummyLibGroup);

      if (resolve_ecs == ResolveECS::YES)
        postLoadScript(script, fname, profile_ref_ticks());

      if (!script.program->options.getBoolOption("rtti", false) && !das::is_in_aot())
        script.program.reset(); // we don't need program to be loaded. Saves memory

      scripts[fname] = eastl::move(script);

      debug("daScript: file %s loaded in %d ms", fname.c_str(), profile_time_usec(loadStartTime) / 1000);
    }
    else
    {
      logerr("internal compile error <%s>\n", fname.c_str());
      return false;
    }
    return true;
  }

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

  bool postLoadScript(TLoadedScript &script, const das::string &fname, uint64_t load_start_time)
  {
    if (!script.moduleGroup)
      return true;

    bool res = true;
    if (!processLoadedScript(script, fname, load_start_time, *script.moduleGroup, script.hasAot ? AotMode::AOT : AotMode::NO_AOT,
          script.requireAot ? AotModeIsRequired::YES : AotModeIsRequired::NO))
    {
      logerr("daScript: failed to process<%s>, internal error\n", fname.c_str());
      res = false;
    }

    if (!keepModuleGroupUserData(fname, *script.moduleGroup))
      script.moduleGroup.reset();

    return res;
  }

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

} // namespace bind_dascript
