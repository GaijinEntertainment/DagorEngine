// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasScripts.h"
#include <daRg/dasBinding.h>
#include <daRg/dag_dasScriptsLoading.h>
#include <memory/dag_dbgMem.h>
#include "guiScene.h"
#include <dasModules/dasFsFileAccess.h>

#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_threads.h> // WORKER_THREADS_AFFINITY_MASK
#include <EASTL/algorithm.h>

using namespace das;


namespace darg
{

// Process-global, recursive. The das environment / shared module registry is process-global,
// so a per-DasScriptsData lock would not serialize two managers' workers against it.
// It serializes: worker compile+simulate; releaseJob() integration (deferred init + publish);
// reload/shutdown teardown.
// Steady-state rendering is NOT serialized by it (render only touches an already-published,
// immutable Context).
static WinCritSec &das_compile_lock()
{
  static WinCritSec lock("darg_das_compile");
  return lock;
}


// Set while a worker compile/simulate mutates shared das state. Read only on das exception
// paths to flag eval/compile overlap (the "eval is env-free" assumption is unenforced; this
// surfaces a violation in crash logs). Always-on: cost is negligible, value is in release.
static volatile int das_compile_active = 0;

bool darg_das_compile_in_flight() { return interlocked_acquire_load(das_compile_active) != 0; }

struct DasCompileActiveScope
{
  DasCompileActiveScope() { interlocked_increment(das_compile_active); }
  ~DasCompileActiveScope() { interlocked_decrement(das_compile_active); }
};


class NoDebugStackCollector
{
public:
  bool wasStackFill;
  NoDebugStackCollector() { wasStackFill = DagDbgMem::enable_stack_fill(false); }
  ~NoDebugStackCollector() { DagDbgMem::enable_stack_fill(wasStackFill); }
};

struct DargContext final : das::Context
{
  DargContext(uint32_t stackSize) : das::Context(stackSize) {}

  DargContext(Context &ctx, uint32_t category) = delete;

  void to_out(const das::LineInfo *, int level, const char *message) override
  {
    const int logLevel = level >= das::LogLevel::error     ? LOGLEVEL_ERR
                         : level >= das::LogLevel::warning ? LOGLEVEL_WARN
                                                           : LOGLEVEL_DEBUG;
    ::logmessage(logLevel, "daRg-das: %s", message);
  }

  virtual uint32_t unlock() override
  {
    const uint32_t res = das::Context::unlock();
    if (insideContext == 0 && !persistent)
    {
#if DAGOR_DBGLEVEL > 0
      if (reportHeap && heap->totalAlignedMemoryAllocated() > heapLimit)
      {

        reportHeap = false;
        ::logerr("%@: heap memory exceeded limit %@ of %@ bytes. Try to reduce memory usage.\n"
                 "Delete allocated array or tables or use `var inscope` to limit their lifetime.\n"
                 "Alternative: consider switching to a persistent_heap with gc.",
          name.c_str(), heap->totalAlignedMemoryAllocated(), heapLimit);
      }
      if (reportStringHeap && stringHeap->totalAlignedMemoryAllocated() > stringHeapLimit)
      {
        reportStringHeap = false;
        ::logerr("%@: string heap memory exceeded limit %@ of %@ bytes. Try to reduce memory usage.\n"
                 "Delete allocated strings or use `var inscope` to limit their lifetime.\n"
                 "Alternative: consider switching to a persistent_heap with gc.",
          name.c_str(), stringHeap->totalAlignedMemoryAllocated(), stringHeapLimit);
      }
#endif
      restartHeaps();
    }
    return res;
  }

#if DAGOR_DBGLEVEL > 0
  uint64_t heapLimit = 512 * 1024;
  uint64_t stringHeapLimit = 512 * 1024;
  bool reportHeap = true;
  bool reportStringHeap = true;
#endif
};


void DasLogWriter::output()
{
  const uint64_t newPos = tellp();
  if (newPos != pos)
  {
    const auto len = newPos - pos;
    ::debug("daRg-das: %.*s", len, data() + pos);
    pos = newPos;
  }
}


class DargFileAccess final : public das::ModuleFileAccess
{
public:
  DargFileAccess() {}

