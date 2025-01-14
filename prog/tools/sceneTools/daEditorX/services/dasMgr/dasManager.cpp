// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <EASTL/unique_ptr.h>
#include <dasModules/dasFsFileAccess.h>
#include <daScript/misc/smart_ptr.h>
#include <daECS/core/entityManager.h>
#include <memory/dag_memStat.h>
#include <ecs/scripts/dasEs.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>

namespace bind_dascript
{
extern HotReload globally_hot_reload;
}

das::smart_ptr<das::FileAccess> get_file_access(char *pak)
{
  return pak ? das::make_smart<bind_dascript::DagFileAccess>(pak, bind_dascript::globally_hot_reload)
             : das::make_smart<bind_dascript::DagFileAccess>(bind_dascript::globally_hot_reload);
}

// hardcoded paths for asset viewer. TODO: read from application.blk
static const char *get_das_init_entry_point() { return "%asset_viewer/asset_viewer_init.das"; }
static const char *get_das_entry_point() { return "%asset_viewer/asset_viewer.das"; }
static const char *get_das_project() { return "%asset_viewer/asset_viewer.das_project"; }
static int get_game_das_loading_threads() { return 1; }


void os_debug_break() { debug("das: script break"); }

void global_init_das()
{
  const bool auto_hot_reload = true;

  bind_dascript::set_das_root("."); // use current dir as root path


  const bool enableAot = false;
  bind_dascript::init_das(enableAot ? bind_dascript::AotMode::AOT : bind_dascript::AotMode::NO_AOT,
    auto_hot_reload ? bind_dascript::HotReload::ENABLED : bind_dascript::HotReload::DISABLED,
    enableAot ? bind_dascript::LogAotErrors::YES : bind_dascript::LogAotErrors::NO);

  bind_dascript::init_scripts(auto_hot_reload ? bind_dascript::HotReload::ENABLED : bind_dascript::HotReload::DISABLED,
    bind_dascript::LoadDebugCode::YES, get_das_project());

  bind_dascript::set_load_threads_num(get_game_das_loading_threads());
}

void pull_das()
{
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC
  NEED_MODULE(Module_FIO)
  NEED_MODULE(Module_Network)
  NEED_MODULE(Module_JobQue)
  NEED_MODULE(Module_UriParser)
#endif
#if DAGOR_DBGLEVEL > 0
  NEED_MODULE(Module_Debugger)
#endif
  NEED_MODULE(DagorTime)
  NEED_MODULE(DagorRandom)
  NEED_MODULE(DagorMathUtils)
  NEED_MODULE(DagorConsole)
  NEED_MODULE(DagorFiles)
  NEED_MODULE(DagorFindFiles)
  NEED_MODULE(DagorResources)
  NEED_MODULE(ECSGlobalTagsModule)
  NEED_MODULE(AppModule)

  NEED_MODULE(Module_dasIMGUI)
  NEED_MODULE(Module_dasIMGUI_NODE_EDITOR)
  NEED_MODULE(DagorImguiModule) // imgui

  NEED_MODULE(GeomNodeTreeModule)
#if HAS_PHYS
  NEED_MODULE(PhysDeclModule)
  NEED_MODULE(AnimV20) // animchar
#endif

  NEED_MODULE(DaProfilerModule) // daprofiler

  NEED_MODULE(Module_StdDlg) // file dialog
}

void require_project_specific_debugger_modules() {} // stub for dng debugger

void init_das()
{
  bind_dascript::pull_das();
  pull_das();
  das::Module::Initialize();
}


void reload_das_init()
{
  bind_dascript::load_event_files(true);
  if (const char *dasInitPoint = get_das_init_entry_point())
  {
    if (dd_file_exist(dasInitPoint))
    {
      debug("dascript: load init script '%s'", dasInitPoint);
      bind_dascript::start_multiple_scripts_loading();
      bind_dascript::load_das_script(dasInitPoint);
      bind_dascript::drop_multiple_scripts_loading(); // unset das_is_in_init_phase, es_reset_order should be called later
      bind_dascript::main_thread_post_load();
    }
    else
    {
      logerr("dascript: no init script '%s'", dasInitPoint);
    }
  }
  else
  {
    debug("dascript: init script is empty, skip initialization");
  }
  bind_dascript::load_event_files(false);
}

void init_das_entry_point()
{
  size_t memUsed = dagor_memory_stat::get_memory_allocated(true);
  reload_das_init();
  memUsed = dagor_memory_stat::get_memory_allocated(true) - memUsed;

  if (const char *dasEntryPoint = get_das_entry_point())
  {
    if (dd_file_exist(dasEntryPoint))
    {
      bind_dascript::start_multiple_scripts_loading();
      bind_dascript::load_entry_script(dasEntryPoint, &init_das, bind_dascript::LoadEntryScriptCtx{int64_t(memUsed)});
      bind_dascript::drop_multiple_scripts_loading(); // unset das_is_in_init_phase, es_reset_order calls in the end of function
      bind_dascript::main_thread_post_load();
    }
    else
    {
      logerr("daScript entry point <%s> can't be found", dasEntryPoint);
    }
  }
  else
  {
    debug("dascript: entry point is empty, skip starting");
  }
}

bind_dascript::ReloadResult das_try_hot_reload()
{
  static unsigned frame = 0;
  if (((++frame) & 31) != 0) //
    return bind_dascript::ReloadResult::NO_CHANGES;
  return bind_dascript::reload_all_changed();
}


class DasManager : public IEditorService
{
public:
  DasManager()
  {
    if (!IDaEditor3Engine::get().registerService(this))
    {
      logerr("Failed to register DAS Manager service.");
      return;
    }
    global_init_das();
    init_das();
    init_das_entry_point();
    if (g_entity_mgr)
      g_entity_mgr->resetEsOrder();
  }

  virtual const char *getServiceName() const override { return serviceName; };
  virtual const char *getServiceFriendlyName() const override { return friendlyServiceName; };

  virtual void setServiceVisible(bool vis) override{};
  virtual bool getServiceVisible() const override { return true; };

  virtual void actService(float dt) override
  {
    auto hotReloadResult = das_try_hot_reload();
    if (hotReloadResult == bind_dascript::ReloadResult::RELOADED)
      DAEDITOR3.conNote("das scripts were reloaded");
    if (hotReloadResult == bind_dascript::ReloadResult::ERRORS)
      DAEDITOR3.conError("das scripts can't reloaded, check errors in log");
  }

  virtual void beforeRenderService() override{};
  virtual void renderService() override{};
  virtual void renderTransService() override{};

  virtual void *queryInterfacePtr(unsigned huid) override { return nullptr; }

private:
  static constexpr const char *serviceName = "dasManager";
  static constexpr const char *friendlyServiceName = nullptr;
};

eastl::unique_ptr<DasManager> dasManager;

void init_das_mgr_service(const DataBlock &app_blk)
{
  if (app_blk.getBool("initDaScript", false))
  {
    const char *das_project = get_das_project();
    if (dd_file_exist(das_project))
      dasManager = eastl::make_unique<DasManager>();
    else
      logerr("dascript: no das_project '%s'", das_project);
  }
}