// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_startup.h>

#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>

#include <string.h>
#include <time.h>

#if _TARGET_PC_WIN
#include <direct.h>
#elif _TARGET_PC_LINUX
#include <unistd.h>
#endif

extern "C" const char *dagor_get_build_stamp_str(char *buf, size_t bufsz, const char *suffix);

static void dump_build_timestamp_and_start_time()
{
  char stamp_buf[256], start_time_buf[32] = {0};
  time_t start_at_time = time(nullptr);
  const char *asctimeResult = asctime(gmtime(&start_at_time));
  strcpy(start_time_buf, asctimeResult ? asctimeResult : "???");
  if (char *p = strchr(start_time_buf, '\n'))
    *p = '\0';
  debug("%s [started at %s UTC+0]\n", dagor_get_build_stamp_str(stamp_buf, sizeof(stamp_buf), ""), start_time_buf);
}

static void dump_command_line()
{
  String cmdLine;
  cmdLine.reserve(DAGOR_MAX_PATH);

#if _TARGET_PC_LINUX
  // dgs_argv[0] is relative on Linux if the application is started with a relative path.
  int rd = readlink("/proc/self/exe", cmdLine.data(), DAGOR_MAX_PATH - 1);
  if (rd > 0)
  {
    cmdLine.data()[rd] = '\0';
    char rpbuf[PATH_MAX];
    cmdLine = realpath(cmdLine.data(), rpbuf);
  }
  if (cmdLine.empty())
    cmdLine = dgs_argv[0];
#else
  cmdLine = dgs_argv[0];
#endif

  cmdLine += ' ';
  for (int i = 1; i < dgs_argc; ++i)
  {
    cmdLine += dgs_argv[i];
    cmdLine += i == (dgs_argc - 1) ? "" : " ";
  }
  debug("cmd: %s", cmdLine.str());
}

static void dump_current_working_directory()
{
  char cwdBuf[DAGOR_MAX_PATH] = {0};
  debug("cwd: %s", getcwd(cwdBuf, sizeof(cwdBuf)));
}

void ec_log_startup_info()
{
  dump_build_timestamp_and_start_time();
  dump_command_line();
  dump_current_working_directory();
}
