// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define __UNLIMITED_BASE_PATH 1
#define __DEBUG_FILEPATH      NULL
#include <startup/dag_mainCon.inc.cpp>
#include <dasModules/dasFsFileAccess.h>
#include <ecs/scripts/dasEs.h>
#include <debug/dag_logSys.h>
#include <osApiWrappers/dag_dbgStr.h> //set_debug_console_handle
#if _TARGET_PC_WIN
#include <windows.h> //set_debug_console_handle
#endif

String modulePath;

das::FileAccessPtr get_file_access(char *pak)
{
  if (pak)
    return das::make_smart<bind_dascript::DagFileAccess>(pak);
  if (!modulePath.empty())
    return das::make_smart<bind_dascript::DagFileAccess>(modulePath.c_str());
  return das::make_smart<bind_dascript::DagFileAccess>(bind_dascript::HotReload::DISABLED);
}

#define MAIN_FUNC_NAME aot_main
#include "../../1stPartyLibs/daScript/utils/daScript/main.cpp"

static void stderr_report_fatal_error(const char *error, const char *msg, const char *stack)
{
  printf("%s: %s\n%s\n", error, msg, stack);
  quit_game(1, false);
}

void os_debug_break()
{
  printf("unhandled exception?\n");
#if _TARGET_PC_WIN
  _exit(1);
#else
  exit(1);
#endif
}

static int log_callback(int lev_tag, const char * /*fmt*/, const void * /*arg*/, int /*anum*/, const char * /*ctx_file*/,
  int /*ctx_line*/)
{
  if (lev_tag == LOGLEVEL_ERR || lev_tag == LOGLEVEL_FATAL || lev_tag == LOGLEVEL_WARN)
    return 1;
  return 0;
}

int DagorWinMain(bool debugmode)
{
  debug_allow_level_files(false);
  setvbuf(stdout, NULL, _IOFBF, 8192);
#if _TARGET_PC_WIN
  set_debug_console_handle((intptr_t)::GetStdHandle(STD_OUTPUT_HANDLE));
#endif
  dgs_report_fatal_error = stderr_report_fatal_error;
  if (dgs_argc < 3)
  {
    ::debug("dasAot <in_script.das> <out_script.das.cpp> [-q] [-j] -- [--config aot_config.blk]\n");
    return -1;
  }
  debug_set_log_callback(&log_callback);

  char exe_dir[DAGOR_MAX_PATH] = {0};
  dd_get_fname_location(exe_dir, dgs_argv[0]);
  dd_simplify_fname_c(exe_dir);
  String cwd = String(exe_dir);

  // dng default
  version2syntax = false;
  gen2MakeSyntax = true;

  DataBlock config;
  for (int ai = 0; ai != dgs_argc; ++ai)
  {
    if (strcmp(dgs_argv[ai], "--config") == 0)
    {
      if (ai + 1 < dgs_argc)
      {
        String configPath = cwd + dgs_argv[ai + 1];
        dd_simplify_fname_c(configPath);
        bool loaded = config.load(configPath);
        if (loaded)
        {
          for (int i = 0; i < config.paramCount(); ++i)
          {
            const char *name = config.getParamName(i);
            if (strcmp(name, "base_path") == 0)
            {
              String basePath = cwd + config.getStr(i);
              dd_simplify_fname_c(basePath);
              dd_add_base_path(basePath);
            }
            else if (strcmp(name, "project") == 0)
            {
              modulePath = String(config.getStr(i));
            }
            else if (strcmp(name, "version2syntax") == 0)
            {
              version2syntax = config.getBool(i);
            }
            else if (strcmp(name, "gen2MakeSyntax") == 0)
            {
              gen2MakeSyntax = config.getBool(i);
            }
          }
        }
      }
      else
      {
        DAG_FATAL("config path expected\n");
        return -1;
      }
    }
  }

  String path2 = String(exe_dir) + "../../prog/";
  dd_simplify_fname_c(path2);
  dd_add_base_path(path2.c_str());
  dd_set_named_mount_path("daslibEcs", "gameLibs/das/ecs/");
  dd_set_named_mount_path("daslib", "1stPartyLibs/daScript/daslib/");
  bind_dascript::set_das_root("1stPartyLibs/daScript/"); // use exe dir as root path
  auto syntax = version2syntax   ? bind_dascript::DasSyntax::V2_0
                : gen2MakeSyntax ? bind_dascript::DasSyntax::V1_5
                                 : bind_dascript::DasSyntax::V1_0;
  bind_dascript::init_systems(bind_dascript::AotMode::NO_AOT, bind_dascript::HotReload::DISABLED, bind_dascript::LoadDebugCode::YES,
    bind_dascript::LogAotErrors::NO, syntax, modulePath.empty() ? nullptr : modulePath.c_str());

  return aot_main(dgs_argc, (char **)dgs_argv);
}
#if _TARGET_PC_LINUX
int os_message_box(const char *, const char *, int) { return 0; }
#endif
