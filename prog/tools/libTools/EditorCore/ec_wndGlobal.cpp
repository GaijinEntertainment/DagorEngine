// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_simpleString.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_unicode.h>
#include <libTools/util/strUtil.h>
#include <debug/dag_log.h>
#include <startup/dag_globalSettings.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#endif

namespace sgg
{
SimpleString mExeDir;
SimpleString mFullExeDir;
SimpleString mFullStartDir;
SimpleString mCommonDataDir;


void set_exe_path(const char path[])
{
  mExeDir = path;

  char start_path[512];
  start_path[0] = 0;
  G_VERIFY(::getcwd(start_path, 512));
  ::make_slashes(start_path);
  mFullStartDir = start_path;
}


const char *get_exe_path() { return mExeDir.str(); }

const char *get_exe_path_full()
{
  if (mFullExeDir == "")
  {
#if _TARGET_PC_WIN
    wchar_t wpath[512];
    String _path;
    GetModuleFileNameW(GetModuleHandle(NULL), wpath, 512);
    convert_to_utf8(_path, wpath);
    ::dd_get_fname_location(_path, _path);

    mFullExeDir = _path;
#else
    char resolved[PATH_MAX];
    const char *exe = dgs_argv[0];
    if (realpath(exe, resolved))
      exe = resolved;
    char *buffer = (char *)memalloc(strlen(exe) + 1, tmpmem);
    *buffer = 0;
    ::dd_get_fname_location(buffer, exe);
    mFullExeDir = buffer;
    memfree(buffer, tmpmem);
#endif
  }

  return mFullExeDir.str();
}

const char *get_start_path() { return mFullStartDir.str(); }

const char *get_common_data_dir()
{
  if (mCommonDataDir == "")
    mCommonDataDir = String(256, "%s/../commonData", get_exe_path_full());
  return mCommonDataDir.str();
}
} // namespace sgg
