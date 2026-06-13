// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <osApiWrappers/dag_atomic.h>

namespace Sqrat
{
class Table;
}

struct SQVM;
typedef struct SQVM *HSQUIRRELVM;

namespace darg
{
typedef void (*TInitDasEnv)();

enum class AotMode
{
  NO_AOT,
  AOT,
};

enum class LogAotErrors
{
  NO,
  YES,
};

void set_das_loading_settings(HSQUIRRELVM vm, AotMode aot_mode, LogAotErrors need_aot_error_log);

class DasLogWriter : public das::TextWriter
{
  virtual void output() override;
  uint64_t pos = 0;
};

class DasLoadAndCompileJob;
class DasScriptsData;

// Async loading model:
//   - compile + simulate + [init]/global-init all run together on the cpujobs worker,
//     sequentially per script - exactly the proven ECS DascriptLoadJob sequence. No
//     compile/simulate split -> Function::index is consumed by each script's simulate before the
//     next compile re-stamps it, so cross-script index corruption cannot occur (no gate, no
//     daScript-core change). Safe because daRg .das global init is host-VM-free
//     (enforced by the init-purity lint in dasScripts.cpp).
//   - DasLoadAndCompileJob::releaseJob() (main thread) only runs diagnostics + publish.
//   - state is published with release/acquire; readers use isReady()/isLoading().
struct DasScript
{
  enum State
  {
    LOADING = 0,
    READY = 1,
    FAILED = 2,
  };

  das::smart_ptr<das::Context> ctx;
  das::ProgramPtr program;

  // Written last by releaseJob() (main thread) with a release store; every reader
  // (DasFunction, RobjDasCanvas) must go through isReady()/isLoading()/isFailed().
  volatile int state = LOADING;

  // Main-thread-only orphan link. If the Sqrat handle is GC'd before the job finishes,
  // ~DasScript() nulls the job's back-pointer: the job still runs [init],
  // but must not publish into this freed handle.
  DasLoadAndCompileJob *loadingJob = nullptr;

  DasScriptsData *owner = nullptr; // back-ref for liveScripts (un)registration

  bool isReady() const { return interlocked_acquire_load(state) == int(READY); }
  bool isLoading() const { return interlocked_acquire_load(state) == int(LOADING); }
  bool isFailed() const { return interlocked_acquire_load(state) == int(FAILED); }
  void setState(State s) { interlocked_release_store(state, int(s)); }

  ~DasScript();
};


class DasScriptsData
{
public:
  DasScriptsData(const char *das_project = nullptr);
  ~DasScriptsData();

  void initDasEnvironment(TInitDasEnv init_callback);
  void shutdownDasEnvironment();
  void initModuleGroup();
  void resetBeforeReload();
  void cleanupAfterReload();

  // Drain (finish + release) every in-flight load job on the calling (main) thread.
  // Does NOT hold the global das compile lock while waiting (the worker needs it to finish).
  void waitAllJobsDone();

  // Sync fallback: block only until THIS script is published, not the whole queue.
  void waitScriptResolved(const DasScript *s);

  // The destructive part of cleanupAfterReload (freeSourceData + moduleGroup->reset +
  // ClearSharedModules). Runs only when numJobsInFlight==0 - either immediately, or deferred to
  // the last job's releaseJob() via pendingModuleGroupCleanup. Takes the global das compile lock.
  void cleanupModuleGroupLocked();

public:
  das::string dasProject;
  das::daScriptEnvironment *dasEnv = nullptr;
  bool isOwnedDasEnv = false;
  TInitDasEnv initCallback = nullptr;

  das::unique_ptr<das::ModuleGroup> moduleGroup;
  DasLogWriter logWriter;
  das::unique_ptr<das::DebugInfoHelper> dbgInfoHelper;
  das::smart_ptr<das::ModuleFileAccess> fAccess;
  AotMode aotMode = AotMode::AOT;
  LogAotErrors needAotErrorLog = LogAotErrors::YES;

  // cpujobs virtual job manager (single dedicated worker thread; jobs run serially).
  int asyncJobMgrId = -1;
  volatile int numJobsInFlight = 0;
  bool pendingModuleGroupCleanup = false;

  // shutdownDasEnvironment releases each script's ctx/program before module teardown: the
  // handles are script-referenced (caches) and outlive env reset, so their dtors must not drop
  // ctx/program against freed modules. GuiScene API thread only (no lock).
  eastl::vector<DasScript *> liveScripts;

  das::TypeInfo *typeGuiContextRef = nullptr;
  das::TypeInfo *typeConstElemRenderDataRef = nullptr;
  das::TypeInfo *typeConstRenderStateRef = nullptr;
  das::TypeInfo *typeConstElemRef = nullptr;
  das::TypeInfo *typeConstPropsRef = nullptr;
};


// Crash breadcrumb: true while a worker das compile/simulate mutates shared state. Read on
// das exception paths to flag eval/compile overlap (the "eval is env-free" invariant is
// unenforced).
bool darg_das_compile_in_flight();

void bind_das(Sqrat::Table &exports);

} // namespace darg
