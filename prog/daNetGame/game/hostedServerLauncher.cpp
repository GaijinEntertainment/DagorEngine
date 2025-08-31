// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include <util/dag_console.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/event.h>
#include <sqrat.h>
#include <sqstdblob.h>
#include <crypto/base64.h>
#include <osApiWrappers/dag_dynLib.h>
#include <osApiWrappers/dag_threads.h>
#include <debug/dag_hwExcept.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_symHlp.h>
#include <util/dag_delayedAction.h>
#include "hostedServerLauncher.h"
#include <util/dag_console.h>


ECS_REGISTER_EVENT(EventHostedInternalServerDidStart);
ECS_REGISTER_EVENT(EventHostedInternalServerDidStop);
ECS_REGISTER_EVENT(EventHostedInternalServerToStart);
ECS_REGISTER_EVENT(EventHostedInternalServerToStop);

void *internal_dedicated_server_dll_handle;
void(__cdecl *exit_internal_dedicated_server)(const char *exit_reason);

extern "C" const char *dedicated_server_dll_fn;

static void unload_dedicated_dll()
{
  ::symhlp_unload(dedicated_server_dll_fn);
  bool result = os_dll_close(internal_dedicated_server_dll_handle);
  if (result)
    debug("unloaded dedic dll");
  else
    debug("failed to unload dedic dll");
  internal_dedicated_server_dll_handle = NULL;
}

struct InternalDedicatedServerMainThread;

static eastl::unique_ptr<InternalDedicatedServerMainThread> current_running_internal_server;
static bool is_externally_shutting_down = false;

static void terminate_and_unload_dlls();
static void kill_current_internal_server_and_wait();

void kill_internal_dedicated_server()
{
  if (exit_internal_dedicated_server)
  {
    exit_internal_dedicated_server("kill_internal_dedicated_server");
    exit_internal_dedicated_server = NULL;
  }
}


static void hosted_server_did_start()
{
  debug("hosted_server_did_start");
  execute_delayed_action_on_main_thread(
    make_delayed_action([&] { g_entity_mgr->broadcastEvent(EventHostedInternalServerDidStart()); }));
}

struct InternalDedicatedServerMainThread final : public DaThread
{

  int _argc;
  char **_argv;
  InternalDedicatedServerMainThread(int argc, char **argv) : DaThread("InternalDedicatedServerMainThread ", 4 << 20)
  {
    _argc = argc;
    _argv = argv;
  }

  ~InternalDedicatedServerMainThread()
  {
    for (int i = 0; i < _argc; i++)
      memfree(_argv[i], strmem);
    delete[] _argv;
  }

  void execute() override
  {
    DagorHwException::setHandler("main_internal_server");
    if (internal_dedicated_server_dll_handle)
    {
      bool(__cdecl * start_dedicated_server_with_cmd_args)() =
        (bool(__cdecl *)())os_dll_get_symbol(internal_dedicated_server_dll_handle, "start_dedicated_server");

      void(__cdecl * dgs_init_argv)(int argc, char **argv) =
        (void(__cdecl *)(int argc, char **argv))os_dll_get_symbol(internal_dedicated_server_dll_handle, "dgs_init_argv_exported");

      void(__cdecl * hosted_server_on_loaded)(void *) =
        (void(__cdecl *)(void *))os_dll_get_symbol(internal_dedicated_server_dll_handle, "hosted_server_on_loaded");

      exit_internal_dedicated_server =
        (void(__cdecl *)(const char *exit_reason))os_dll_get_symbol(internal_dedicated_server_dll_handle, "exit_game_exported");

      if (start_dedicated_server_with_cmd_args && dgs_init_argv)
      {
        ::symhlp_load(dedicated_server_dll_fn);
        hosted_server_on_loaded((void *)(&hosted_server_did_start));
        dgs_init_argv(_argc, _argv);
        start_dedicated_server_with_cmd_args();
      }
      if (!is_externally_shutting_down)
      {
        run_action_on_main_thread([] {
          terminate_and_unload_dlls();
          g_entity_mgr->broadcastEvent(EventHostedInternalServerDidStop());
        });
      }
      else
        run_action_on_main_thread([] { g_entity_mgr->broadcastEvent(EventHostedInternalServerDidStop()); });
    }
    else
      run_action_on_main_thread([] { g_entity_mgr->broadcastEvent(EventHostedInternalServerDidStop()); });
  }
};


