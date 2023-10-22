#include <perfMon/dag_cpuFreq.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/random/dag_random.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_Point3.h>
#include <debug/dag_logSys.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_statDrv.h>
#include <daECS/core/internal/performQuery.h>
#include <ecs/scripts/dasEs.h>
#include <startup/dag_globalSettings.h>
#include <daECS/core/sharedComponent.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_findFiles.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>

#include <ioSys/dag_dataBlock.h>
#include <ecs/io/blk.h>

#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>
#include <dasModules/dasFsFileAccess.h>
#include "unitModule.h"

extern bool NEED_DAS_AOT_COMPILE;

static constexpr int DEF_RUNS = 100;
namespace d3d
{
int driver_command(int, void *, void *, void *) { return 0; }
void beginEvent(const char *) {}
void endEvent() {}
void *get_device() { return 0; }
} // namespace d3d

struct SomeComponent
{
  SomeComponent() { debug("created some"); }
  static void requestResources(const char *, const ecs::resource_request_cb_t &res_cb)
  {
    res_cb("some_name", 0);
    debug("requested");
  }
};
ECS_DECLARE_RELOCATABLE_TYPE(SomeComponent);
ECS_REGISTER_RELOCATABLE_TYPE(SomeComponent, nullptr);


ECS_BROADCAST_EVENT_TYPE(EventCPPSimpleTestEvent, float, int)
ECS_REGISTER_EVENT(EventCPPSimpleTestEvent)

ECS_BROADCAST_EVENT_TYPE(EventCPPTestEvent, float, int, SimpleString)
ECS_BROADCAST_EVENT_TYPE(EventStart)
ECS_BROADCAST_EVENT_TYPE(EventStart2)
ECS_BROADCAST_EVENT_TYPE(EventEnd)

ECS_REGISTER_EVENT(EventCPPTestEvent)
ECS_REGISTER_EVENT(EventStart)
ECS_REGISTER_EVENT(EventStart2)
ECS_REGISTER_EVENT(EventEnd)

ECS_BROADCAST_EVENT_TYPE(EventCPPTestStringEvent, SimpleString)
ECS_REGISTER_EVENT(EventCPPTestStringEvent)

ECS_BROADCAST_EVENT_TYPE(EventEmpty)
ECS_REGISTER_EVENT(EventEmpty)

static bool had_errors = 0;

void os_debug_break()
{
  logerr("script break");
  had_errors = true;
}
void os_message_box(const char *s, const char *s2, int) { logerr("%s:%s", s, s2); }
static void my_fatal_handler(const char *title, const char *msg, const char *call_stack)
{
  printf("%s:%s\n%s", title, msg, call_stack);
  exit(1);
}

static int log_callback(int lev_tag, const char * /*fmt*/, const void * /*arg*/, int /*anum*/, const char * /*ctx_file*/,
  int /*ctx_line*/)
{
  if (lev_tag == LOGLEVEL_ERR || lev_tag == LOGLEVEL_FATAL)
    had_errors = true;
  return 1;
}

#include <osApiWrappers/dag_symHlp.h>
#include <osApiWrappers/dag_dbgStr.h> //set_debug_console_handle
#if _TARGET_PC_WIN
#include <windows.h> //set_debug_console_handle
#endif
extern bool dgs_execute_quiet;

int myMain2(int startArgC)
{
  if (df_get_real_name("entities.blk"))
  {
    ecs::TemplateRefs trefs;
    SimpleString fname("entities.blk");
    ecs::load_templates_blk(make_span_const(&fname, 1), trefs);
    g_entity_mgr->addTemplates(trefs);
  }

  if (dd_dir_exist(dgs_argv[startArgC]))
  {
    Tab<SimpleString> file_list;
    if (find_files_in_folder(file_list, dgs_argv[startArgC], "*.das", false, true, true))
    {
      for (auto &s : file_list)
      {
        if (!bind_dascript::load_das_script(s.c_str()))
        {
          printf("Can't compile <%s>\n", s.c_str());
          return 1;
        }
        printf("Successfully compiled <%s>\n", s.c_str());
      }
    }
    else
    {
      printf("Successfully compiled <%s>\n", dgs_argv[startArgC]);
      return 1;
    }
  }
  else
  {
    // check compilation
    if (bind_dascript::load_das_script(dgs_argv[startArgC]))
    {
      printf("Successfully compiled <%s>\n", dgs_argv[startArgC]);
      return 0;
    }
    else
    {
      printf("Error compiling <%s>\n", dgs_argv[startArgC]);
      return 1;
    }
  }

  if (df_get_real_name("scene.blk"))
    ecs::create_entities_blk(DataBlock("scene.blk"), NULL);
  printf("scene loaded entities\n");
  for (int i = 0; i < 100; ++i)
    g_entity_mgr->tick();
  g_entity_mgr->broadcastEventImmediate(EventStart());
  printf("EventStart sent\n");
  debug("EventStart");
  g_entity_mgr->broadcastEventImmediate(EventCPPTestEvent(2.f, 10, "test_string"));


  g_entity_mgr->tick();
  for (int i = 0; i < DEF_RUNS; ++i)
    g_entity_mgr->update(ecs::UpdateStageInfoAct(0.1, 0.1));

  printf("%d acts called\n", DEF_RUNS);
  for (int i = 0; i < 100; ++i)
    g_entity_mgr->tick();

  g_entity_mgr->broadcastEventImmediate(EventStart2());
  printf("EventStart2 sent\n");
  debug("EventStart2");

  printf("%d acts called\n", DEF_RUNS);
  for (int i = 0; i < 100; ++i)
    g_entity_mgr->tick();

  g_entity_mgr->broadcastEventImmediate(EventEnd());
  printf("EventEnd sent\n");

  G_ASSERT(get_test_value("EventStartTriggered") == 1);
  G_ASSERT(get_test_value("EventEndTriggered") == 1);
  int64_t reft = ref_time_ticks();
  g_entity_mgr->clear();
  debug("clear in %dus", get_time_usec(reft));
  printf("closed%s\n", had_errors ? " with unhandled errors (see debug file, run with -debug)" : "");
  return had_errors ? 1 : 0;
}

