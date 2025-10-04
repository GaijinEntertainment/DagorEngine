// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>

#if _TARGET_PC_WIN
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <eh.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <debug/dag_minidumpCallback.h>
#include <util/dag_stdint.h>
#include <util/limBufWriter.h>
#include <util/dag_watchdog.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_threads.h>
#include <osApiWrappers/dag_miscApi.h>
#include <dbghelp.h>
#include <time.h>
#include <malloc.h>
#include <osApiWrappers/basePath.h>
#include "debugPrivate.h"
#include <osApiWrappers/winHwExceptUtils.h>

static constexpr int EXCEPT_BUF_SZ = 2048;
static char common_buf[EXCEPT_BUF_SZ];

static void __cdecl hard_except_handler_named(EXCEPTION_POINTERS *eptr, char *buf, int buf_len)
{
  G_ASSERT(eptr);
  G_ASSERT(eptr->ExceptionRecord);
  G_ASSERT(eptr->ContextRecord);

  if (!WinHwExceptionUtils::checkException(eptr->ExceptionRecord->ExceptionCode))
    return;

  interlocked_exchange(g_debug_is_in_fatal, 1);

#if DAGOR_DBGLEVEL > 0
  const char *call_stack = "";
  const char **call_stack_ptr = &call_stack;
#else
  const char **call_stack_ptr = NULL;
#endif


  WinHwExceptionUtils::parseExceptionInfo(eptr, "Exception occured. Game will be closed!", buf, buf_len, call_stack_ptr);

#if DAGOR_DBGLEVEL > 0
  if (*debug_internal::dbgCrashDumpPath)
#endif
  {
    WinHwExceptionUtils::setupCrashDumpFileName();

    int fn_slen = (int)strlen(debug_internal::dbgCrashDumpPath);
    wchar_t *fn_u16 = (wchar_t *)alloca((fn_slen + 1) * sizeof(wchar_t));
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, debug_internal::dbgCrashDumpPath, fn_slen + 1, fn_u16, fn_slen + 1) == 0)
      MultiByteToWideChar(CP_ACP, 0, debug_internal::dbgCrashDumpPath, fn_slen + 1, fn_u16, fn_slen + 1);

    HANDLE hDumpFile = CreateFileW(fn_u16, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE != hDumpFile)
    {
      MINIDUMP_EXCEPTION_INFORMATION minidumpExcInfo = {::GetCurrentThreadId(), eptr, FALSE};
      DagorHwException::MinidumpExceptionData param(eptr, minidumpExcInfo.ThreadId);
      MINIDUMP_CALLBACK_INFORMATION mci;
      mci.CallbackRoutine = DagorHwException::minidump_callback;
      mci.CallbackParam = (void *)&param;
      HANDLE hProcess = ::GetCurrentProcess();
      DWORD dwProcessId = ::GetCurrentProcessId();

      bool success = false;
      __try
      {
        success = MiniDumpWriteDump(hProcess, dwProcessId, hDumpFile, MiniDumpNormal, &minidumpExcInfo, NULL, &mci);
      }
      __except (DagorHwException::write_minidump_fallback(hProcess, dwProcessId, hDumpFile, GetExceptionInformation(), nullptr))
      {
        success = true;
        logerr("Exception in minidump callback was handled");
      }
      if (!success || !param.memCallbackCalled)
      {
        debug("Minidump with callback write error, use generic minidump (ok=%i, memCallbackCalled=%i)", success,
          param.memCallbackCalled);
        DagorHwException::write_minidump_fallback(hProcess, dwProcessId, hDumpFile, eptr, nullptr);
      }

      CloseHandle(hDumpFile);
      debug_internal::dbgCrashDumpPath[0] = 0; // no more dumps
    }
  }


  if (dgs_pre_shutdown_handler)
    dgs_pre_shutdown_handler();

#if DAGOR_DBGLEVEL > 0
  if (dgs_report_fatal_error)
    dgs_report_fatal_error("Exception", buf, call_stack);
#else
  if (dgs_report_fatal_error)
    dgs_report_fatal_error("Exception", buf, "");
#endif

  flush_debug_file();
  fflush(stdout);
  fflush(stderr);
  ExitProcess(2);
}


static long __stdcall hard_except_top_level(EXCEPTION_POINTERS *ep)
{
  hard_except_handler_named(ep, common_buf, sizeof(common_buf));

  return EXCEPTION_CONTINUE_SEARCH;
}


