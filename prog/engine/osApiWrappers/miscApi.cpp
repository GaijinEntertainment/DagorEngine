// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_lockProfiler.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <supp/_platform.h>
#include <util/dag_globDef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#if _TARGET_APPLE
#include <sys/types.h>
#include <sys/sysctl.h>
#elif _TARGET_PC_LINUX
#include <fcntl.h>
#include <sys/stat.h>
#elif _TARGET_C3

#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_XBOX
#include "gdk/misc_api.h"
#elif _TARGET_PC_WIN
#include <windows.h>
#elif _TARGET_ANDROID

#include <supp/dag_android_native_app_glue.h>

extern "C" void android_native_app_destroy(android_app *app);
extern void win32_set_instance(void *hinst);

#endif

#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | _TARGET_C3
#include <pthread.h>
#include <sched.h>
#endif

#ifdef __GNUC__
#include <unistd.h>
#endif

#if _TARGET_ANDROID
#include "android/and_debuggerPresence.inc.cpp"
#endif

void sleep_usec(uint64_t us)
{
#if _TARGET_PC_WIN | _TARGET_XBOX
  uint32_t ms = us / 1000; // convert msec
  if (ms == 0)
    SwitchToThread(); // Sleep(0) doesn't give up cpu to lower priority threads
  else
    Sleep(ms);
#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_C3

#elif defined(__GNUC__)
  if (us == 0)
    sched_yield();
  else
  {
    struct timespec req, rem;
    req.tv_sec = us / 1000000;
    req.tv_nsec = ((us % 1000000) * 1000);
    while (1)
    {
      if (nanosleep(&req, &rem) < 0 && errno == EINTR)
        req = rem; // sleep was interrupted by signal, continue sleeping
      else
        break;
    }
  }
#endif
}

void sleep_msec(int ms)
{
  ScopeLockProfiler<da_profiler::DescSleep> lp;
  G_UNUSED(lp);
  sleep_usec(uint64_t(ms < 0 ? 0 : ms) * 1000ULL);
}

void terminate_process(int code)
{
#if _TARGET_PC_WIN
  TerminateProcess(GetCurrentProcess(), code);
#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_APPLE | _TARGET_PC_LINUX
  _exit(code);
#elif _TARGET_ANDROID

#if DAGOR_DBGLEVEL > 0
  // disable automatic app restart in debug mode
  G_UNUSED(code);
  android_app *state = (android_app *)win32_get_instance();
  win32_set_instance(nullptr);
  if (state != nullptr)
  {
    android::activity_finish(state->activity);
    android_native_app_destroy(state);
  }

  abort();
#else
  _exit(code);
#endif // DAGOR_DBGLEVEL

#elif _TARGET_XBOX
  TerminateProcess(GetCurrentProcess(), code); //==
#endif
}

int get_local_time(DagorDateTime *outTime)
{
  if (!outTime)
    return -1;
  int ret;
#if _TARGET_PC_WIN
  SYSTEMTIME dt;
  GetLocalTime(&dt);
  outTime->year = dt.wYear;
  outTime->month = dt.wMonth;
  outTime->day = dt.wDay;
  outTime->hour = dt.wHour;
  outTime->minute = dt.wMinute;
  outTime->second = dt.wSecond;
  outTime->microsecond = dt.wMilliseconds * 1000;
  ret = 0;
#else
  time_t rawtime = time(NULL);
  tm *t = (rawtime == ((time_t)-1)) ? NULL : localtime(&rawtime);
  ret = t ? 0 : (errno ? errno : -1);
  if (t)
  {
    outTime->year = t->tm_year + 1900;
    outTime->month = t->tm_mon + 1;
    outTime->day = t->tm_mday;
    outTime->hour = t->tm_hour;
    outTime->minute = t->tm_min;
    outTime->second = t->tm_sec;
    outTime->microsecond = 0;
  }
#endif
  return ret;
}

int get_timezone_minutes(long &outTimezone)
{
  int ret = 0;
#if _TARGET_PC_WIN
  TIME_ZONE_INFORMATION tzi{};
  // https://learn.microsoft.com/en-us/windows/win32/api/timezoneapi/nf-timezoneapi-gettimezoneinformation
  DWORD res = GetTimeZoneInformation(&tzi);
  if (res == TIME_ZONE_ID_INVALID)
    return ret;

  outTimezone = tzi.Bias;
  if (res == TIME_ZONE_ID_STANDARD)
    outTimezone += tzi.StandardBias;
  else if (res == TIME_ZONE_ID_DAYLIGHT)
    outTimezone += tzi.DaylightBias;

  // GetTimeZoneInformation returns Bias from formula: UTC = local + Bias
  // but we need local = UTC + Bias
  // so the sign is wrong
  outTimezone = -outTimezone;
  ret = 1;
#endif
  // TODO: implement for other platforms
  G_UNUSED(outTimezone);
  return ret;
}