void require_project_specific_debugger_modules() {} // stub for dng debugger

int myMain(int startArgC)
{
  dgs_execute_quiet = true;
  g_entity_mgr.demandInit();
  // bind_dascript::set_das_root("../../../1stPartyLibs/daScript/"); // use current dir as root path
  bind_dascript::init_systems(NEED_DAS_AOT_COMPILE ? bind_dascript::AotMode::AOT : bind_dascript::AotMode::NO_AOT,
    bind_dascript::HotReload::ENABLED, bind_dascript::LoadDebugCode::YES,
    DAGOR_DBGLEVEL > 0 ? bind_dascript::LogAotErrors::YES : bind_dascript::LogAotErrors::NO);
  NEED_MODULE(DagorFiles)
  NEED_MODULE(DasEcsUnitTest)

  int ret = myMain2(startArgC);

  bind_dascript::shutdown_systems();
  return ret;
}

extern void default_crt_init_kernel_lib();
extern void default_crt_init_core_lib();

extern "C" int main(int argc, char **argv)
{
  G_STATIC_ASSERT(ecs::ecs_data_alignment(alignof(Point3)) == 4);
  G_STATIC_ASSERT(ecs::ecs_data_alignment(alignof(void *)) == sizeof(size_t));
  default_crt_init_kernel_lib();
  default_crt_init_core_lib();
  symhlp_init_default();
  ::dgs_argc = argc;
  ::dgs_argv = argv;
  dgs_report_fatal_error = my_fatal_handler;
  debug_set_log_callback(&log_callback);
  int startArgC = 1;
  bool debugFile = false;
  bool verbose = false;
  while (dgs_argc - startArgC > 1)
  {
    if (strcmp(dgs_argv[startArgC], "-debug") == 0)
    {
      debugFile = true;
      startArgC++;
    }
    else if (strcmp(dgs_argv[startArgC], "-verbose") == 0)
    {
      verbose = true;
      startArgC++;
    }
    else
    {
      startArgC = dgs_argc;
      break;
    }
  }
  if (dgs_argc - startArgC < 1)
  {
    printf("Usage: [-debug] [-verbose] file_name|dir_name; file_name.das or dir_name/*.das will be checked\n");
    return 1;
  }
#if _TARGET_PC_WIN
  if (verbose)
    set_debug_console_handle((intptr_t)::GetStdHandle(STD_OUTPUT_HANDLE));
#endif
  start_classic_debug_system(debugFile ? "debug" : nullptr, false);
  dd_get_fname(""); //== pull in directoryService.obj
  char buf[512];
  eastl::string loc;
  if (strstr(dgs_argv[0], "/") || strstr(dgs_argv[0], "\\"))
    loc = eastl::string(dd_get_fname_location(buf, dgs_argv[0]));
  else
    loc = "../../";
  dd_set_named_mount_path("daslibEcs", (loc + "../../prog/gameLibs/das/ecs").c_str());
  dd_set_named_mount_path("daslib", (loc + "../../prog/1stPartyLibs/daScript/daslib").c_str());
  bind_dascript::set_das_root((loc + "../../prog/1stPartyLibs/daScript").c_str()); // use exe dir as root path
  printf("daScript+daECS unit test\n");
  debug_flush(true);
  dd_add_base_path("");
  DagorHwException::setHandler("main");
  int retcode = 0;

  DAGOR_TRY { retcode = myMain(startArgC); }
  DAGOR_CATCH(DagorException e)
  {
#ifdef DAGOR_EXCEPTIONS_ENABLED
    DagorHwException::reportException(e, true);
#endif
    printf("exception");
    return 1;
  }

  DagorHwException::cleanup();

  TIME_PROFILER_SHUTDOWN();
  return retcode;
}

#include <startup/dag_leakDetector.inc.cpp>

// extern bool dgs_execute_quiet;
// #include <windows.h>
// #include <startup/dag_leakDetector.inc.cpp>
// #include <startup/dag_mainCon.inc.cpp>