  DargFileAccess(const char *pak) : das::ModuleFileAccess(pak, das::make_smart<DargFileAccess>()) {}

  virtual das::FileInfo *getNewFileInfo(const das::string &fname) override;
  virtual das::ModuleInfo getModuleInfo(const das::string &req, const das::string &from) const override;
};

static bool hasDaslibMountPoint()
{
  static const bool hasDaslibMountPoint_ = (bool)dd_get_named_mount_path("daslib");
  return hasDaslibMountPoint_;
}

das::FileInfo *DargFileAccess::getNewFileInfo(const das::string &file_name)
{
  das::string fname = file_name;
  if (hasDaslibMountPoint() && das::string_view(fname).starts_with("./daslib/"))
  {
    fname = das::string(das::string::CtorSprintf(), "%%%s", fname.c_str() + 2);
  }
  if (!dd_file_exist(fname.c_str()))
  {
    logerr("sq: Script file %s not found", fname.c_str());
    return nullptr;
  }
  return setFileInfo(fname, das::make_unique<bind_dascript::FsFileInfo>());
}


das::ModuleInfo DargFileAccess::getModuleInfo(const das::string &req, const das::string &from) const
{
  if (!failed())
    return das::ModuleFileAccess::getModuleInfo(req, from);

  bool reqUsesMountPoint = strncmp(req.c_str(), "%", 1) == 0;
  return das::ModuleFileAccess::getModuleInfo(req, reqUsesMountPoint ? das::string() : from);
}


/****************************************************************************/
/****************************************************************************/


static bool is_das_inited()
{
  if (!*daScriptEnvironment::bound)
    return false;
  bool isDargBound = false;
  das::Module::foreach([&](Module *module) -> bool {
    if (module->name == "darg")
    {
      isDargBound = true;
      return false;
    }
    return true;
  });
  return isDargBound;
}


static void process_loaded_script(const das::Context &ctx, const char *filename)
{
  const uint64_t heapBytes = ctx.heap->bytesAllocated();
  const uint64_t stringHeapBytes = ctx.stringHeap->bytesAllocated();
  if (heapBytes > 0)
    logerr("daScript: script <%s> allocated %@ bytes for global variables", filename, heapBytes);
  if (stringHeapBytes > 0)
  {
    das::string strings = "";
    ctx.stringHeap->forEachString([&](const char *str) {
      if (strings.length() < 250)
        strings.append_sprintf("%s\"%s\"", strings.empty() ? "" : ", ", str);
    });
    logerr("daRg-das: script <%s> allocated %@ bytes for global strings. Allocated strings: %s", filename, stringHeapBytes,
      strings.c_str());
  }
}


/****************************************************************************/
/* Host compile-scope hooks (host-installed; null in dargbox / non-host)    */
/****************************************************************************/

// Host-installed via set_das_async_compile_scope_hooks(). Lets a host wrap the
// worker compile with whatever thread-local state its compile-time macros need
static void *(*das_async_compile_scope_open)() = nullptr;
static void (*das_async_compile_scope_close)(void *) = nullptr;

void set_das_async_compile_scope_hooks(void *(*open)(), void (*close)(void *))
{
  das_async_compile_scope_open = open;
  das_async_compile_scope_close = close;
}


/****************************************************************************/
/* Init-purity lint                                                         */
/****************************************************************************/

// Worker-side init (compile+simulate+init run together there) must not reach
// host-affine code - the Quirrel VM and darg-bound C++ API are main-thread
// bound. Deliberately laxer than policies.no_init: pure non-const init
// (E3DCOLOR/fixed_array/ref_time_ticks/const math) still passes; only init
// reaching the darg native module is rejected. Walks Function::useFunctions.
static const char *const DARG_HOST_MODULE_NAME = "darg";

static bool init_reaches_host_module(Function *fn, das_hash_set<Function *> &visited)
{
  if (!fn || visited.find(fn) != visited.end()) // same visited-set idiom as isRecursive()
    return false;
  visited.insert(fn);
  if (fn->module && fn->module->name == DARG_HOST_MODULE_NAME)
    return true;
  for (Function *callee : fn->useFunctions)
    if (init_reaches_host_module(callee, visited))
      return true;
  return false;
}

// Returns true and fills `err` if not init-pure; false if clean. simulate() runs
// [init]/global init for EVERY module, so walk the whole library, not just the
// root file. builtIn (native/foreign) modules are skipped - their init is
// C++-side; the concern is user-authored .das init.
static bool darg_das_init_purity_violation(Program *program, eastl::string &err)
{
  if (!program)
    return false;

  bool violated = false;
  program->library.foreach(
    [&](das::Module *mod) -> bool {
      if (!mod || mod->builtIn)
        return true; // skip native/foreign modules

      // (1) No user-authored [init] / [finalize]. Same dead/template filter as
      // daScript's own simulate-time check (ast_simulate.cpp).
      mod->functions.foreach([&](auto &fn) {
        if (violated || !fn || fn->index < 0 || !fn->used || fn->isTemplate)
          return;
        if (fn->init || fn->shutdown)
        {
          err.sprintf("init-purity: '%s' (module '%s') is an [%s] function - async daRg das loading "
                      "runs init on a cpujobs worker, so [init]/[finalize] are not allowed",
            fn->name.c_str(), mod->name.c_str(), fn->init ? "init" : "finalize");
          violated = true;
        }
      });
      if (violated)
        return false;

      // (2) No global whose initializer's call graph reaches the darg native
      // module (the host-affine darg/Quirrel API).
      mod->globals.foreach([&](auto &gv) {
        if (violated || !gv || !gv->used || !gv->init)
          return;
        das_hash_set<Function *> visited;
        for (Function *callee : gv->useFunctions)
          if (init_reaches_host_module(callee, visited))
          {
            err.sprintf("init-purity: global '%s' (module '%s') initializer reaches the '%s' native "
                        "module - async daRg das loading runs global init on a cpujobs worker, but "
                        "the darg/Quirrel API is main-thread bound and must not be called from init",
              gv->name.c_str(), mod->name.c_str(), DARG_HOST_MODULE_NAME);
            violated = true;
            break;
          }
      });
      return !violated; // stop iterating modules once violated
    },
    "*");

  return violated;
}


/****************************************************************************/
/* The async load + compile + simulate job                                  */
/****************************************************************************/

class DasLoadAndCompileJob final : public cpujobs::IJob
{
public:
  DasScriptsData *dasMgr = nullptr;
  GuiScene *guiScene = nullptr;
  DasScript *script = nullptr; // main-thread-only; nulled by ~DasScript when orphaned
  eastl::string fileName;
  CodeOfPolicies policies;

