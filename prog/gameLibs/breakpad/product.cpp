// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <breakpad/binder.h>
#include <osApiWrappers/dag_unicode.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_globDef.h>
#include <stdio.h>
#include <string.h>
#if _TARGET_PC_WIN
#include <windows.h>
#elif _TARGET_PC_LINUX
#include <unistd.h>
#endif

extern "C" const char *dagor_exe_build_date;
extern "C" const char *dagor_exe_build_time;

namespace breakpad
{

Product::Product(bool with_defaults)
{
  if (!with_defaults)
    return;

  version = "0.0.0.0";
  name = "Unknown";

  const int n = 6;
  char signParseBuf[n][n] = {};
  sscanf(dagor_exe_build_date, "%s %s %s", signParseBuf[1], signParseBuf[2], signParseBuf[0]);

  sscanf(dagor_exe_build_time, "%2s:%2s:%2s", signParseBuf[3], signParseBuf[4], signParseBuf[5]);

  for (int i = 0; i < n; i++)
    build.append(signParseBuf[i]);

  started.printf(32, "%llu", time(NULL));

#if (DAGOR_DBGLEVEL == 1)
  channel = "dev";
#elif (DAGOR_DBGLEVEL == 0)
  channel = "rel";
#elif (DAGOR_DBGLEVEL == -1)
  channel = "irel";
#elif (DAGOR_DBGLEVEL == 2)
  channel = "dbg";
#else
  channel = "Unknown";
#endif

  if (::dgs_argv && *::dgs_argv) // Otherwise we have no defaults to use
  {
    String &exe = commandLine.push_back();
    exe = ::dgs_argv[0];
#if _TARGET_PC_WIN
    char utf8buf[MAX_PATH * 3] = {'\0'};
    wchar_t exePathW[MAX_PATH] = {L'\0'};
    if (::GetModuleFileNameW(NULL, exePathW, countof(exePathW)))
      exe = wcs_to_utf8(exePathW, utf8buf, countof(utf8buf));
#elif _TARGET_PC_LINUX
    char buf[MAX_PATH] = {'\0'};
    int sz = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (sz > 0 && sz < sizeof(buf))
      exe = buf;
#endif

    for (int i = 1; i < ::dgs_argc; ++i)
    {
      String &arg = commandLine.push_back();
      char *space = strchr(::dgs_argv[i], ' ');
      if (space)
        arg += "\\\"";
#if _TARGET_PC_WIN
      Tab<char> buf;
      arg += acp_to_utf8(::dgs_argv[i], buf);
#else
      arg += ::dgs_argv[i];
#endif
      if (space)
        arg += "\\\"";
    }
  }
}

} // namespace breakpad
