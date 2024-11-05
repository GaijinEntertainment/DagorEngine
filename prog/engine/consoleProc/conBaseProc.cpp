// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <util/dag_string.h>
#include <math/dag_math3d.h>
#include <memory/dag_memStat.h>
#include <generic/dag_sort.h>
#include <generic/dag_initOnDemand.h>
#include <generic/dag_tabWithLock.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_debug.h>
#include "consolePrivate.h"
#include <osApiWrappers/dag_threads.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <util/dag_delayedAction.h>
using namespace console_private;

//************************************************************************
//* common console commands
//************************************************************************
static volatile float test_zero = 0.f; // for debug console command "divzero"
struct TestErrorThread : public DaThread
{
  bool crash; // crash or NaN
  TestErrorThread(bool c) : DaThread(), crash(c) {}
  virtual void execute()
  {
    if (crash)
      *(volatile int *)0 = 0;
    else
      test_zero /= test_zero;
  }
};

eastl::vector<eastl::pair<int, eastl::string>> delayedCommands;

static void delayedCommandsUpdate(void *)
{
  const int time = get_time_msec();
  delayedCommands.erase(eastl::remove_if(delayedCommands.begin(), delayedCommands.end(),
                          [=](const auto &cmd) {
                            if (cmd.first - time <= 0)
                            {
                              console::command(cmd.second.c_str());
                              return true;
                            }
                            return false;
                          }),
    delayedCommands.end());
}

class ConsoleConProc : public console::ICommandProcessor
{
public:
  ConsoleConProc() : console::ICommandProcessor(1000) {}
  virtual void destroy() { unregister_regular_action_to_idle_cycle(delayedCommandsUpdate, nullptr); }

  static bool tobool(const char *s)
  {
    if (!s || !*s)
      return false;
    else if (!stricmp(s, "true") || !stricmp(s, "on"))
      return true;
    else if (!stricmp(s, "false") || !stricmp(s, "off"))
      return false;
    else
      return console::ICommandProcessor::to_int(s);
  }


  static int stringCompare(const console::CommandStruct *s1, const console::CommandStruct *s2) { return strcmp(s1->str(), s2->str()); }

  static int handleConVars(const char **argv, int argc)
  {
    char buf[512];
    ConVarIterator cit;
    while (ConVarBase *cv = cit.nextVar())
    {
      if (!ICommandProcessor::cmdCollector || ICommandProcessor::cmdSearchMode)
      {
        if (stricmp(argv[0], cv->getName()) == 0)
        {
          if (argc < 1 || argc > 2) // wrong number of arguments
            return -1;

          if (ICommandProcessor::cmdCollector)
          {
            cv->describeValue(buf, sizeof(buf));
            char res[513];
            const char *help = cv->getHelpTip();
            snprintf(res, sizeof(res), "%s %s", buf, help ? help : "");
            console::CommandStruct command(cv->getName(), 1, 2, res);
            ICommandProcessor::cmdCollector->push_back(command);
            return -1;
          }
          else
          {
            cv->parseString(argc > 1 ? argv[1] : NULL);
            cv->describeValue(buf, sizeof(buf));
            console::print_d(buf);
          }
          return 1;
        }
      }
      else
      {
        console::CommandStruct command;
        command.command = cv->getName();
        command.minArgs = 1;
        command.maxArgs = 2;
        command.description = cv->getHelpTip();
        cv->describeValue(buf, sizeof(buf));
        const char *p = strstr(buf, "= ");
        command.varValue = p ? p + 2 : buf;
        ICommandProcessor::cmdCollector->push_back(command);
      }
    }
    return 0;
  }