int get_process_uid()
{
#if _TARGET_PC_WIN
  return ::GetCurrentProcessId();
#elif _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_ANDROID
  return getpid();
#else
  return 1; //== other platforms use single-tasking, so pseudo-ID is sufficient
#endif
}

static const char *target_platform_names[TP_TOTAL] = {"unknown", "win32", "win64", "iOS", "android", "macosx", "ps3", "ps4", "xbox360",
  "linux64", "linux32", "xboxOne", "xboxScarlett", "tvOS", "nswitch", "ps5"};

const char *get_platform_string_by_id(TargetPlatform id)
{
  if (id > TP_UNKNOWN && id < TP_TOTAL)
    return target_platform_names[id];

  return target_platform_names[TP_UNKNOWN];
}

TargetPlatform get_platform_id_by_string(const char *name)
{
  if (name != nullptr && name[0] != 0)
  {
    for (int i = TP_WIN32; i < TP_TOTAL; ++i)
    {
      if (strcmp(name, target_platform_names[i]) == 0)
        return (TargetPlatform)i;
    }
    if (strcmp(name, "win32_wow64") == 0)
      return TP_WIN32;
  }

  return TP_UNKNOWN;
}

const char *get_platform_string_id() { return get_platform_string_by_id(get_platform_id()); }

void *get_console_window_handle()
{
#if _TARGET_PC_WIN
  return GetConsoleWindow();
#else
  return nullptr;
#endif
}

void flash_window(void *wnd_handle, bool always_flash)
{
#if _TARGET_PC_WIN
  if (!wnd_handle)
    wnd_handle = win32_get_main_wnd();

  if (always_flash || GetForegroundWindow() != (HWND)wnd_handle)
  {
    FLASHWINFO flashInfo;
    memset(&flashInfo, 0, sizeof(flashInfo));

    flashInfo.cbSize = sizeof(flashInfo);
    flashInfo.hwnd = (HWND)wnd_handle;
    flashInfo.uCount = 8;
    flashInfo.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;

    FlashWindowEx(&flashInfo);
  }
#else
  (void)wnd_handle;
  (void)always_flash;
#endif
}

#if _TARGET_PC_WIN
bool get_version_ex(OSVERSIONINFOEXW *osversioninfo)
{
  // Deprecated GetVersionEx returns 6.2 on Windows 10 build 1511 (10.0.10586.0) even with manifest trick,
  // while suggested replacements like IsWindowsVersionOrGreater are not available on Windows XP.

  if (!osversioninfo)
    return false;

  HMODULE ntdllHandle = GetModuleHandle("ntdll");
  if (ntdllHandle)
  {
    long(WINAPI * rtlGetVersion)(LPOSVERSIONINFOEXW);
    *(FARPROC *)&rtlGetVersion = GetProcAddress(ntdllHandle, "RtlGetVersion");
    if (rtlGetVersion)
      return rtlGetVersion(osversioninfo) == 0;
  }

  // Fallback.
  return GetVersionExW((OSVERSIONINFOW *)osversioninfo);
}
#endif

WindowsVersion get_windows_version()
{
#if _TARGET_PC_WIN
  OSVERSIONINFOEXW osvi{};
  if (get_version_ex(&osvi))
    return {osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber};
#endif
  return {};
}

bool is_debugger_present()
{
#if DAGOR_DBGLEVEL == 0
  return false;
#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_PC_WIN || _TARGET_XBOX
  return ::IsDebuggerPresent() != 0;
#elif _TARGET_PC_LINUX
  int statusFd = open("/proc/self/status", O_RDONLY);
  if (statusFd == -1)
    return false;
  char buf[1024];
  ssize_t cnt = read(statusFd, buf, sizeof(buf) - 1);
  close(statusFd);
  if (cnt > 0)
  {
    buf[cnt] = 0;
    const char tracerPidStr[] = "TracerPid:";
    char *tracerPid = strstr(buf, tracerPidStr);
    if (tracerPid)
      return atoi(tracerPid + sizeof(tracerPidStr) - 1) != 0;
  }
  return false;
#elif _TARGET_APPLE
                                               // kinfo_proc structure is conditionalized by _APPLE_API_UNSTABLE,
                                               // so we should restrict use it to debug build only
#if DAGOR_DBGLEVEL > 0
  int junk;
  int mib[4];
  struct kinfo_proc info;
  size_t size;
  // Initialize the flags so that, if sysctl fails for some bizarre
  // reason, we get a predictable result.
  info.kp_proc.p_flag = 0;
  // Initialize mib, which tells sysctl the info we want, in this case
  // we're looking for information about a specific process ID.
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PID;
  mib[3] = getpid();
  // Call sysctl.
  size = sizeof(info);
  junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
  if (junk != 0)
    return false;
  // We're being debugged if the P_TRACED flag is set.
  return (info.kp_proc.p_flag & P_TRACED) != 0;
#else
  return false;
#endif
#elif _TARGET_ANDROID
  return isDebuggerPresent_android();
#else
  return false;
#endif
}


