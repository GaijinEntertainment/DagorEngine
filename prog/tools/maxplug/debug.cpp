// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <shlobj.h>
#include <maxversion.h>
#include <string>
#include <fstream>
#include <filesystem>
#include "debug.h"
#include "common.h"

namespace fs = std::filesystem;

static std::ofstream debugfile;
static bool debug_file_available = true;

void debug(const char *s, ...)
{
  va_list ap;
  va_start(ap, s);
  CStr res;
  res.vprintf(s, ap);
  va_end(ap);

  debug(L"%s", strToWide(res.data()).c_str());
}
void debug(const wchar_t *s, ...)
{
  va_list ap;
  va_start(ap, s);
  TSTR res;
  res.vprintf(s, ap);
  va_end(ap);
  res += L"\n";

  OutputDebugStringW(res.data());
  if (!debug_file_available)
    return;

  if (!debugfile.is_open())
  {
    fs::path debugfile_name =
      format_str(L"dagor2_plugin_max%d.%d.%d.log", MAX_PRODUCT_VERSION_MAJOR, MAX_PRODUCT_VERSION_MINOR, MAX_PRODUCT_VERSION_POINT);

    TCHAR folder[MAX_PATH];
    fs::path debugfile_path;
    if (SUCCEEDED(SHGetFolderPath(NULL, (CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE), NULL, 0, folder)))
      debugfile_path = fs::path(folder) / debugfile_name;
    else
      debugfile_path = fs::path(L"d:\\") / debugfile_name;

    debugfile.open(debugfile_path);
    if (!debugfile)
    {
      debug_file_available = false;
      OutputDebugStringW(format_str(L"failed to create debug file: %s\n", debugfile_path.c_str()).c_str());
      return;
    }
  }

  debugfile << wideToStr(res.data());

  debugfile.flush();
}
