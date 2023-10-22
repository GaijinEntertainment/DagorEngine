#include <dasModules/dasScriptsLoader.h>
#include <util/dag_threadPool.h>
#include "../../gameLibs/ecs/scripts/das/das_ecs.h"

namespace bind_dascript
{
constexpr uint32_t stackSize = 16 * 1024;

das::StackAllocator &get_shared_stack()
{
  static thread_local das::StackAllocator sharedStack(stackSize);
  return sharedStack;
}

bool enableCompression = false;
bool debugSerialization = false;
bool enableSerialization = false;
bool serializationReading = false;
bool suppressSerialization = false;
das::AstSerializer initSerializer;
das::AstSerializer initDeserializer;

template <typename TLoadedScript, typename TContext>
void DasScripts<TLoadedScript, TContext>::fillSharedModules(TLoadedScript &script, const das::string &fname,
  das::smart_ptr<DagFileAccess> access, das::ModuleGroup &lib_group)
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

template <typename TLoadedScript, typename TContext>
bool DasScripts<TLoadedScript, TContext>::loadScriptInternal(const das::string &fname, das::smart_ptr<DagFileAccess> access,
  AotMode aot_mode, ResolveECS resolve_ecs, LogAotErrors log_aot_errors, EnableDebugger enable_debugger)
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

  das::ProgramPtr program;

  if (enableSerialization && serializationReading)
  {
    program = das::make_smart<das::Program>();
    initDeserializer.thisModuleGroup = dummyLibGroup.get();
    program->serialize(initDeserializer);
    program->thisModuleGroup = dummyLibGroup.get();
    program->library.foreach(
      [&](das::Module *m) {
        dummyLibGroup->addModule(m);
        return true;
      },
      "*");
  }
  else
  {
    program = das::compileDaScript(fname, access, tout, *dummyLibGroup, policies);
  }
  if (program)
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

    // Do not write out anything when loading is in progress (in serveral threads)
    if (enableSerialization && !serializationReading && !suppressSerialization)
      program->serialize(initSerializer);

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

    if (!enableSerialization)
      dummyLibGroup->reset(); // cleanup submodules to reduce memory usage

    script.moduleGroup = eastl::move(dummyLibGroup);

    if (resolve_ecs == ResolveECS::YES)
      postLoadScript(script, fname, profile_ref_ticks());

    // Note: In case of serialization we cannot reset now, as the data will be needed later
    // The clean up is moved to "after the load" stage if we are reading, or after the write if we are writing.
    if (!enableSerialization && !script.program->options.getBoolOption("rtti", false) && !das::is_in_aot())
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

template bool DasScripts<LoadedScript, EsContext>::loadScriptInternal(const das::string &fname, das::smart_ptr<DagFileAccess> access,
  AotMode aot_mode, ResolveECS resolve_ecs, LogAotErrors log_aot_errors, EnableDebugger enable_debugger);


template <typename TLoadedScript, typename TContext>
bool DasScripts<TLoadedScript, TContext>::postLoadScript(TLoadedScript &script, const das::string &fname, uint64_t load_start_time)
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

  if (!keepModuleGroupUserData(fname, *script.moduleGroup) && !enableSerialization)
    script.moduleGroup.reset();

  return res;
}

template bool DasScripts<LoadedScript, EsContext>::postLoadScript(LoadedScript &script, const das::string &fname,
  uint64_t load_start_time);

} // namespace bind_dascript
