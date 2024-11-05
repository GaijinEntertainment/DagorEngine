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

bool verboseLogs = false;
bool strict = false;
int strictExit = 0;
String modulePath;

das::FileAccessPtr get_file_access(char *pak)
{
  if (pak)
    return das::make_smart<bind_dascript::DagFileAccess>(pak, bind_dascript::HotReload::DISABLED);
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

static int log_callback(
  int lev_tag, const char * /*fmt*/, const void * /*arg*/, int /*anum*/, const char * /*ctx_file*/, int /*ctx_line*/)
{
  if (lev_tag == LOGLEVEL_ERR || lev_tag == LOGLEVEL_FATAL)
    strictExit = 1;
  if (lev_tag == LOGLEVEL_ERR || lev_tag == LOGLEVEL_FATAL || lev_tag == LOGLEVEL_WARN)
    return 1;
  return verboseLogs ? 1 : 0;
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
    ::debug("dasAot <in_script.das> <out_script.das.cpp> [-q] [-j] "
            "-- [--config aot_config.blk] [--das-root=das/root] [--strict] [--verbose]\n");
    return -1;
  }
  debug_set_log_callback(&log_callback);

  String dasRoot;

  DataBlock config;
  for (int ai = 0; ai != dgs_argc; ++ai)
  {
    if (strcmp(dgs_argv[ai], "--config") == 0)
    {
      if (ai + 1 < dgs_argc)
      {
        String configPath(dgs_argv[ai + 1]);
        char config_dir[DAGOR_MAX_PATH] = {0};
        dd_get_fname_location(config_dir, configPath);
        String configDir(config_dir);
        bool loaded = config.load(configPath);
        if (!loaded)
        {
          logerr("Unable to load config file '%s'", configPath.c_str());
        }
        else
        {
          const int basePathId = config.getNameId("base_path");
          const int projectId = config.getNameId("project");
          const int dasRootId = config.getNameId("dasRoot");
          for (int i = 0; i < config.paramCount(); ++i)
          {
            const int paramNameId = config.getParamNameId(i);
            if (paramNameId == basePathId)
            {
              String basePath = configDir + config.getStr(i);
              dd_simplify_fname_c(basePath);
              dd_add_base_path(basePath);
            }
            else if (paramNameId == projectId)
              modulePath = String(config.getStr(i));
            else if (paramNameId == dasRootId)
              dasRoot = String(config.getStr(i));
          }
          const int mountPointsId = config.getNameId("mountPoints");
          for (int i = 0; i < config.blockCount(); ++i)
          {
            const DataBlock *childBlock = config.getBlock(i);
            if (childBlock->getBlockNameId() == mountPointsId)
            {
              for (int j = 0; j < childBlock->paramCount(); ++j)
              {
                String mountPath = configDir + childBlock->getStr(j);
                dd_simplify_fname_c(mountPath);
                dd_set_named_mount_path(childBlock->getParamName(j), mountPath);
              }
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
    else if (strcmp(dgs_argv[ai], "--das-root") == 0)
    {
      dasRoot = String(dgs_argv[ai + 1 < dgs_argc ? ai + 1 : ai]);
    }
    else if (strcmp(dgs_argv[ai], "--base-path") == 0)
    {
      dd_add_base_path(dgs_argv[ai + 1 < dgs_argc ? ai + 1 : ai]);
    }
    else if (strcmp(dgs_argv[ai], "--project") == 0)
    {
      modulePath = String(dgs_argv[ai + 1 < dgs_argc ? ai + 1 : ai]);
    }
    else if (strcmp(dgs_argv[ai], "--strict") == 0)
    {
      strict = true;
    }
    else if (strcmp(dgs_argv[ai], "--verbose") == 0)
    {
      verboseLogs = true;
    }
  }

  if (!dasRoot.empty())
    bind_dascript::set_das_root(dasRoot.c_str());
  bind_dascript::init_systems(bind_dascript::AotMode::NO_AOT, bind_dascript::HotReload::DISABLED, bind_dascript::LoadDebugCode::YES,
    bind_dascript::LogAotErrors::NO, !modulePath.empty() ? modulePath.c_str() : nullptr);

  const int res = aot_main(dgs_argc, (char **)dgs_argv);
  return res != 0 || !strict ? res : strictExit;
}
#if _TARGET_PC_LINUX
int os_message_box(const char *, const char *, int) { return 0; }
#endif