#if _TARGET_PC_WIN
static void *wine_get_version_proc()
{
  const HMODULE handle = ::GetModuleHandle("ntdll.dll");
  return (handle != 0) ? ::GetProcAddress(handle, "wine_get_version") : NULL;
}


static bool get_dll_version(const char *dllName, DWORD &versionMajor, DWORD &versionMinor)
{
  G_ASSERT(dllName != NULL && dllName[0] != 0);

  DWORD handle;
  const DWORD size = ::GetFileVersionInfoSize(dllName, &handle);

  if (size == 0)
    return false;

  void *buf = malloc(size);
  if (!buf)
    return false;

  if (::GetFileVersionInfo(dllName, handle, size, buf) == 0)
  {
    free(buf);
    return false;
  }

  VS_FIXEDFILEINFO *fileInfo = NULL;
  UINT len = 0;

  if (::VerQueryValue(buf, TEXT("\\"), (void **)(&fileInfo), &len) == 0 || fileInfo == NULL || len == 0)
  {
    free(buf);
    return false;
  }

  versionMajor = HIWORD(fileInfo->dwProductVersionMS);
  versionMinor = LOWORD(fileInfo->dwProductVersionMS);
  free(buf);
  return true;
}


bool detect_os_compatibility_mode(char *os_real_name, size_t os_real_name_size)
{
  if (os_real_name != NULL)
  {
    G_ASSERT(os_real_name_size > 0);
    os_real_name[0] = 0;
  }

  if (wine_get_version_proc() != NULL)
    return false;

  // https://stackoverflow.com/questions/57124/how-to-detect-true-windows-version/
  // Get the version that is returned from GetVersionEx()
  // and compare it to file version on 'kernel32.dll'
  DWORD dllVersionMajor, dllVersionMinor;
  if (get_dll_version("kernel32.dll", dllVersionMajor, dllVersionMinor))
  {
    OSVERSIONINFOEXW osvi;
    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (get_version_ex(&osvi))
    {
      if (dllVersionMajor != osvi.dwMajorVersion || dllVersionMinor != osvi.dwMinorVersion)
      {
        if (os_real_name != NULL)
        {
          SNPRINTF(os_real_name, os_real_name_size, "Windows %s v%d.%d",
            osvi.wProductType == VER_NT_WORKSTATION ? "Workstation" : "Server", (int)dllVersionMajor, (int)dllVersionMinor);
        }
        return true;
      }
    }
  }

  return false;
}
#else
bool detect_os_compatibility_mode(char *, size_t) { return false; }
#endif

#include <supp/dag_define_KRNLIMP.h>

static int64_t main_thread_id = -1;

#if _TARGET_STATIC_LIB
thread_local bool tls_is_main_thread = 0;
KRNLIMP void init_main_thread_id()
{
  main_thread_id = get_current_thread_id();
  tls_is_main_thread = 1;
}
#else
bool is_main_thread() { return get_current_thread_id() == get_main_thread_id(); }
KRNLIMP void init_main_thread_id() { main_thread_id = get_current_thread_id(); }
#endif

int64_t get_main_thread_id() { return main_thread_id; }

int64_t get_current_thread_id()
{
  int64_t tid = -1;
#if __GNUC__
  tid = (int64_t)pthread_self();
#else
  tid = (int64_t)GetCurrentThreadId();
#endif
  return tid;
}

static inline ConsoleModel get_console_model_impl()
{
#if _TARGET_C1 || _TARGET_C2

#elif _TARGET_XBOX
  return xbox_get_console_model();
#elif _TARGET_C3

#else
  return ConsoleModel::UNKNOWN;
#endif
}


ConsoleModel get_console_model()
{
  static ConsoleModel cached = get_console_model_impl();
  return cached;
}


const char *get_console_model_revision(ConsoleModel model)
{
  switch (model)
  {
    case ConsoleModel::PS4_PRO:
    case ConsoleModel::PS5_PRO: return "PRO";
    case ConsoleModel::XBOXONE_X: return "X";
    case ConsoleModel::XBOXONE_S: return "S";
    case ConsoleModel::XBOX_LOCKHART: return "LOCKHART";
    case ConsoleModel::XBOX_ANACONDA: return "ANACONDA";
    default: return "";
  }
}


#define EXPORT_PULL dll_pull_osapiwrappers_miscApi
#include <supp/exportPull.h>