void DagorHwException::reportException(DagorException &caught_except, bool terminate, const char *)
{
  char stackInfo[8192];
  ::stackhlp_get_call_stack(stackInfo, sizeof(stackInfo), caught_except.getStackPtr(), 32);

  logmessage(LOGLEVEL_FATAL, "CATCHED EXCEPTION: %s\n%s", caught_except.excDesc, stackInfo);

  if (!terminate)
    return;

  if (dgs_pre_shutdown_handler)
    dgs_pre_shutdown_handler();

  if (dgs_report_fatal_error)
    dgs_report_fatal_error("Exception", caught_except.excDesc, stackInfo);

  flush_debug_file();
  fflush(stdout);
  fflush(stderr);
  ExitProcess(2);
}


int DagorHwException::setHandler(const char * /*thread*/)
{

  ::SetUnhandledExceptionFilter(&hard_except_top_level);
  return -1;
}


void DagorHwException::cleanup() {}


void DagorHwException::registerProductInfo(ProductInfo *) {}


void DagorHwException::updateProductInfo(ProductInfo *) {}


void DagorHwException::forceDump(const char *, const char *, int) {}


void *DagorHwException::getHandler() { return NULL; }


void DagorHwException::removeHandler(int) {}


#else // _TARGET_PC_WIN

#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_miscApi.h>

#if _TARGET_APPLE | _TARGET_PC_LINUX
#include <signal.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <execinfo.h>

extern const char *os_get_process_name();

static void top_lev_handler(int sig)
{
  interlocked_exchange(g_debug_is_in_fatal, 1);

#if DAGOR_DBGLEVEL > 0
  debug("signal %d received!", sig);
  debug_flush(true);
#endif

  flush_debug_file();
  fflush(stdout);
  fflush(stderr);

  if (dgs_pre_shutdown_handler)
    dgs_pre_shutdown_handler();
#if _TARGET_APPLE
#if _TARGET_TVOS
  return;
#endif
  logerr("see crash log in ~/Library/Logs/DiagnosticReports/%s_*.crash", os_get_process_name());
  flush_debug_file();

  signal(SIGILL, SIG_DFL);
  signal(SIGSEGV, SIG_DFL);
  signal(SIGBUS, SIG_DFL);
  return;
#endif

  static char buf[1024];
  _snprintf(buf, 1024, "signal %d received!\n", sig);

  stackhelp::CallStackCaptureStore<32> stack;
  stackhelp::ext::CallStackCaptureStore extStack;
  stack.capture();
  extStack.capture();

  if (dgs_report_fatal_error)
  {
    char stackInfo[8192];
    dgs_report_fatal_error("Exception", buf, get_call_stack(stackInfo, sizeof(stackInfo), stack, extStack));
  }

  flush_debug_file();
  fflush(stdout);
  fflush(stderr);
  terminate_process(2);

  interlocked_exchange(g_debug_is_in_fatal, 0);
}
#endif

#if _TARGET_ANDROID
#include "android/dagorHwExcept.inc.cpp"
AndroidCrashHandler android_crash_handler;
#endif

int DagorHwException::setHandler(const char * /*thread*/)
{
#if _TARGET_APPLE
  signal(SIGILL, &top_lev_handler);
  signal(SIGSEGV, &top_lev_handler);
  signal(SIGBUS, &top_lev_handler);
#elif _TARGET_ANDROID
#if ANDROID_ENABLE_SIGNAL_LOG
  android_crash_handler.setup();
#endif
#endif

#if _TARGET_IOS
  // do not crash on this signal, some sockets can trigger it and if not handled we may simple ignore it
  signal(SIGPIPE, SIG_IGN);
#endif
  return -1;
}
void DagorHwException::removeHandler(int /*id*/) {}
void DagorHwException::reportException(DagorException &caught_except, bool terminate, const char * /*add_dump_fname*/)
{
  out_debug_str_fmt("reportException: code=%d", caught_except.excCode);
  out_debug_str(caught_except.excDesc);
#ifdef _MSC_VER
  __debugbreak();
#endif
  if (terminate)
    terminate_process(2);
}
void DagorHwException::cleanup()
{
#if _TARGET_ANDROID
#if ANDROID_ENABLE_SIGNAL_LOG
  android_crash_handler.clear();
#endif
#endif
}
void DagorHwException::registerProductInfo(DagorHwException::ProductInfo *) {}
void DagorHwException::updateProductInfo(DagorHwException::ProductInfo *) {}

#endif

#define EXPORT_PULL dll_pull_kernel_dagorHwExcept
#include <supp/exportPull.h>
