// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include "game/gameScripts.h"
#include "ui/userUi.h"
#include <ui/overlay.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_string.h>
#include <sqrat.h>
#include <squirrel/memtrace.h>
#include <util/dag_delayedAction.h>
#include <debug/dag_logSys.h>
#include <memory/dag_framemem.h>
#include <daECS/core/entityManager.h>
#include <ecs/scripts/dascripts.h>


namespace gamescripts
{
uint32_t initialize_deserializer(const char *);
}

namespace bind_dascript
{
extern bool serializationReading;
extern bool enableSerialization;
} // namespace bind_dascript

enum SelectedVM
{
  VM_UNKNOWN,
  VM_OVERLAY_UI,
  VM_USER_UI,
  VM_GAME
};

static SelectedVM current_vm = VM_UNKNOWN;

static SelectedVM parse_vm(const char *vm)
{
  if (dd_stricmp(vm, "overlay_ui") == 0)
    return VM_OVERLAY_UI;
  if (dd_stricmp(vm, "user_ui") == 0)
    return VM_USER_UI;
  if (dd_stricmp(vm, "game") == 0)
    return VM_GAME;
  return VM_UNKNOWN;
}

static const char *get_vm_name(SelectedVM vm)
{
  if (vm == VM_OVERLAY_UI)
    return "overlay_ui";
  if (vm == VM_USER_UI)
    return "user_ui";
  if (vm == VM_GAME)
    return "game";
  return "unknown";
}


static bool sq_console_handler(const char *argv[], int argc) // move it somewhere else
{
  if (argc < 1)
    return false;

  if (argv[0][0] == '$')
  {
    HSQUIRRELVM vm = nullptr;
    const char *exec = "";
    if (current_vm == VM_UNKNOWN)
    {
      console::print("Run sq.set_vm first");
      return true;
    }
    else if (current_vm == VM_OVERLAY_UI)
    {
      exec = "overlay_ui.exec";
      vm = overlay_ui::get_vm();
    }
    else if (current_vm == VM_USER_UI)
    {
      exec = "user_ui.exec";
      vm = user_ui::get_vm();
    }
    else if (current_vm == VM_GAME) //-V547
    {
      exec = "sqgame.exec";
      vm = gamescripts::get_vm();
    }
    else
    {
      G_ASSERTF(0, "Unhandled current_vm");
      return true;
    }

    String code(framemem_ptr());
    for (int i = 0; i < argc; i++)
    {
      if (i > 0)
        code += " ";
      int offset = (i == 0 ? 1 : 0); // skip '$'
      code += (argv[i] + offset);
    }

    if (code.empty())
      return true;

    String wrappedCode(framemem_ptr());
    wrappedCode.printf(0, "%s %s", exec, code.str());
    debug("executing sq: %s", wrappedCode.str());
    console::command(wrappedCode);

    return true;
  }

  int found = 0;
  CONSOLE_CHECK_NAME("sq", "set_vm", 1, 2)
  {
    if (argc > 1)
      current_vm = parse_vm(argv[1]);
    console::print("Selected <%s> VM. Options are: 'overlay_ui', 'user_ui', 'game'.", get_vm_name(current_vm));
  }
  CONSOLE_CHECK_NAME("sq", "reload_script", 1, 1) { gamescripts::reload_sq_modules(); }
  CONSOLE_CHECK_NAME("sq", "reload_ui_scripts", 1, 1)
  {
    overlay_ui::reload_scripts(false);
    user_ui::reload_user_ui_script(false);
  }
  CONSOLE_CHECK_NAME("sq_memtrace", "reset_all", 1, 1) { sqmemtrace::reset_all(); }
  CONSOLE_CHECK_NAME("sq_memtrace", "dump_all", 1, 2)
  {
    sqmemtrace::dump_all(argc > 1 ? console::to_int(argv[1]) : -1);
    flush_debug_file();
  }

  return found;
}

REGISTER_CONSOLE_HANDLER(sq_console_handler);

namespace bind_dascript
{
extern size_t dump_memory();
}

static bool das_console_handler(const char *argv[], int argc) // move it somewhere else
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("das", "dump_mem", 1, 1) { console::print_d("total memory used: <%d>", bind_dascript::dump_memory()); } //-V547
  CONSOLE_CHECK_NAME("das", "reload_scripts", 1, 2)
  {
    bool announce = (argc > 1) ? console::to_bool(argv[1]) : true;
    delayed_call([announce]() {
      gamescripts::unload_all_das_scripts_without_debug_agents();
      gamescripts::reload_das_init();
      gamescripts::reload_das_modules();
      gamescripts::main_thread_post_load();
      if (announce)
        g_entity_mgr->broadcastEvent(EventDaScriptReloaded());
    });
  }
  CONSOLE_CHECK_NAME("das", "reload_scripts_serialized_from", 2, 3)
  {
    const char *filename = argv[1];
    const bool announce = (argc > 2) ? console::to_bool(argv[2]) : true;
    delayed_call([filename, announce]() {
      const uint32_t version = gamescripts::initialize_deserializer(filename);
      debug("Loading scripts version %u from %s", version, filename);
      if (version != -1 && version != 0)
        bind_dascript::enableSerialization = bind_dascript::serializationReading = true;
      gamescripts::unload_all_das_scripts_without_debug_agents();
      gamescripts::reload_das_init();
      gamescripts::reload_das_modules();
      gamescripts::main_thread_post_load();
      if (announce)
        g_entity_mgr->broadcastEvent(EventDaScriptReloaded());
    });
  }
  CONSOLE_CHECK_NAME("das", "aot", 1, 2)
  {
    if (argc > 1)
      gamescripts::set_das_aot(console::to_bool(argv[1]));
    console::print_d("das %s aot mode", gamescripts::get_das_aot() ? "in" : "without");
  }
  CONSOLE_CHECK_NAME("das", "log_aot_errors", 1, 2)
  {
    if (argc > 1)
      gamescripts::set_das_log_aot_errors(console::to_bool(argv[1]));
    console::print_d("das will%s log aot errors", gamescripts::get_das_log_aot_errors() ? "" : " not");
  }
  CONSOLE_CHECK_NAME("das", "loading_threads", 1, 2)
  {
    if (argc > 1)
      gamescripts::set_load_threads_num(console::to_int(argv[1]));
    console::print_d("das loads in %@ threads", gamescripts::get_load_threads_num());
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(das_console_handler);
