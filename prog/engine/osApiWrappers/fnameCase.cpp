// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_direct.h>
#include <supp/_platform.h>
#include <string.h>

#if _TARGET_PC_WIN
bool dd_is_fname_case_valid(const char *fname)
{
  char full[MAX_PATH];
  DWORD len = GetFullPathNameA(fname, sizeof(full), full, nullptr);
  if (!len || len >= sizeof(full))
    return true;

  char *p = full;
  if (full[0] == '\\' && full[1] == '\\')
  {
    p = strchr(full + 2, '\\');
    p = p ? strchr(p + 1, '\\') : nullptr;
    if (!p)
      return true;
    p++;
  }
  else if (full[1] == ':')
    p = full + 3;

  while (*p)
  {
    char *sep = strchr(p, '\\');
    if (sep)
      *sep = 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(full, &fd);
    if (h != INVALID_HANDLE_VALUE)
    {
      FindClose(h);
      if (strcmp(fd.cFileName, p) != 0)
        return false;
    }
    if (!sep)
      break;
    *sep = '\\';
    p = sep + 1;
  }
  return true;
}
#else
bool dd_is_fname_case_valid(const char *) { return true; }
#endif

#define EXPORT_PULL dll_pull_osapiwrappers_fnameCase
#include <supp/exportPull.h>
