// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dasScripts.h"
#include <daRg/dasBinding.h>
#include <memory/dag_dbgMem.h>
#include "guiScene.h"
#include <dasModules/dasFsFileAccess.h>

#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>

using namespace das;


namespace darg
{

class NoDebugStackCollector
{
public:
  bool wasStackFill;
  NoDebugStackCollector() { wasStackFill = DagDbgMem::enable_stack_fill(false); }
  ~NoDebugStackCollector() { DagDbgMem::enable_stack_fill(wasStackFill); }
};

struct DargContext final : das::Context
{
  DargContext(GuiScene *scene_, uint32_t stackSize) : das::Context(stackSize), scene(scene_) {}

  DargContext(Context &ctx, uint32_t category) = delete;

  void to_out(const das::LineInfo *, int level, const char *message) override
  {
    const int logLevel = level >= das::LogLevel::error     ? LOGLEVEL_ERR
                         : level >= das::LogLevel::warning ? LOGLEVEL_WARN
                                                           : LOGLEVEL_DEBUG;
    if (logLevel == LOGLEVEL_ERR)
      scene->errorMessageWithCb(message);
    else
      ::logmessage(logLevel, "daRg-das: %s", message);
  }

  GuiScene *scene = nullptr;
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


das::FileInfo *DargFileAccess::getNewFileInfo(const das::string &fname)
{
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
  {
    das::daScriptEnvironmentGuard envGuard(dasEnv, dasEnv);
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
  if (isOwnedDasEnv)
    initDasEnvironment(initCallback); // recreate environment to lose all stored shared modules
  if (fAccess)
    fAccess->reset();
}


void DasScriptsData::cleanupAfterReload()
{
  if (fAccess)
    fAccess->freeSourceData();

  {
    das::daScriptEnvironmentGuard envGuard(dasEnv, dasEnv);
    if (moduleGroup)
      moduleGroup->reset();
    if (isOwnedDasEnv)
      das::Module::ClearSharedModules();
  }
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
  das::daScriptEnvironmentGuard envGuard(guiScene->dasScriptsData->dasEnv, guiScene->dasScriptsData->dasEnv);
  DasScriptsData *dasMgr = guiScene->dasScriptsData.get();
  if (!dasMgr)
    return sq_throwerror(vm, "Not using daScript in this VM");

  ::debug("daScript: load script <%s> aot:%d", filename, dasMgr->aotMode == AotMode::AOT ? 1 : 0);
  CodeOfPolicies policies;
  policies.aot = dasMgr->aotMode == AotMode::AOT;
  policies.fail_on_no_aot = false;
  policies.fail_on_lack_of_aot_export = true;
  policies.no_unused_function_arguments = true;
  policies.no_unused_block_arguments = true;
  policies.no_aliasing = true;
  policies.strict_unsafe_delete = true;
  //  policies.ignore_shared_modules = hard_reload;
  if (dgs_get_settings() != nullptr)
  {
    policies.gen2_make_syntax = dgs_get_settings()->getBool("game_das_gen_2_make_syntax", false);
  }

  eastl::string strFileName(filename);

  dasMgr->fAccess->invalidateFileInfo(strFileName); // force reload, let quirrel script manage lifetime

  ProgramPtr program;
  {
    NoDebugStackCollector noStackCollector;

    program = compileDaScript(strFileName, dasMgr->fAccess, guiScene->dasScriptsData->logWriter,
      *guiScene->dasScriptsData->moduleGroup, policies);
  }

  if (program->failed())
  {
    eastl::string details(eastl::string::CtorSprintf{}, "Failed to compile '%s'", filename);

    for (const Error &e : program->errors)
    {
      details += "\n=============\n";
      details += das::reportError(e.at, e.what, e.extra, e.fixme, e.cerr);
    }
    guiScene->errorMessageWithCb(details.c_str());
    return 0;
  }

  if (dasMgr->aotMode == AotMode::AOT && !program->aotErrors.empty())
  {
    logwarn("daScript: failed to link cpp aot <%s>\n", filename);
    if (dasMgr->needAotErrorLog == LogAotErrors::YES)
      for (auto &err : program->aotErrors)
      {
        logwarn(das::reportError(err.at, err.what, err.extra, err.fixme, err.cerr).c_str());
      }
  }

  auto ctx = make_smart<DargContext>(guiScene, program->getContextStackSize());

  if (!program->simulate(*ctx, guiScene->dasScriptsData->logWriter))
  {
    eastl::string details(eastl::string::CtorSprintf{}, "Failed to simulate '%s'", filename);

    for (const Error &e : program->errors)
    {
      details += "\n=============\n";
      details += reportError(e.at, e.what, e.extra, e.fixme, e.cerr);
    }

    guiScene->errorMessageWithCb(details.c_str());
    return 0;
  }

  process_loaded_script(*ctx, filename);

  DasScript *s = new DasScript();
  s->ctx = ctx;

  if (program->options.getBoolOption("rtti", false))
  {
    logerr("daScript: script <%s> uses RTTI, which is not supported in rgui scripts. Remove this error if you are sure you need RTTI",
      filename);
    s->program = program;
  }

  if (!ctx->persistent)
  {
    program->library.foreach(
      [&strFileName](das::Module *mod) {
        mod->globals.foreach([&strFileName](auto globVar) {
          if (globVar && globVar->used && !globVar->type->isNoHeapType())
          {
            const auto ignoreMark = globVar->annotation.find("ignore_heap_usage", das::Type::tBool);
            if (ignoreMark == nullptr || !ignoreMark->bValue)
              logerr("global variable '%s' in script <%s> requires heap memory, but heap memory is regularly cleared."
                     "Only value types/fixed arrays are supported in this case",
                globVar->name, strFileName.c_str());
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
    {
      if (SimFunction *fn = ctx->getFunction(i))
      {
        if (fn->aot)
          ++aotFn;
        else
        {
          ++intFn;
          if (dasMgr->aotMode == AotMode::AOT)
            ::debug("daScript: function %s interpreted in file %s", fn->name, filename);
        }
      }
    }
    ::debug("daScript: <%s> loaded: %d interpreted func, %d aot func", filename, intFn, aotFn);
  }

  // Sqrat::ClassType<DasScript>::PushNativeInstance(vm, s);

  sq_pushobject(vm, Sqrat::ClassType<DasScript>::getClassData(vm)->classObj);
  G_VERIFY(SQ_SUCCEEDED(sq_createinstance(vm, -1)));
  Sqrat::ClassType<DasScript>::SetManagedInstance(vm, -1, s);

  return 1;
}


void bind_das(Sqrat::Table &exports)
{
  HSQUIRRELVM vm = exports.GetVM();

  Sqrat::Class<DasScript, Sqrat::NoConstructor<DasScript>> sqDasScript(vm, "DasScript");

  exports.SquirrelFunc("load_das", load_das, 2, ".s");
}

} // namespace darg