  // produced on the worker in doJob(), consumed on the main thread in releaseJob()
  ProgramPtr program;
  smart_ptr<DargContext> ctx;
  eastl::string errorText;
  bool compiledOk = false;

  DasLoadAndCompileJob(DasScriptsData *mgr, GuiScene *scene, DasScript *s, const char *fn, const CodeOfPolicies &pol) :
    dasMgr(mgr), guiScene(scene), script(s), fileName(fn), policies(pol)
  {
    interlocked_increment(dasMgr->numJobsInFlight);
  }

  virtual const char *getJobName(bool &copy) const override
  {
    copy = false;
    return "DargDasLoadAndCompileJob";
  }

  // ---- worker thread -------------------------------------------------------
  virtual void doJob() override
  {
    WinAutoLock lock(das_compile_lock());
    das::daScriptEnvironmentGuard envGuard(dasMgr->dasEnv, dasMgr->dasEnv);
    NoDebugStackCollector noStackCollector;

    // Must outlive compile+simulate yet be destroyed before envGuard/lock, so the
    // host TLS restore stays inside the guarded region. Null in non-host -> no-op.
    struct HostCompileScope
    {
      void *h;
      HostCompileScope() : h(das_async_compile_scope_open ? das_async_compile_scope_open() : nullptr) {}
      ~HostCompileScope()
      {
        if (das_async_compile_scope_close)
          das_async_compile_scope_close(h);
      }
    } hostScope;

    DasCompileActiveScope compileActive; // marks the shared-state window for the breadcrumb

#if DAGOR_DBGLEVEL > 0
    // Force reload. Here (under das_compile_lock), not in load_das() on the main thread:
    // compileDaScript() below reads the shared fAccess cache an unlocked mutation would race.
    dasMgr->fAccess->invalidateFileInfo(fileName);
#endif

    program = compileDaScript(fileName.c_str(), dasMgr->fAccess, dasMgr->logWriter, *dasMgr->moduleGroup, policies);
    if (!program || program->failed())
    {
      buildError("Failed to compile");
      return;
    }

    // Enforce init-purity between compile and simulate (simulate runs init on
    // this worker). Loud FAILED, never a crash; runs in all hosts.
    eastl::string lintErr;
    if (darg_das_init_purity_violation(program.get(), lintErr))
    {
      buildError(lintErr.c_str());
      return;
    }

    auto c = make_smart<DargContext>(program->getContextStackSize());

    // compile+simulate (incl. [init]/global init) run together per script, the proven
    // ECS sequence. NOT split: Function::index is consumed by this simulate before the
    // next compile re-stamps it, so no cross-script index corruption (no gate needed).
    // Safe because daRg .das init is host-VM-free - enforced by the lint above.
    if (!program->simulate(*c, dasMgr->logWriter))
    {
      buildError("Failed to simulate");
      return;
    }

    ctx = c;
    compiledOk = true;
  }