  // process command
  virtual bool processCommand(const char *argv[], int argc)
  {
    // debug("processCommand: '%s'", argv ? argv[0] : NULL);

    if (argc <= 0)
      return 0;

    int found = handleConVars(argv, argc);
    if (found != 0)
      return found == -1 ? false : true;

    CONSOLE_CHECK_NAME("console", "progress_indicator", 2, 3)
    {
      // show progress indicator in console:
      //   console.progress_indicator <id> "<title>"

      // hide progress indicator:
      //   console.progress_indicator <id>

      if (console::get_visual_driver())
        console::get_visual_driver()->set_progress_indicator(argv[1], argc == 2 ? nullptr : argv[2]);
    }
    CONSOLE_CHECK_NAME("console", "mode", 2, 2)
    {
      if (argc >= 2)
      {
        real h = 0;
        if (dd_stricmp(argv[1], "full") == 0 || dd_stricmp(argv[1], "f") == 0)
          h = 1.0f;
        else if (dd_stricmp(argv[1], "half") == 0 || dd_stricmp(argv[1], "h") == 0)
          h = 0.5f;
        else if (dd_stricmp(argv[1], "min") == 0 || dd_stricmp(argv[1], "m") == 0)
          h = 0.1f;
        else
        {
          h = to_real(argv[1]) / 100.0f;
          if (h < 0.05)
            h = 0.05;
          else if (h > 1.0f)
            h = 1.0f;
        }
        if (float_nonzero(h))
        {
          conHeight = h;
          if (console::get_visual_driver())
            console::get_visual_driver()->set_cur_height(h);
        }
      }
    }

    CONSOLE_CHECK_NAME("con", "save", 1, 2)
    {
      if (argc >= 2)
        console::get_visual_driver()->save_text(argv[1]);
      else
        console::print_d("no filename specified");
    }

    CONSOLE_CHECK_NAME("con", "main_background_color", 2, 2) { console::main_background_color = console::to_int_from_hex(argv[1]); }
    CONSOLE_CHECK_NAME("con", "tips_background_color", 2, 2) { console::tips_background_color = console::to_int_from_hex(argv[1]); }
    CONSOLE_CHECK_NAME("con", "max_tips", 2, 2) { console::max_tips = console::to_int(argv[1]); }
    CONSOLE_CHECK_NAME("con", "print_logerr_to_console", 2, 2) { console::print_logerr_to_console = console::to_bool(argv[1]); }

    CONSOLE_CHECK_NAME("con", "speed", 1, 2)
    {
      if (argc >= 2)
      {
        consoleSpeed = to_real(argv[1]);
      }
      else
      {
        console::print_d("conspeed = %.4f pixels/sec", consoleSpeed);
      }
    }
    CONSOLE_CHECK_NAME("app", "autoexec", 1, 2)
    {
      extern bool run_con_batch(const char *pathname);
      if (argc > 1)
        run_con_batch(argv[1]);
      else
        console::print("usage: autoexec console_commands_file");
    }
    CONSOLE_CHECK_NAME("app", "flush", 1, 2) { debug_flush(argc > 1 ? to_bool(argv[1]) : false); }
    CONSOLE_CHECK_NAME("app", "memleak", 1, 1) { memalloc(1, tmpmem); }
    CONSOLE_CHECK_NAME("app", "heapoverrun", 1, 1)
    {
      char *a = (char *)memalloc(1, tmpmem);
      for (int i = 0; i < 256; ++i)
        a[i] = '\0';
      memfree(a, tmpmem);
    }
    CONSOLE_CHECK_NAME("app", "freeze", 1, 1)
    {
      while (1)
        sleep_msec(0);
    }
    CONSOLE_CHECK_NAME("app", "divzero", 1, 1) { debug("%f", 1.0f / test_zero); }
    CONSOLE_CHECK_NAME("app", "crash", 1, 1) { *(volatile int *)0 = 0; }
#if _TARGET_PC_WIN
    CONSOLE_CHECK_NAME("app", "corruptstack", 1, 1) { memset((char *)_AddressOfReturnAddress() - 0x148, 0xEE, 0x200); }
    CONSOLE_CHECK_NAME("app", "corruptstack2", 1, 1) { *(uintptr_t *)_AddressOfReturnAddress() = 0xAABBCCDD; }
#endif
    CONSOLE_CHECK_NAME("app", "fatal", 1, 1) { DAG_FATAL("manual fatal"); }
    CONSOLE_CHECK_NAME("app", "threadcrash", 1, 1)
    {
      TestErrorThread thr(true);
      thr.start();
      thr.terminate(true);
    }
    CONSOLE_CHECK_NAME("app", "threadnan", 1, 1)
    {
      TestErrorThread thr(false);
      thr.start();
      thr.terminate(true);
    }
    CONSOLE_CHECK_NAME("app", "terminate", 1, 2) { terminate_process((argc >= 2) ? atoi(argv[1]) : 0); }
    CONSOLE_CHECK_NAME("app", "memory_chunks", 1, 2)
    {
      console::print_d("memory_chunks = %zu", dagor_memory_stat::get_memchunk_count(argc < 2 ? false : tobool(argv[1])));
    }
    CONSOLE_CHECK_NAME("app", "memory_allocated", 1, 2)
    {
      console::print_d("memory_allocated = %zu", dagor_memory_stat::get_memory_allocated(argc < 2 ? false : tobool(argv[1])));
    }
    CONSOLE_CHECK_NAME("con", "cmdlist", 1, 2)
    {
      // show list of commands
      console::print("cmdlist:");
      console::CommandList commandList(tmpmem_ptr());

      console::get_command_list(commandList);

      sort(commandList, &stringCompare);

      String cmdPart(argc > 1 ? argv[1] : "");

      int count = 0;
      if (cmdPart.length())
      {
        // list part of the all commands
        for (const console::CommandStruct &info : commandList)
        {
          if (dd_strnicmp(cmdPart, info.str(), cmdPart.length()) == 0)
          {
            console::print(info.str());
            if (!info.description.empty())
              console::print("  %s", info.description);
            count++;
          }
        }
      }
      else
      {
        // list all commands
        for (const console::CommandStruct &info : commandList)
        {
          console::print(info.str());
          if (!info.description.empty())
            console::print("  %s", info.description);
          count++;
        }
      }

      console::print("\ntotal %d command(s)", count);
    }
    CONSOLE_CHECK_NAME_EX("con", "help", 1, 2, "prints information about the command", "<command>")
    {
      if (argc > 1)
      {
        console::CommandStruct info;
        if (console::find_console_command(argv[1], info))
        {
          console::print("command \"%s\" minArgs = %d, maxArgs = %d", info.command, info.minArgs, info.maxArgs);
          console::print(info.description.length() ? info.description.str() : "no description found");
        }
        else
        {
          console::print_d("command \"%s\" not found", argv[1]);
        }
      }
      else
        console::print("usage: con.help [command_name]");
    }
    CONSOLE_CHECK_NAME_EX("con", "sleep", 3, 100, "delay command execution for the specified seconds", "<seconds> <command>...")
    {
      int at = get_time_msec() + 1000 * atof(argv[1]);
      eastl::string cmd;
      for (int argNo = 2; argNo < argc; argNo++)
      {
        if (argNo != 2)
          cmd += " ";
        cmd += argv[argNo];
      }
      delayedCommands.emplace_back(at, cmd);
      register_regular_action_to_idle_cycle(delayedCommandsUpdate, nullptr);
    }
    return found;
  }
};

static InitOnDemand<ConsoleConProc> con_conproc;


void console_private::registerBaseConProc()
{
  con_conproc.demandInit();
  add_con_proc(con_conproc);
}
void console_private::unregisterBaseConProc() { remove_con_proc(con_conproc); }
