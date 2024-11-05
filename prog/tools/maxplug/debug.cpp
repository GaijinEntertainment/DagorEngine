// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <shlobj.h>
#include <maxversion.h>
#include <string>
#include "debug.h"

M_STD_STRING strToWide(const char *sz);
std::string wideToStr(const TCHAR *sw);


static FILE *debugfile = NULL;
static bool debug_not_possible = false;

void close_debug()
{
  if (!debugfile)
    return;
  fclose(debugfile);
  debugfile = NULL;
}

#define MAX_STR_DEBUG 8192

void debug(const char *s, ...)
{
  va_list ap;
  va_start(ap, s);

  static char tmp[MAX_STR_DEBUG];
  _vsnprintf(tmp, sizeof(tmp) - 1, s, ap);
  tmp[sizeof(tmp) - 1] = 0;
  va_end(ap);

  debug(L"%hs", tmp);
}
void debug(const wchar_t *s, ...)
{
  va_list ap;
  va_start(ap, s);

  static wchar_t tmp[MAX_STR_DEBUG];
  vswprintf(tmp, MAX_STR_DEBUG - 1, s, ap);
  tmp[MAX_STR_DEBUG - 2] = 0;
  wcscat(tmp, L"\n");
  va_end(ap);

  OutputDebugStringW(tmp);
  if (debug_not_possible)
    return;

  if (!debugfile)
  {
    TCHAR temp_path[MAX_PATH];
    TCHAR temp_path2[MAX_PATH];
    _stprintf(temp_path2, _T("d:\\dagor2_plugin_max%d.%d.%d"), MAX_PRODUCT_VERSION_MAJOR, MAX_PRODUCT_VERSION_MINOR,
      MAX_PRODUCT_VERSION_POINT);
#ifdef DAG_3DSMAX_IMPORTER
    _tcscat(temp_path2, _T("-imp.log"));
#else
    _tcscat(temp_path2, _T(".log"));
#endif
    if (SUCCEEDED(SHGetFolderPath(NULL, (CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE), NULL, 0, temp_path)))
      _tcscat(temp_path, temp_path2 + 2);
    else
      _tcscpy(temp_path, temp_path2);

    debugfile = _tfopen(temp_path, _T("wt"));
    if (!debugfile)
    {
      TCHAR tmpw[MAX_STR_DEBUG];
      debug_not_possible = true;
      _stprintf(tmpw, _T("failed to create debug file: %s\n"), temp_path);
      OutputDebugString(tmpw);
      return;
    }
  }

  size_t n = wcslen(tmp) * 3 + 1;
  char *stmp = new char[n];
  wcstombs(stmp, tmp, n);
  fputs(stmp, debugfile);
  delete[] stmp;

  fflush(debugfile);
}
