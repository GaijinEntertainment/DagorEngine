// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../webbrowser.h"
#include "UnicodeString.h"

#include "host.h"

#if _TARGET_PC_WIN
#include <windows.h>
#include <sys/types.h>
#include <wchar.h>
#endif

#define CEF_MAX_PATH          4096


namespace webbrowser
{
static UnicodeString baseExeFolder;
static UnicodeString cefprocessPath;

void def_base_exe_folder()
{
  wchar_t tmp[CEF_MAX_PATH] = { 0 };
  if (size_t l = GetModuleFileNameW(NULL, tmp, CEF_MAX_PATH))
  {
    if (wchar_t *sep = wcsrchr(tmp, L'\\'))
      *sep = L'\0';

    baseExeFolder.set(tmp);
  }
}

bool have_binaries()
{
#if _TARGET_PC_WIN
  const wchar_t * const known_deps[] =
  {
    L"cefprocess.exe",
    L"d3dcompiler_47.dll",
    L"icudtl.dat",
    L"libEGL.dll",
    L"libGLESv2.dll",
    L"libcef.dll",
    L"chrome_elf.dll",
    L"snapshot_blob.bin",
    L"v8_context_snapshot.bin",
  };

  if (baseExeFolder.empty())
    def_base_exe_folder();

  wchar_t path[CEF_MAX_PATH] = { 0 };
  struct _stat unused;
  for (const auto d : known_deps)
  {
    if (_snwprintf(path, CEF_MAX_PATH, L"%s\\%s", baseExeFolder.wide(), d))
      if (_wstat(path, &unused) == -1)
        return false;
  }

  return true;
#else
  return true;
#endif
}

bool is_os_supported()
{
#if _TARGET_PC_WIN
  // Chromium 50+ does not support anything older than Win7 (v.6.1)
  OSVERSIONINFOEX osvi = { 0 };
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  osvi.dwMajorVersion = 6;
  osvi.dwMinorVersion = 1;

  DWORDLONG req = 0;
  VER_SET_CONDITION(req, VER_MAJORVERSION, VER_GREATER_EQUAL);
  VER_SET_CONDITION(req, VER_MINORVERSION, VER_GREATER_EQUAL);
  return VerifyVersionInfo(&osvi, VER_MAJORVERSION|VER_MINORVERSION, req);
#elif _TARGET_PC_MACOSX
  return true;
#else
  return false;
#endif
}

void set_cef_folder(const wchar_t* path)
{
  baseExeFolder.set(path);
}

const wchar_t* get_cefprocess_exe_path()
{
  if (baseExeFolder.empty())
    def_base_exe_folder();

  if (cefprocessPath.empty())
  {
    wchar_t ret[CEF_MAX_PATH] = { 0 };
    if (_snwprintf(ret, CEF_MAX_PATH, L"%s\\%s", baseExeFolder.wide(), L"cefprocess.exe"))
      cefprocessPath.set(ret);
    else
      cefprocessPath.set("cefprocess.exe");
  }

  return cefprocessPath.wide();
}

bool is_available()
{
  return is_os_supported() && have_binaries();
}

IBrowser *create(const IBrowser::Properties& p, IHandler& h, ILogger& l)
{
  return new Host(p, h, l);
}

} // namespace webbrowser
