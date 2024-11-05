// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fs_utils.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <string>
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

// https://stackoverflow.com/questions/74451/getting-actual-file-name-with-proper-casing-on-windows/81493#81493
std::string get_actual_path_name(const char *path)
{
  const char kSeparator = '\\';
  int length = int(strlen(path));
  char buffer[MAX_PATH];
  memcpy(buffer, path, (length + 1) * sizeof(path[0]));

  size_t i = 0;

  std::string result;

  if (length >= 2 && buffer[0] == kSeparator && buffer[1] == kSeparator)
  {
    int skippedCount = 0;
    i = 2; // start after '\\'
    while (i < length && skippedCount < 2)
    {
      if (buffer[i] == kSeparator)
        ++skippedCount;
      ++i;
    }

    result.append(buffer, i);
  }
  else if (length >= 2 && buffer[1] == L':')
  {
    result += towupper(buffer[0]);
    result += L':';
    if (length >= 3 && buffer[2] == kSeparator)
    {
      result += kSeparator;
      i = 3; // start after drive, colon and separator
    }
    else
    {
      i = 2; // start after drive and colon
    }
  }

  size_t lastComponentStart = i;
  bool addSeparator = false;

  while (i < length)
  {
    while (i < length && buffer[i] != kSeparator)
      ++i;

    if (addSeparator)
      result += kSeparator;

    // if we found path separator, get real filename of this
    // last path name component
    bool foundSeparator = (i < length);
    buffer[i] = 0;
    SHFILEINFOA info;

    // nuke the path separator so that we get real name of current path component
    info.szDisplayName[0] = 0;
    if (SHGetFileInfoA(buffer, 0, &info, sizeof(info), SHGFI_DISPLAYNAME))
    {
      result += info.szDisplayName;
    }
    else
    {
      result.append(buffer + lastComponentStart, i - lastComponentStart);
    }

    if (foundSeparator)
      buffer[i] = kSeparator;

    ++i;
    lastComponentStart = i;
    addSeparator = true;
  }

  return result;
}

bool is_file_case_valid(const char *file_name)
{
  if (!file_name)
    return false;

  char full_path[MAX_PATH] = {0};
  GetFullPathNameA(file_name, MAX_PATH, full_path, nullptr);
  std::string actual = get_actual_path_name(full_path);
  if (actual != full_path)
  {
    if (full_path[0] && full_path[1] == ':' && actual.length() >= 2)
      if (!strcmp(full_path + 1, actual.c_str() + 1))
        return true;

    return false;
  }
  else
    return true;
}

#else

#include <unistd.h>

bool change_dir(const char *dir) { return chdir(dir) == 0; }

bool is_file_case_valid(const char *file_name) { return !!file_name; }

eastl::string resolve_absolute_file_name(const char *file_name)
{
  char buffer[PATH_MAX] = {0};
  if (realpath(file_name, buffer))
    return eastl::string(buffer);
  else
    return eastl::string(file_name);
}


#endif
