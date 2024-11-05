// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_simpleString.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_unicode.h>
#include <libTools/util/strUtil.h>
#include <windows.h>
#include <direct.h>

namespace sgg
{
SimpleString mExeDir;
SimpleString mFullExeDir;
SimpleString mFullStartDir;


void set_exe_path(const char path[])
{
  mExeDir = path;

  char start_path[512];
  ::getcwd(start_path, 512);
  ::make_slashes(start_path);
  mFullStartDir = start_path;
}


const char *get_exe_path() { return mExeDir.str(); }

const char *get_exe_path_full()
{
  if (mFullExeDir == "")
  {
    wchar_t wpath[512];
    String _path;
    GetModuleFileNameW(GetModuleHandle(NULL), wpath, 512);
    convert_to_utf8(_path, wpath);
    ::dd_get_fname_location(_path, _path);

    mFullExeDir = _path;
  }

  return mFullExeDir.str();
}

const char *get_start_path() { return mFullStartDir.str(); }
} // namespace sgg