  // ---- main thread ---------------------------------------------------------
  virtual void releaseJob() override
  {
    {
      WinAutoLock lock(das_compile_lock());
      das::daScriptEnvironmentGuard envGuard(dasMgr->dasEnv, dasMgr->dasEnv);

      if (compiledOk)
      {
        // compile+simulate+[init] already ran on the worker; for a GC'd
        // (orphaned) handle only the *publish* is guarded by script != null.
        integrateReady();
      }
      else
      {
        guiScene->errorMessageWithCb(errorText.c_str());
        if (script)
          script->setState(DasScript::FAILED);
      }

      if (script)
        script->loadingJob = nullptr;
    }

    // Run module-group teardown deferred by cleanupAfterReload, on the last job.
    if (interlocked_decrement(dasMgr->numJobsInFlight) == 0 && dasMgr->pendingModuleGroupCleanup)
    {
      dasMgr->pendingModuleGroupCleanup = false;
      dasMgr->cleanupModuleGroupLocked(); // we are on the main thread (release_done_jobs / inline fallback)
    }

    delete this;
  }

private:
  void buildError(const char *what)
  {
    errorText.sprintf("%s '%s'", what, fileName.c_str());
    if (program)
      for (const Error &e : program->errors)
      {
        errorText += "\n=============\n";
        errorText += das::reportError(e.at, e.what, e.extra, e.fixme, e.cerr).c_str();
      }
  }

