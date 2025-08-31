// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "das_scripts.h"
#include "das_serialization.h"
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_vromfs.h>
#if _TARGET_PC_WIN
#include <direct.h> // getcwd
#elif _TARGET_PC_LINUX || _TARGET_PC_MACOSX
#include <unistd.h> // getcwd
#endif

namespace bind_dascript
{
extern thread_local ecs::EntityManager *g_das_entity_mgr;

static bool ends_with_suffix(const eastl::string_view &fname, const char **suffixes_begin, const char **suffixes_end)
{
  for (const char **suffix = suffixes_begin; suffix < suffixes_end; ++suffix)
    if (fname.ends_with(*suffix))
      return true;
  return false;
}

bool Scripts::loadScript(const das::string &fname, das::smart_ptr<DagFileAccess> access, LoadScriptCtx ctx)
{
#if DAGOR_DBGLEVEL > 0
  if (profileLoading)
  {
    TIME_PROFILE_NAME(loading_profile, fname.c_str());
    return loadScriptInternal(fname, access, ctx);
  }
  else
#endif
  {
    return loadScriptInternal(fname, access, ctx);
  }
}
bool Scripts::unloadScript(const char *fname, bool strict)
{
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

void Scripts::postProcessModuleGroups()
{
  for (const auto &script : scripts)
  {
    if (script.second.moduleGroup)
      postProcessModuleGroupUserData(script.first, *script.second.moduleGroup);
  }
}

void Scripts::clearStatistics()
{
  linkAotErrorsCount = 0u;
  compileErrorsCount = 0;
}


void Scripts::setSandboxMode(bool is_sandbox)
{
  if (sandboxModuleFileAccess.dasProject.empty())
  {
    logerr("daScript: sandbox mode without restrictions! Not set das_project file. see init_sandbox");
    return;
  }
  sandboxMode = is_sandbox;
  storeTemporaryScripts = is_sandbox;
  unloadTemporaryScript();
}

void Scripts::unloadTemporaryScript()
{
  for (auto &script : temporaryScripts)
    unloadScript(script.c_str(), /*strict*/ false);
  temporaryScripts.clear();
}

ReloadResult Scripts::reloadAllChanged(AotMode aot_mode, LogAotErrors log_aot_errors)
{
#if DAS_HAS_DIRECTORY_WATCH
  DagFileAccess *globalFileAccess = !sandboxMode ? moduleFileAccess.get() : nullptr;
  if (0 == eastl::count_if(dirsOpened.begin(), dirsOpened.end(), [&](auto &dir) { return could_be_changed_folder(dir.second.get()); }))
    return NO_CHANGES;
  struct ChangedScript
  {
    DasSyntax syntax;
    EnableDebugger hasDebugger;
    void *userData;
  };
  ska::flat_hash_set<eastl::string> scriptsToReload;
  ska::flat_hash_map<eastl::string, ChangedScript, ska::power_of_two_std_hash<eastl::string>> changed;

  for (auto &[file, info] : changedFileToScripts)
  {
    DagorStat buf;
    memset(&buf, 0, sizeof(buf));
    if (df_stat(file.c_str(), &buf) == -1) // file removed?
    {
      debug("file %s: removed, reload %d scripts", file.c_str(), info.scriptsToReload.size());
      scriptsToReload.insert(info.scriptsToReload.begin(), info.scriptsToReload.end());
    }
    else if (buf.mtime > info.lastWriteTime && info.lastWriteTime != -1) // if it is -1, than it is loaded from vrom, so we can't
                                                                         // re-load it
    {
      debug("file %s has new time: mtime = %d was %d", file.c_str(), buf.mtime, info.lastWriteTime);
      info.lastWriteTime = buf.mtime;
      scriptsToReload.insert(info.scriptsToReload.begin(), info.scriptsToReload.end());
      if (globalFileAccess)
      {
        auto globalFile = globalFileAccess->filesOpened.find_as(file.c_str());
        if (globalFile != globalFileAccess->filesOpened.end())
          globalFile->second = buf.mtime;
      }
    }
  }
  for (const eastl::string &fname : scriptsToReload)
  {
    auto scriptIt = scripts.find_as(fname.c_str());
    if (scriptIt == scripts.end())
    {
      debug("daScript: script %s not found, skipping", fname.c_str());
      continue;
    }
    if (strstr(fname.c_str(), "inplace:"))
      continue;
    changed.insert_or_assign(fname,
      ChangedScript{scriptIt->second.syntax, scriptIt->second.hasDebugger ? EnableDebugger::YES : EnableDebugger::NO,
        scriptIt->second.ctx ? scriptIt->second.ctx->userData : nullptr});
  }
  ReloadResult ret = changed.empty() ? NO_CHANGES : RELOADED;
  if (!changed.empty())
  {
    freeSourceData();
    das::Module::ClearSharedModules();
  }
  for (const eastl::pair<eastl::string, ChangedScript> &c : changed)
  {
    if (!loadScript(c.first.c_str(), das::make_smart<DagFileAccess>(getFileAccess(), HotReload::ENABLED),
          LoadScriptCtx{aot_mode, ResolveECS::YES, log_aot_errors, c.second.syntax, c.second.hasDebugger, c.second.userData}))
      ret = ERRORS;
  }
  compileErrorsCount = -1; // Can't count errors because not all files have been processed.
  return ret;
#else
  G_UNUSED(aot_mode);
  G_UNUSED(log_aot_errors);
  return NO_CHANGES;
#endif
}

void Scripts::freeSourceData()
{
  moduleFileAccess.freeSourceData();
  sandboxModuleFileAccess.freeSourceData();
}
bool Scripts::mainThreadDone()
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

void Scripts::cleanupMemoryUsage()
{
  freeSourceData();
  das::clearGlobalAotLibrary();
  das::Module::ClearSharedModules();
}
void Scripts::thisThreadDone()
{
  moduleFileAccess.thisThreadDone();
  sandboxModuleFileAccess.thisThreadDone();
  helper.thisThreadDone();
#if DAGOR_DBGLEVEL > 0
  argStrings.thisThreadDone();
#endif
}

void Scripts::processModuleGroupUserData(const das::string &fname, das::ModuleGroup &libGroup)
{
  DebugArgStrings *debugArgStrings = nullptr;
#if DAGOR_DBGLEVEL > 0
  debugArgStrings = argStrings.getOrCreateValuePtr();
#endif
  ESModuleGroupData *mgd = new ESModuleGroupData(debugArgStrings, mgr);
  mgd->hashedScriptName = ECS_HASH_SLOW(fname.c_str()).hash;
  mgd->helper = helper.getOrCreateValuePtr();
  libGroup.setUserData(mgd);
}

int Scripts::getPendingSystemsNum(das::ModuleGroup &group) const
{
  ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
  return (int)mgd->unresolvedEs.size();
}

int Scripts::getPendingQueriesNum(das::ModuleGroup &group) const
{
  ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
  return (int)mgd->unresolvedQueries.size();
}

void Scripts::storeSharedQueries(das::ModuleGroup &group)
{
  ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
  eastl::erase_if(mgd->unresolvedQueries, [&](QueryData &queryData) {
    const bool isShared = queryData.desc->shared;
    if (isShared)
      sharedQueries.emplace_back(eastl::move(queryData));
    return isShared;
  });
}
static bool contains_all_tags(const ecs::TagsSet &src, const ecs::TagsSet &filter_data)
{
  for (unsigned tag : src)
    if (filter_data.find(tag) == filter_data.end())
      return false;
  return true;
}
bool Scripts::postProcessModuleGroupUserData(const das::string &fname, das::ModuleGroup &group)
{
  ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
  ecs::EntityManager *groupMgr = mgd->mgr;
  if (!groupMgr)
    return true;
  const ecs::TagsSet &filterTags = groupMgr->getTemplateDB().info().filterTags;
  G_ASSERTF_RETURN(mgd, false, "ESModuleGroupData is not found for script = %s", fname.c_str());
  if (mgd->templates.empty())
    return true;
  ecs::TemplateRefs trefs(*groupMgr);
  trefs.addComponentNames(mgd->trefs);
  for (const CreatingTemplate &templateData : mgd->templates)
  {
    if (templateData.filterTags.empty() || contains_all_tags(templateData.filterTags, filterTags))
    {
      ecs::ComponentsMap comps;
      ecs::Template::component_set tracked = templateData.tracked;
      ecs::Template::component_set replicated = templateData.replicated;
      ecs::Template::component_set ignored = templateData.ignored;
      for (const auto &comp : templateData.map)
      {
        auto compTags = templateData.compTags.find(comp.first);
        if (compTags == templateData.compTags.end() || contains_all_tags(compTags->second, filterTags))
        {
          comps[comp.first] = comp.second;
        }
        else
        {
          tracked.erase(comp.first);
          replicated.erase(comp.first);
          ignored.erase(comp.first);
        }
      }
      ecs::Template::ParentsList parents = templateData.parents;

      trefs.emplace(ecs::Template(templateData.name.c_str(), eastl::move(comps), eastl::move(tracked), eastl::move(replicated),
                      eastl::move(ignored), false),
        eastl::move(parents));
    }
  }
  if (!groupMgr->updateTemplates(trefs, true, ECS_HASH_SLOW(fname.c_str()).hash,
        [&](const char *n, ecs::EntityManager::UpdateTemplateResult r) {
          switch (r)
          {
            case ecs::EntityManager::UpdateTemplateResult::InvalidName: logerr("%s wrong name\n", n); break;
            case ecs::EntityManager::UpdateTemplateResult::RemoveHasEntities:
              logerr("%s has entities and cant be removed\n", n);
              break;
            case ecs::EntityManager::UpdateTemplateResult::DifferentTag:
              logerr("%s is registered with different tag and can't be updated\n", n);
              break;
            default: break;
          }
        }))
    return false;
  return true;
}

bool Scripts::keepModuleGroupUserData(const das::string &fname, const das::ModuleGroup &group) const
{
  G_UNUSED(fname);
  ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
  G_ASSERTF_RETURN(mgd, false, "ESModuleGroupData is not found for script = %s", fname.c_str());
  return !mgd->templates.empty();
}

bool Scripts::processLoadedScript(LoadedScript &script, const das::string &fname, uint64_t load_start_time, das::ModuleGroup &group,
  AotMode aot_mode, AotModeIsRequired aot_mode_is_required)
{
  if (script.postProcessed)
    return true;
  script.postProcessed = true;
  ESModuleGroupData *mgd = (ESModuleGroupData *)group.getUserData("es");
  G_ASSERTF_RETURN(mgd, false, "ESModuleGroupData is not found for script = %s", fname.c_str());
  G_UNUSED(fname);

  mgd->registerEvents();

  ecs::EntityManager *scriptMgr = script.ctx->mgr;

  storeSharedQueries(group);
  script.queries = eastl::move(mgd->unresolvedQueries);

  if (scriptMgr)
  {
    for (QueryData &queryData : script.queries)
    {
      queryData.desc->base.resolveUnresolved(scriptMgr); // try to resolve asap
      queryData.desc->query = scriptMgr->createQuery(ecs::NamedQueryDesc(queryData.name.c_str(),
        queryData.desc->base.getSlice(BaseEsDesc::RW_END), queryData.desc->base.getSlice(BaseEsDesc::RO_END),
        queryData.desc->base.getSlice(BaseEsDesc::RQ_END), queryData.desc->base.getSlice(BaseEsDesc::NO_END)));
    }

    for (QueryData &queryData : sharedQueries)
    {
      if (ecs::ecs_query_handle_t(queryData.desc->query) == ecs::INVALID_QUERY_HANDLE_VAL)
      {
        queryData.desc->base.resolveUnresolved(scriptMgr); // try to resolve asap
        queryData.desc->query = scriptMgr->createQuery(ecs::NamedQueryDesc(queryData.name.c_str(),
          queryData.desc->base.getSlice(BaseEsDesc::RW_END), queryData.desc->base.getSlice(BaseEsDesc::RO_END),
          queryData.desc->base.getSlice(BaseEsDesc::RQ_END), queryData.desc->base.getSlice(BaseEsDesc::NO_END)));
      }
    }
  }

  const char *debugFn = nullptr;
#if DAGOR_DBGLEVEL > 0
  if (script.fn.empty())
  {
    script.fn = fname.c_str();
    debugFn = script.fn.c_str();
  }
#endif
  mgd->es_resolve_function_ptrs(script.ctx.get(), script.systems, debugFn, load_start_time, aot_mode, aot_mode_is_required,
    statistics);

  const bool hasMtSystems = eastl::find_if(script.systems.begin(), script.systems.end(),
                              [](ecs::EntitySystemDesc *desc) { return desc->getQuant() > 0; }) != script.systems.end();

  if (hasMtSystems)
  {
    for (int idx = 0, n = script.ctx->getTotalVariables(); idx < n; ++idx)
    {
      const das::VarInfo *globalVar = script.ctx->getVariableInfo(idx);
      if (globalVar && !globalVar->isConst())
      {
        logerr("daScript: Global variable '%s' in script <%s>."
               "This script contains multithreaded systems and shouldn't have global variables",
          globalVar->name, fname.c_str());
        return false;
      }
    }
  }

  if (!script.ctx->persistent)
  {
    const uint64_t heapBytes = script.ctx->heap->bytesAllocated();
    const uint64_t stringHeapBytes = script.ctx->stringHeap->bytesAllocated();
    if (heapBytes > 0)
      logerr("daScript: script <%s> allocated %@ bytes for global variables", fname.c_str(), heapBytes);
    if (stringHeapBytes > 0)
    {
      das::string strings = "";
      script.ctx->stringHeap->forEachString([&](const char *str) {
        if (strings.length() < 250)
          strings.append_sprintf("%s\"%s\"", strings.empty() ? "" : ", ", str);
      });
      logerr("daScript: script <%s> allocated %@ bytes for global strings. Allocated strings: %s", fname.c_str(), stringHeapBytes,
        strings.c_str());
    }
    if (heapBytes > 0 || stringHeapBytes > 0)
      script.ctx->useGlobalVariablesMem();
  }
  return true;
}

void Scripts::unloadAllScripts(UnloadDebugAgents unload_debug_agents)
{
#if DAS_HAS_DIRECTORY_WATCH
  dirsOpened.clear();
#endif
  scripts.clear();
  das::Module::Reset(/*reset_debug_agent*/ unload_debug_agents == UnloadDebugAgents::YES);
  clearStatistics();
  moduleFileAccess.clear();
  sandboxModuleFileAccess.clear();
  scriptsMemory.clear();
  sharedQueries.clear();
  helper.clear();
#if DAGOR_DBGLEVEL > 0
  argStrings.clear();
#endif
}
size_t Scripts::dumpMemory()
{
  size_t globals = 0, stack = 0, code = 0, codeTotal = 0, debugInfo = 0, debugInfoTotal = 0, constStrings = 0, constStringsTotal = 0,
         heap = 0, heapTotal = 0, string = 0, stringTotal = 0, shared = 0, unique = 0;
  for (const auto &sc : scripts)
  {
    auto &s = sc.second;
    // globals += s.ctx->globalsSize;//currently is protected!
    globals = 0;
    stack += s.ctx->stack.size();
    code += s.ctx->code->bytesAllocated();
    codeTotal += s.ctx->code->totalAlignedMemoryAllocated();
    constStrings += s.ctx->constStringHeap->bytesAllocated();
    constStringsTotal += s.ctx->constStringHeap->totalAlignedMemoryAllocated();
    debugInfo += s.ctx->debugInfo->bytesAllocated();
    debugInfoTotal += s.ctx->debugInfo->totalAlignedMemoryAllocated();
    heap += s.ctx->heap->bytesAllocated();
    heapTotal += s.ctx->heap->totalAlignedMemoryAllocated();
    string += s.ctx->stringHeap->bytesAllocated();
    stringTotal += s.ctx->stringHeap->totalAlignedMemoryAllocated();
    shared += s.ctx->getSharedMemorySize();
    unique += s.ctx->getUniqueMemorySize();
  }
  size_t total = stack + globals + codeTotal + constStringsTotal + debugInfoTotal + heapTotal + stringTotal;
  size_t waste = constStringsTotal - constStrings + debugInfoTotal - debugInfo + heapTotal - heap + stringTotal - string;
  G_UNUSED(total);
  G_UNUSED(waste);
  debug("daScript: memory usage total = %d, waste=%d\n"
        "globals=%d, stack=%d, code=%d(%d), constStr=%d(%d) debugInfo=%d(%d),heap=%d(%d),string=%d(%d),"
        "shared=%d, unique = %d",
    total, waste, globals, stack, code, codeTotal, constStrings, constStringsTotal, debugInfo, debugInfoTotal, heap, heapTotal, string,
    stringTotal, shared, unique);
  return total;
}
#if DAS_HAS_DIRECTORY_WATCH
static int64_t get_file_mtime(const das::string &file_path, DagFileAccess *global_file_access)
{
  if (global_file_access)
  {
    auto globalFile = global_file_access->filesOpened.find(file_path);
    if (globalFile != global_file_access->filesOpened.end())
    {
      return globalFile->second;
    }
  }
  DagorStat buf;
  df_stat(file_path.c_str(), &buf);
  return buf.mtime;
}
#endif // DAS_HAS_DIRECTORY_WATCH
void Scripts::gatherDependencies(const das::string &fname, das::smart_ptr<DagFileAccess> access, das::ModuleGroup &lib_group)
{
  G_UNUSED(fname);
  G_UNUSED(access);
  G_UNUSED(lib_group);
#if DAS_HAS_DIRECTORY_WATCH
  if (!access->storeOpenedFiles)
    return;
  DagFileAccess *globalFileAccess = !sandboxMode ? moduleFileAccess.get() : nullptr;

  auto filesOpened = eastl::move(access->filesOpened);

  // file itself is not contained in lib_group.getModules(), adds it separately

  filesOpened.emplace(fname, get_file_mtime(fname, globalFileAccess));

  for (das::Module *module : lib_group.getModules())
  {
    const eastl::string &fileName = module->fileName;
    if (fileName.empty())
      continue;
    if (filesOpened.find(fileName) == filesOpened.end())
    {
      filesOpened.emplace(fileName, get_file_mtime(fileName, globalFileAccess));
    }
  }

  for (const auto &[file, time] : filesOpened)
  {
    auto &info = changedFileToScripts[file];
    info.lastWriteTime = time;
    info.scriptsToReload.insert(fname);
  }

  for (auto &f : filesOpened)
  {
    const char *realName = f.first.c_str();
    if (!realName)
      continue;
    const char *fnameExt = dd_get_fname(realName);
    char buf[512];
    if (!fnameExt || fnameExt <= realName || fnameExt - realName >= sizeof(buf))
      continue;
    memcpy(buf, realName, fnameExt - realName - 1);
    buf[fnameExt - realName - 1] = 0;
    dd_simplify_fname_c(buf);
    das::lock_guard<das::recursive_mutex> guard(mutex); // dirsOpened
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
    eastl::string str;
    const char *resolvedName;
    if (dd_resolve_named_mount(str, buf))
      resolvedName = str.c_str();
    else
      resolvedName = buf;
    WatchedFolderMonitorData *handle = add_folder_monitor(resolvedName, 100);
    if (handle)
    {
      dirsOpened.emplace(buf, eastl::unique_ptr<WatchedFolderMonitorData, DirWatchHandleDeleter>(handle));
      debug("watch dir = %s", buf);
    }
  }
#endif
}

void populateDummyLibGroup(das::ProgramPtr program, das::ModuleGroup *dummyLibGroup)
{
  program->thisModuleGroup = dummyLibGroup;
  program->library.foreach(
    [&](das::Module *m) {
      dummyLibGroup->addModule(m);
      return true;
    },
    "*");
  dummyLibGroup->getModules().pop_back();
}

bool Scripts::loadScriptInternal(const das::string &fname, das::smart_ptr<DagFileAccess> access, LoadScriptCtx ldr_ctx)
{
  static const char *alwaysSkipSuffixes[] = {"_common.das", "_macro.das"};
  static const char *debugFiles[] = {"_console.das", "_debug.das"};
  static const char *eventFiles[] = {"_events.das"};
  if (!is_in_documentation() && ends_with_suffix(fname, eastl::begin(alwaysSkipSuffixes), eastl::end(alwaysSkipSuffixes)))
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

  if (!mgr)
    mgr = g_das_entity_mgr;
  if (!mgr) // lazy initialization
    mgr = g_entity_mgr.get();

  const uint64_t loadStartTime = profile_ref_ticks();
  DebugPrinter tout;
  auto dummyLibGroup = eastl::make_unique<das::ModuleGroup>();
  processModuleGroupUserData(fname, *dummyLibGroup);
  das::CodeOfPolicies policies;
  policies.aot = ldr_ctx.aotMode == AotMode::AOT;
  const bool noLinter = (bool)::dgs_get_argv("das-no-linter");
  policies.no_unused_function_arguments = !noLinter;
  policies.no_unused_block_arguments = !noLinter;
  policies.fail_on_lack_of_aot_export = true;
  policies.no_global_variables = true;
  policies.fail_on_no_aot = false;
  policies.debugger = ldr_ctx.enableDebugger == EnableDebugger::YES;
  policies.no_unsafe = sandboxMode;
  policies.no_aliasing = true;
  policies.strict_unsafe_delete = true;
  policies.gen2_make_syntax = ldr_ctx.syntax == DasSyntax::V1_5;
  policies.version_2_syntax = ldr_ctx.syntax == DasSyntax::V2_0;
  policies.stack = 4096;
  policies.export_all = is_in_documentation();
  debug("daScript: load script <%s> syntax %@", fname.c_str(), (int)ldr_ctx.syntax);

  das::ProgramPtr program;

  if (enableSerialization && serializationReading)
  {
    program = das::make_smart<das::Program>();
    if (initDeserializer)
      initDeserializer->thisModuleGroup = dummyLibGroup.get();
    if (initDeserializer && initDeserializer->serializeScript(program))
    {
      populateDummyLibGroup(program, dummyLibGroup.get());
    }
    else
    {
      dummyLibGroup->reset();                                                        // clean partially serialized modules
      program = das::compileDaScript(fname, access, tout, *dummyLibGroup, policies); // try to recompile to get better error messages
      bind_dascript::enableSerialization = false;
    }
  }

  if (!program)
  {
    program = das::compileDaScript(fname, access, tout, *dummyLibGroup, policies);
  }

  if (program)
  {
    if (DAGOR_UNLIKELY(program->failed()))
    {
      char cwd[DAGOR_MAX_PATH];
#if _TARGET_PC
      if (!getcwd(cwd, sizeof(cwd)))
#endif
        cwd[0] = '\0';
      logerr("failed to compile <%s> cwd=%s\n", df_get_abs_fname(fname.c_str()), cwd);
      for (auto &err : program->errors)
      {
        logerr("%s", das::reportError(err.at, err.what, err.extra, err.fixme, err.cerr).c_str());
        compileErrorsCount++;
      }
      das::lock_guard<das::recursive_mutex> guard(mutex); // scripts
      auto it = scripts.find(fname);
      if (it == scripts.end())
      {
        LoadedScript script;
        script.syntax = ldr_ctx.syntax;
        if (ldr_ctx.userData)
        {
          script.ctx = das::make_shared<EsContext>(mgr, program->unsafe ? program->getContextStackSize() : 0);
          script.ctx->userData = ldr_ctx.userData;
        }
        gatherDependencies(fname, access, *dummyLibGroup);
        scripts[fname] = eastl::move(script);
      }
      else
      {
        gatherDependencies(fname, access, *dummyLibGroup);
      }
      return false;
    }
    if (ldr_ctx.resolveEcs == ResolveECS::YES)
    {
      if (!postProcessModuleGroupUserData(fname, *dummyLibGroup))
      {
        logerr("daScript: failed to post process<%s>, internal error, use %d\n", fname.c_str(), program->use_count());
        return false;
      }
    }

    // Do not write out anything when loading is in progress (in serveral threads)
    if (enableSerialization && !serializationReading && !suppressSerialization && initSerializer)
      program->serialize(*initSerializer);

    das::shared_ptr<EsContext> ctx = das::make_shared<EsContext>(mgr, program->unsafe ? program->getContextStackSize() : 0);
    ctx->userData = ldr_ctx.userData;
#if DAS_HAS_DIRECTORY_WATCH
    ctx->name = fname;
#endif
    bool simulateRes = false;
    if (!program->unsafe)
    {
      // das::SharedFramememStackGuard guard(*ctx);
      // simulateRes = program->simulate(*ctx, tout, &guard);
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
    if (ldr_ctx.aotMode == AotMode::AOT && !program->aotErrors.empty())
    {
      linkAotErrorsCount += uint32_t(program->aotErrors.size());
      logwarn("daScript: failed to link cpp aot <%s>\n", fname.c_str());
      if (ldr_ctx.logAotErrors == LogAotErrors::YES)
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
      if (!ctx->persistent && program->options.getBoolOption("gc", false))
        logerr("`%s`: options gc - is allowed only for debug/console scripts.", fname.c_str());
      if (program->options.getBoolOption("rtti", false))
        logerr("`%s`: options rtti - is allowed only for debug/console/macro scripts.", fname.c_str());
    }

    AotModeIsRequired aotModeIsRequired = !debugScript ? AotModeIsRequired::YES : AotModeIsRequired::NO;
    LoadedScript script(eastl::move(program), eastl::move(ctx), access, aotModeOverride, aotModeIsRequired, ldr_ctx.syntax,
      ldr_ctx.enableDebugger);
    gatherDependencies(fname, access, *dummyLibGroup);


    if (script.program->thisModule->isModule)
    {
      if (!is_in_documentation() && !eastl::string_view(fname).ends_with("events.das")) // we still load/unload events files to perform
                                                                                        // annotations macro
        logerr("daScript: '%s' is module. If it's file with events, rename it to <fileName>_events.das. Otherwise rename it to "
               "<fileName>_common.das or <fileName>_macro.das (macro in case it contains macroses)",
          fname.c_str());
      const int num = getPendingSystemsNum(*script.program->thisModuleGroup);
      if (num == 0)
      {
        if (!das::is_in_aot() && !is_in_documentation())
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


    das::lock_guard<das::recursive_mutex> guard(mutex); // scripts

    if (!script.ctx->persistent)
    {
      auto memIt = scriptsMemory.find(fname);
      if (memIt != scriptsMemory.end())
        scriptsMemory.erase(memIt);
    }
    else
    {
      // persistent
      scriptsMemory[fname] =
        ScriptMemory{(uint64_t)script.ctx->heap->getInitialSize(), (uint64_t)script.ctx->stringHeap->getInitialSize(), 0};
    }

    if (storeTemporaryScripts)
      temporaryScripts.push_back(fname);

    auto it = scripts.find(fname);
    if (it != scripts.end())
    {
      debug("daScript: unload %s", fname.c_str());
      it->second.unload();
    }

    if (!enableSerialization && !is_in_documentation())
      dummyLibGroup->reset(); // cleanup submodules to reduce memory usage

    script.moduleGroup = eastl::move(dummyLibGroup);

    if (ldr_ctx.resolveEcs == ResolveECS::YES)
      postLoadScript(script, fname, profile_ref_ticks());

    // Note: In case of serialization we cannot reset now, as the data will be needed later
    // The clean up is moved to "after the load" stage if we are reading, or after the write if we are writing.
    if (!enableSerialization && !script.program->options.getBoolOption("rtti", false) && !das::is_in_aot() && !is_in_documentation())
      script.program.reset(); // we don't need program to be loaded. Saves memory

    scripts[fname] = eastl::move(script);
  }
  else
  {
    logerr("internal compile error <%s>\n", fname.c_str());
    return false;
  }
  debug("daScript: file %s loaded in %d ms", fname.c_str(), profile_time_usec(loadStartTime) / 1000);
  return true;
}


bool Scripts::postLoadScript(LoadedScript &script, const das::string &fname, uint64_t load_start_time)
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

  if (script.moduleGroup && !keepModuleGroupUserData(fname, *script.moduleGroup) && !enableSerialization && !is_in_documentation())
    script.moduleGroup->reset();

  return res;
}
} // namespace bind_dascript