// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fs_utils.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include <direct.h>


eastl::string resolve_absolute_file_name(const char *file_name)
{
  char buffer[_MAX_DIR] = {0};
  if (GetFullPathNameA(file_name, sizeof(buffer), buffer, nullptr) == 0)
    return eastl::string(file_name);
  else
    return eastl::string(buffer);
}


bool change_dir(const char *dir) { return _chdir(dir) == 0; }

#else

#include <unistd.h>

bool change_dir(const char *dir) { return chdir(dir) == 0; }

eastl::string resolve_absolute_file_name(const char *file_name)
{
  char buffer[PATH_MAX] = {0};
  if (realpath(file_name, buffer))
    return eastl::string(buffer);
  else
    return eastl::string(file_name);
}


#endif
