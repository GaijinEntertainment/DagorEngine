//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if _TARGET_PC_WIN
#include <windows.h>
#include <malloc.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <debug/dag_log.h>
#endif

// Necessary, because some code runs before the logger is initialized.
struct WinDeferredStartupLogs
{
#if _TARGET_PC_WIN
  bool failed_SetProcessDpiAwarenessContext = false;
  bool failed_SetProcessDPIAware = false;
  bool failed_SetThreadDpiAwarenessContext = false;
#endif // _TARGET_PC_WIN
};

inline void win_set_process_dpi_aware(WinDeferredStartupLogs &deferredLogs)
{
#if _TARGET_PC_WIN
#if !defined(DAGOR_NO_DPI_AWARE) || (DAGOR_NO_DPI_AWARE != 1)
#if defined(DAGOR_NO_DPI_AWARE) && (DAGOR_NO_DPI_AWARE < 0)
  for (unsigned i = 1; i < __argc; i++)
    if (strcmp(__argv[i], "-noHDPI") == 0)
      return;
#endif

  if (HMODULE hm = LoadLibraryA("user32.dll"))
  {
    typedef DPI_AWARENESS_CONTEXT(WINAPI * PSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
    typedef BOOL(WINAPI * PSetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
    typedef BOOL(WINAPI * PSetProcessDPIAware)(void);
    typedef UINT(WINAPI * PGetDpiForSystem)(void);

    auto SetThreadDpiAwarenessContext = (PSetThreadDpiAwarenessContext)(void *)GetProcAddress(hm, "SetThreadDpiAwarenessContext");
    auto SetProcessDpiAwarenessContext = (PSetProcessDpiAwarenessContext)(void *)GetProcAddress(hm, "SetProcessDpiAwarenessContext");
    auto SetProcessDPIAware = (PSetProcessDPIAware)(void *)GetProcAddress(hm, "SetProcessDPIAware");
    auto GetDpiForSystem = (PGetDpiForSystem)(void *)GetProcAddress(hm, "GetDpiForSystem");

    // Try multiple ways to set DPI awareness.
    // See https://learn.microsoft.com/en-us/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process
    bool isAwarenessSetForProcess = false;

    // Windows 10+. Preferred, best way.
    if (SetProcessDpiAwarenessContext)
    {
      isAwarenessSetForProcess = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
      if (!isAwarenessSetForProcess)
        deferredLogs.failed_SetProcessDpiAwarenessContext = true;
    }

    // Windows Vista+. Does not handle multi-monitor setups with different DPI settings.
    if (!isAwarenessSetForProcess && SetProcessDPIAware)
    {
      auto res = SetProcessDPIAware();
      if (!res)
        deferredLogs.failed_SetProcessDPIAware = true;
    }

    // Windows 10+, fallback path. Original comment:
    // The DPI scaling has been disabled to get a pixel to pixel rendering, except the
    // following cases:
    // 1. Change monitor DPI scaling without reboot. PER_MONITOR_AWARE flag is
    // required to override system scaling after that.
    // 2. Set DPI scaling compatibility options for far.exe and start the game from
    // it. Only SetThreadDpiAwarenessContext can override inherited process options.
    // Both are available only on Windows 10, so keep SetProcessDPIAware as a
    // fallback.
    // Fix: Incorrect window size and blurred picture in some cases.
    //
    // > SetProcessDpiAwarenessContext does not work here because it cannot override compatibility
    // > options of the parent process (far.exe for example), while the SetThreadDpiAwarenessContext can.
    //
    // In cases, where SetProcessDpiAwarenessContext has failed, also using this seems better than just
    // SetProcessDPIAware. However, it may lead to subtle bugs when WinAPI calls are made outside of the
    // main thread. Even if we don't make these calls explicitly, 3rd-party code might.
    if (!isAwarenessSetForProcess && SetThreadDpiAwarenessContext)
    {
      auto res = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
      if (res == NULL)
        deferredLogs.failed_SetThreadDpiAwarenessContext = true;
    }

    // This must be called after setting the DPI awareness context, otherwise it will return the default value.
    if (GetDpiForSystem)
      win32_system_dpi = GetDpiForSystem();

    FreeLibrary(hm);
  }
#endif // DAGOR_NO_DPI_AWARE
#else
  (void)deferredLogs;
#endif
}

inline void win_recover_systemroot_env()
{
#if _TARGET_PC_WIN
  DWORD len = GetEnvironmentVariableW(L"SystemRoot", NULL, 0);
  wchar_t *varBuf = (wchar_t *)alloca((len > MAX_PATH ? len : MAX_PATH) * sizeof(wchar_t));
  WIN32_FILE_ATTRIBUTE_DATA attrs;
  if (len && GetEnvironmentVariableW(L"SystemRoot", varBuf, len) && GetFileAttributesExW(varBuf, GetFileExInfoStandard, &attrs) &&
      (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    return;
  if (GetWindowsDirectoryW(varBuf, MAX_PATH))
    SetEnvironmentVariableW(L"SystemRoot", varBuf);
#endif
}
