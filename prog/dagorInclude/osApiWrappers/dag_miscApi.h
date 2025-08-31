//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <osApiWrappers/dag_lockProfiler.h>
#include <supp/dag_define_KRNLIMP.h>
#if _TARGET_SIMD_SSE
#ifdef _MSC_VER
extern "C" void _mm_pause(void);
#pragma intrinsic(_mm_pause)
#else
#include <emmintrin.h> // _mm_pause
#endif
#endif


#define SPINS_BEFORE_SLEEP 8192


//! causes sleep for given time in msec (for Win32 uses Sleep)
KRNLIMP void sleep_msec(int time_msec);

//! causes sleep for given time in usec (for Win32 uses Sleep, which has 1 msec precision, using rounding!)
// this is not sleep_usec_precise by any means
KRNLIMP void sleep_usec(uint64_t time_usec);

//! signals to the processor that the thread is doing nothing. Not relevant to OS thread scheduling.
#if _TARGET_SIMD_SSE
#define cpu_yield _mm_pause
#elif defined(__arm__) || defined(__aarch64__)
#define cpu_yield() __asm__ __volatile__("yield")
#else
#define cpu_yield() ((void)0)
#endif

//! returns true when current thread is main thread
#if _TARGET_STATIC_LIB
extern thread_local bool tls_is_main_thread;
inline bool is_main_thread() { return tls_is_main_thread; }
#else
KRNLIMP bool is_main_thread();
#endif

//! returns ID main thread
KRNLIMP int64_t get_main_thread_id();
//! returns ID current thread
KRNLIMP int64_t get_current_thread_id();

//! terminates process (for Win32 uses TerminateProcess)
KRNLIMP void terminate_process(int code);

//! convert IP address (as in struct in_addr) to string (in xxx.yyy.zzz.www format)
//! uses inet_ntoa (winsock2)
//! Uses static buffer for conversion! not thread-safe, can't be used twice in same printf
KRNLIMP const char *ip_to_string(unsigned int ip);

//! convert string (in xxx.yyy.zzz.www format) to number for struct in_addr
//! uses inet_addr (winsock2)
KRNLIMP unsigned int string_to_ip(const char *str);

//! returns process unique ID (different for 2 simultaneously running processes)
KRNLIMP int get_process_uid();

//! structure to represent date/time
struct DagorDateTime
{
  unsigned short year;      // exact number (different range on different platforms)
  unsigned short month;     // [1..12]
  unsigned short day;       // [1..31]
  unsigned short hour;      // [0..23]
  unsigned short minute;    // [0..59]
  unsigned short second;    // [0..59], but on some platforms could be also 60 (for leap seconds)
  unsigned int microsecond; // [0..999999], but also could be always 0 on some platforms
};

KRNLIMP int get_local_time(DagorDateTime *outTime); // return zero on success

//! returns 0 if failed, 1 if succeeded. outTimezone is offset in minutes from UTC. current PC time = UTC + outTimezone
//! implemented for Windows only
KRNLIMP int get_timezone_minutes(long &outTimezone);

enum TargetPlatform : uint8_t
{
  // Change carefully, it's already stored in database
  TP_UNKNOWN = 0,
  TP_WIN32 = 1,
  TP_WIN64 = 2,
  TP_IOS = 3,
  TP_ANDROID = 4,
  TP_MACOSX = 5,
  TP_PS3 = 6, // dropped
  TP_PS4 = 7,
  TP_XBOX360 = 8, // dropped
  TP_LINUX64 = 9,
  TP_LINUX32 = 10, // dropped
  TP_XBOXONE = 11,
  TP_XBOX_SCARLETT = 12,
  TP_TVOS = 13,
  TP_NSWITCH = 14,
  TP_PS5 = 15,
  TP_TOTAL
};

