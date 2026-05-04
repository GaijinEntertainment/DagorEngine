// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_winVersionQuery.h>

#if _TARGET_PC_WIN
#include <Windows.h>


bool get_version_ex(OSVERSIONINFOEXW *osversioninfo)
{
  // Deprecated GetVersionEx returns 6.2 on Windows 10 build 1511 (10.0.10586.0) even with manifest trick,
  // while suggested replacements like IsWindowsVersionOrGreater are not available on Windows XP.

  if (!osversioninfo)
  {
    return false;
  }

  if (HMODULE ntdllHandle = GetModuleHandle("ntdll"))
  {
    long(WINAPI * rtlGetVersion)(LPOSVERSIONINFOEXW);
    *(FARPROC *)&rtlGetVersion = GetProcAddress(ntdllHandle, "RtlGetVersion");
    if (rtlGetVersion)
      return rtlGetVersion(osversioninfo) == 0;
  }

  // Fallback.
  return GetVersionExW((OSVERSIONINFOW *)osversioninfo);
}

eastl::optional<WindowsVersion> get_windows_version(bool zero_initialize)
{
  if (OSVERSIONINFOEXW osvi{.dwOSVersionInfoSize = sizeof(osvi)}; get_version_ex(&osvi))
  {
    return WindowsVersion{
      .major = osvi.dwMajorVersion,
      .minor = osvi.dwMinorVersion,
      .build = osvi.dwBuildNumber,
    };
  }
  return zero_initialize ? eastl::optional{WindowsVersion{}} : eastl::nullopt;
}

#endif

#define EXPORT_PULL dll_pull_osapiwrappers_winVersionQuery
#include <supp/exportPull.h>