  void integrateReady()
  {
    if (dasMgr->aotMode == AotMode::AOT && !program->aotErrors.empty())
    {
      logwarn("daScript: failed to link cpp aot <%s>\n", fileName.c_str());
      if (dasMgr->needAotErrorLog == LogAotErrors::YES)
        for (auto &err : program->aotErrors)
          logwarn(das::reportError(err.at, err.what, err.extra, err.fixme, err.cerr).c_str());
    }

#if DAGOR_DBGLEVEL > 0
    if (const uint64_t heapLimit = ctx->heap->getLimit())
    {
      ctx->heapLimit = heapLimit;
      ::debug("daScript: set heap limit %@ for script <%s>", heapLimit, fileName.c_str());
    }
    if (const uint64_t stringHeapLimit = ctx->stringHeap->getLimit())
    {
      ctx->stringHeapLimit = stringHeapLimit;
      ::debug("daScript: set string heap limit %@ for script <%s>", stringHeapLimit, fileName.c_str());
    }
#endif

    process_loaded_script(*ctx, fileName.c_str());

    if (!ctx->persistent)
    {
      program->library.foreach(
        [this](das::Module *mod) {
          mod->globals.foreach([this](auto globVar) {
            if (globVar && globVar->used && !globVar->type->isNoHeapType())
            {
              const auto ignoreMark = globVar->annotation.find("ignore_heap_usage", das::Type::tBool);
              if (ignoreMark == nullptr || !ignoreMark->bValue)
                logerr("global variable '%s' in script <%s> requires heap memory, but heap memory is regularly cleared."
                       "Only value types/fixed arrays are supported in this case",
                  globVar->name, fileName.c_str());
            }
          });
          return true;
        },
        "*");
    }

    {
      int aotFn = 0;
      int intFn = 0;
      for (int i = 0, is = ctx->getTotalFunctions(); i != is; ++i)
        if (SimFunction *fn = ctx->getFunction(i))
        {
          if (fn->builtin)
            continue;
          if (fn->aot)
            ++aotFn;
          else
          {
            ++intFn;
            if (dasMgr->aotMode == AotMode::AOT)
              ::debug("daScript: function %s interpreted in file %s", fn->name, fileName.c_str());
          }
        }
      ::debug("daScript: <%s> loaded: %d interpreted func, %d aot func", fileName.c_str(), intFn, aotFn);
    }

    // Orphaned fire-and-forget load (the Sqrat handle was GC'd before the job
    // finished): init already ran on the worker, so just drop ctx/program.
    if (!script)
      return;

    ctx->name = dd_get_fname(fileName.c_str());
    script->ctx = ctx;
    if (program->options.getBoolOption("rtti", program->policies.rtti))
    {
      logerr("daScript: script <%s> uses RTTI, which is not supported in rgui scripts. Remove this error if you are "
             "sure you need RTTI",
        fileName.c_str());
      script->program = program;
    }
    // ctx/program written above; publish READY *last* with a release store
    // (every reader acquire-loads state via isReady()/isLoading()).
    script->setState(DasScript::READY);
  }
};


DasScript::~DasScript()
{
  if (owner)
  {
    auto &live = owner->liveScripts;
    auto it = eastl::find(live.begin(), live.end(), this);
    if (it != live.end())
      live.erase_unsorted(it);
    owner = nullptr;
  }

  // Orphan handshake runs on the GuiScene API thread (~DasScript / releaseJob; doJob never
  // touches these). Nothing in flight -> nothing to orphan; skip the lock (else reload-time GC
  // stalls behind an unrelated worker compile).
  if (!loadingJob)
    return;
  WinAutoLock lock(das_compile_lock()); // conservative, mirrors releaseJob()'s lock
  loadingJob->script = nullptr;
}


/****************************************************************************/
/****************************************************************************/


void DasScriptsData::initModuleGroup()
{
  G_ASSERT(dasEnv);
  das::daScriptEnvironmentGuard envGuard(dasEnv, dasEnv);
  if (!fAccess)
  {
    if (!dasProject.empty())
      fAccess = make_smart<DargFileAccess>(dasProject.c_str());
    else
      fAccess = make_smart<DargFileAccess>();
  }
  moduleGroup = eastl::make_unique<das::ModuleGroup>();
  dbgInfoHelper = eastl::make_unique<DebugInfoHelper>();
  typeGuiContextRef = dbgInfoHelper->makeTypeInfo(nullptr, makeType<StdGuiRender::GuiContext &>(*moduleGroup));
  typeConstElemRenderDataRef = dbgInfoHelper->makeTypeInfo(nullptr, makeType<const ElemRenderData &>(*moduleGroup));
  typeConstRenderStateRef = dbgInfoHelper->makeTypeInfo(nullptr, makeType<const RenderState &>(*moduleGroup));
  typeConstElemRef = dbgInfoHelper->makeTypeInfo(nullptr, makeType<const Element &>(*moduleGroup));
  typeConstPropsRef = dbgInfoHelper->makeTypeInfo(nullptr, makeType<const Properties &>(*moduleGroup));

  if (asyncJobMgrId < 0)
    asyncJobMgrId = cpujobs::create_virtual_job_manager(256 << 10, WORKER_THREADS_AFFINITY_MASK, "DargDasJobMgr");
}


DasScriptsData::DasScriptsData(const char *das_project)
{
  dasProject = das_project ? das_project : "";
  if (is_das_inited())
  {
    dasEnv = *daScriptEnvironment::bound;
    G_ASSERT(dasEnv);
    initModuleGroup();
  }
}


DasScriptsData::~DasScriptsData()
{
  waitAllJobsDone();
  if (asyncJobMgrId >= 0)
  {
    cpujobs::destroy_virtual_job_manager(asyncJobMgrId, true);
    asyncJobMgrId = -1;
  }
}


void DasScriptsData::initDasEnvironment(TInitDasEnv init_callback)
{
  if (!init_callback)
    logerr("Das environment initialization callback can't be null");

  shutdownDasEnvironment();
  dasEnv = new das::daScriptEnvironment();
  isOwnedDasEnv = true;
  initCallback = init_callback;

  {
    das::daScriptEnvironmentGuard envGuard(dasEnv, dasEnv);
    init_callback();
  }
  initModuleGroup();
}


void DasScriptsData::shutdownDasEnvironment()
{
  // Final teardown: finish/cancel everything before the env disappears.
  waitAllJobsDone();

  WinAutoLock lock(das_compile_lock());
  {
    das::daScriptEnvironmentGuard envGuard(dasEnv, dasEnv);
    // Drop script ctx/program before the modules (see liveScripts decl); jobs already drained.
    for (DasScript *s : liveScripts)
    {
      s->setState(DasScript::FAILED);
      s->ctx.reset();
      s->program.reset();
    }

    // module group must be destroyed before dasEnv
    moduleGroup.reset();
  }
  if (isOwnedDasEnv)
  {
    {
      das::daScriptEnvironmentGuard envGuard(dasEnv, dasEnv);
      das::Module::Shutdown();
    }

    isOwnedDasEnv = false;
    dasEnv = nullptr;
  }
}


void DasScriptsData::resetBeforeReload()
{
  // Drains the PREVIOUS scene's jobs before the env is recreated. Does NOT make
  // the new scene synchronous - its jobs are queued later, during the run.
  waitAllJobsDone();

  WinAutoLock lock(das_compile_lock());
  if (isOwnedDasEnv)
    initDasEnvironment(initCallback); // recreate environment to lose all stored shared modules
  if (fAccess)
    fAccess->reset();
}


void DasScriptsData::cleanupAfterReload()
{
  // Jobs may still be compiling against moduleGroup/fAccess; draining here would
  // make every first load synchronous, so defer cleanup to the last job.
  if (interlocked_acquire_load(numJobsInFlight) > 0)
  {
    pendingModuleGroupCleanup = true;
    return;
  }
  cleanupModuleGroupLocked();
}


void DasScriptsData::cleanupModuleGroupLocked()
{
  WinAutoLock lock(das_compile_lock());
  // Free the parser source cache only after the last compile (shared fAccess).
  if (fAccess)
    fAccess->freeSourceData();

  das::daScriptEnvironmentGuard envGuard(dasEnv, dasEnv);
  if (moduleGroup)
    moduleGroup->reset();
  if (isOwnedDasEnv)
    das::Module::ClearSharedModules();
}


void DasScriptsData::waitAllJobsDone()
{
  // Must NOT hold das_compile_lock: release_done_jobs() below runs releaseJob(),
  // which takes it (would deadlock the worker). Caller is the GuiScene API thread.
  while (interlocked_acquire_load(numJobsInFlight) > 0)
  {
    cpujobs::release_done_jobs();
    if (interlocked_acquire_load(numJobsInFlight) > 0)
      sleep_msec(1);
  }
}


void DasScriptsData::waitScriptResolved(const DasScript *s)
{
  // Sync fallback: blocks only until THIS script is published. release_done_jobs()
  // is global, so other subsystems' releaseJob can run reentrantly here (tolerated).
  while (s->isLoading())
  {
    cpujobs::release_done_jobs();
    if (s->isLoading())
      sleep_msec(1);
  }
}


void set_das_loading_settings(HSQUIRRELVM vm, AotMode aot_mode, LogAotErrors need_aot_error_log)
{
  GuiScene *guiScene = GuiScene::get_from_sqvm(vm);
  G_ASSERT(guiScene);

  if (DasScriptsData *dasMgr = guiScene->dasScriptsData.get())
  {
    dasMgr->aotMode = aot_mode;
    dasMgr->needAotErrorLog = need_aot_error_log;
  }
}


static SQInteger load_das(HSQUIRRELVM vm)
{
  const char *filename = nullptr;
  sq_getstring(vm, 2, &filename);

  GuiScene *guiScene = GuiScene::get_from_sqvm(vm);
  G_ASSERT(guiScene);
  DasScriptsData *dasMgr = guiScene->dasScriptsData.get();
  if (!dasMgr)
    return sq_throwerror(vm, "Not using daScript in this VM");

  ::debug("daScript: load script <%s> aot:%d", filename, dasMgr->aotMode == AotMode::AOT ? 1 : 0);
  CodeOfPolicies policies;
  policies.aot = dasMgr->aotMode == AotMode::AOT;
  policies.fail_on_no_aot = false;
  policies.fail_on_lack_of_aot_export = true;
  policies.no_aliasing = true;
  policies.strict_unsafe_delete = true; //-V1048
  //  policies.ignore_shared_modules = hard_reload;
  // NB: policies.no_init is deliberately NOT set here. Init runs on the cpujobs
  // worker so it must stay host-VM-free, but no_init is the wrong gate: it is a
  // hard error that would also reject the legitimate non-constant pure
  // initializers daRg uses (E3DCOLOR/float4/fixed_array/ref_time_ticks/const
  // math). That requirement is enforced instead by the init-purity check
  // (darg_das_init_purity_violation, run between compile and simulate).
  if (dgs_get_settings() != nullptr)
  {
    policies.gen2_make_syntax = dgs_get_settings()->getBool("game_das_gen_2_make_syntax", false);
  }

  // NB: the dev-only fAccess->invalidateFileInfo() moved into doJob() (under the compile
  // lock) - mutating the shared fAccess cache here raced an in-flight worker compile.

  DasScript *s = new DasScript();
  s->owner = dasMgr;
  dasMgr->liveScripts.push_back(s);

  // Push the (still LOADING) handle to Quirrel immediately - this is what makes load_das async.
  sq_pushobject(vm, Sqrat::ClassType<DasScript>::getClassData(vm)->classObj); //-V522
  G_VERIFY(SQ_SUCCEEDED(sq_createinstance(vm, -1)));
  Sqrat::ClassType<DasScript>::SetManagedInstance(vm, -1, s);

  auto *job = new DasLoadAndCompileJob(dasMgr, guiScene, s, filename, policies);
  s->loadingJob = job;

  if (dasMgr->asyncJobMgrId >= 0 && cpujobs::add_job(dasMgr->asyncJobMgrId, job))
  {
    // queued; DasLoadAndCompileJob::releaseJob() will integrate + publish on the main thread.
  }
  else
  {
    // Job manager unavailable / add_job failed -> the manager does NOT own the job.
    // Run it inline on the calling (main) thread; same terminal state as the old sync loader.
    // releaseJob() self-deletes (see its tail) - do NOT delete here (would double-free).
    job->doJob();
    job->releaseJob();
  }

  return 1;
}


void bind_das(Sqrat::Table &exports)
{
  HSQUIRRELVM vm = exports.GetVM();

  Sqrat::Class<DasScript, Sqrat::NoConstructor<DasScript>> sqDasScript(vm, "DasScript");

  exports.SquirrelFunc("load_das", load_das, 2, ".s");
}

} // namespace darg