constexpr TargetPlatform get_platform_id()
{
  TargetPlatform platform =
#if _TARGET_PC_WIN
#if _TARGET_64BIT
    TP_WIN64
#else
    TP_WIN32
#endif
#elif _TARGET_IOS
    TP_IOS
#elif _TARGET_ANDROID
    TP_ANDROID
#elif _TARGET_PC_MACOSX
    TP_MACOSX
#elif _TARGET_C1

#elif _TARGET_PC_LINUX
    TP_LINUX64
#elif _TARGET_XBOXONE
    TP_XBOXONE
#elif _TARGET_SCARLETT
    TP_XBOX_SCARLETT
#elif _TARGET_TVOS
    TP_TVOS
#elif _TARGET_C3

#elif _TARGET_C2

#else
#error "Undefined platform type"
    TP_UNKNOWN
#endif
    ;
  return platform;
}

KRNLIMP const char *get_platform_string_by_id(TargetPlatform id);

KRNLIMP TargetPlatform get_platform_id_by_string(const char *name);

//! returns string that uniquely identifies platform (that target was build for)
KRNLIMP const char *get_platform_string_id();

//! returns host platform (uses notation like in jBuild: windows, linux, macOS, etc.)
KRNLIMP const char *get_host_platform_string();
//! returns host platform architecture (uses notation like in jBuild: x86_64, arm64, x86, etc.)
KRNLIMP const char *get_host_arch_string();

//! flash window in the taskbar if inactive
KRNLIMP void flash_window(void *wnd_handle = NULL, bool always_flash = false);

KRNLIMP void *get_console_window_handle();

#if _TARGET_PC_WIN
//! replacement of GetVersionEx that is working on all versions from Windows XP to Windows 10 build 1511.
typedef struct _OSVERSIONINFOEXW OSVERSIONINFOEXW;
KRNLIMP bool get_version_ex(OSVERSIONINFOEXW *osversioninfo);
#endif

struct WindowsVersion
{
  uint32_t MajorVersion;
  uint32_t MinorVersion;
  uint32_t BuildNumber;
};

// On Windows will return the current OS' version number, on other platform will return 0 initialized object.
KRNLIMP WindowsVersion get_windows_version();

// Returns true if the current process is being debugged (either
// running under the debugger or has a debugger attached post facto).
KRNLIMP bool is_debugger_present();

// Returns real Windows version when run in compatibility mode for older Windows version.
KRNLIMP bool detect_os_compatibility_mode(char *os_real_name = NULL, size_t os_real_name_size = 0);

enum class ConsoleModel
{
  UNKNOWN = 0,
  PS4,
  PS4_PRO,
  XBOXONE,
  XBOXONE_S,
  XBOXONE_X,
  XBOX_LOCKHART,
  XBOX_ANACONDA,
  PS5,
  PS5_PRO,
  NINTENDO_SWITCH,
  TOTAL
};

KRNLIMP ConsoleModel get_console_model();
KRNLIMP const char *get_console_model_revision(ConsoleModel model);

template <typename F>
__forceinline void spin_wait_no_profile(F keep_waiting_cb, int init_spins = SPINS_BEFORE_SLEEP)
{
  int spinsLeftBeforeSleep = init_spins;
  while (keep_waiting_cb())
  {
    if (--spinsLeftBeforeSleep > 0)
      cpu_yield();
    else if (spinsLeftBeforeSleep > -(init_spins / 8))
      sleep_usec(0);
    else
    {
      sleep_usec(1000);
      spinsLeftBeforeSleep = 0;
    }
  }
}

template <typename F>
inline void spin_wait(F keep_waiting_cb, int spins = SPINS_BEFORE_SLEEP, uint32_t token = da_profiler::DescSleep,
  uint32_t threshold_us = DEFAULT_LOCK_PROFILER_USEC_THRESHOLD)
{
  if (!LOCK_PROFILER_ENABLED || keep_waiting_cb()) //-V547
  {
    lock_start_t reft = dagor_lock_profiler_start();
    spin_wait_no_profile(keep_waiting_cb, spins);
    dagor_lock_profiler_stop(reft, token, threshold_us);
  }
}

#include <supp/dag_undef_KRNLIMP.h>