static void terminate_and_unload_dlls()
{
  current_running_internal_server->terminate(true, -1);
  // thread stopped, we can unload the dll, but before ~InternalDedicatedServerMainThread(), because it will destroy args strings
  unload_dedicated_dll();
  current_running_internal_server.reset();
}

static void kill_current_internal_server_and_wait()
{
  if (!current_running_internal_server)
    return;
  is_externally_shutting_down = true;
  kill_internal_dedicated_server();
  terminate_and_unload_dlls();
  is_externally_shutting_down = false;
}


void launch_internal_dedicated_server_with_args(int external_argc, char **external_argv)
{
  if (current_running_internal_server)
  {
    debug("can't launch internal dedicated server while existing is still running, waiting now for its termination");
    kill_current_internal_server_and_wait();
    return;
  }
  char **argv = new char *[external_argc + 1];

  argv[0] = str_dup((char *)(dedicated_server_dll_fn), strmem);
  for (int i = 0; i < external_argc; i++)
  {
    argv[i + 1] = str_dup(external_argv[i], strmem);
    debug(external_argv[i]);
  }

  internal_dedicated_server_dll_handle = os_dll_load_deep_bind(dedicated_server_dll_fn);

  current_running_internal_server.reset(new InternalDedicatedServerMainThread(external_argc + 1, argv));
  current_running_internal_server->start();
}

static SQRESULT launch_internal_dedicated_server(HSQUIRRELVM vm)
{
  Sqrat::Var<Sqrat::Table> params_var(vm, 2);
  const Sqrat::Table &params = params_var.value;

  Sqrat::Array cmd_args = params["cmd_args"];

  int cmd_args_count = (cmd_args.GetType() == OT_ARRAY) ? cmd_args.Length() : 0;
  int argc = cmd_args_count;
  char **argv = new char *[argc];

  for (int i = 0; i < cmd_args_count; i++)
  {
    char *cmd_str = (char *)cmd_args[SQInteger(i)].GetVar<const char *>().value;
    argv[i] = str_dup(cmd_str, strmem);
  }
  launch_internal_dedicated_server_with_args(argc, argv);
  for (int i = 0; i < cmd_args_count; i++)
  {
    delete argv[i];
  }
  delete[] argv;
  return 0;
}

static SQRESULT kill_internal_dedicated_server_from_script(HSQUIRRELVM)
{
  kill_internal_dedicated_server();
  return 0;
}

void bind_hosting_internal_dedicated_server(Sqrat::Table &ns)
{
  ns.SquirrelFunc("launch_internal_dedicated_server", launch_internal_dedicated_server, 2, ".t");
  ns.SquirrelFunc("kill_internal_dedicated_server", kill_internal_dedicated_server_from_script, 1, NULL);
}


void shutdown_internal_server_on_host_exit() { kill_current_internal_server_and_wait(); }


static bool debug_host_mode_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("host_mode", "launch_internal_dedicated_server", 0, 40)
  {
    for (int i = 0; i < argc; i++)
      debug(argv[i]);
    launch_internal_dedicated_server_with_args(argc - 1, (char **)(&argv[1]));
  }

  CONSOLE_CHECK_NAME("host_mode", "kill_internal_dedicated_server", 0, 0) { kill_internal_dedicated_server(); }
  return found;
}

REGISTER_CONSOLE_HANDLER(debug_host_mode_console_handler);
