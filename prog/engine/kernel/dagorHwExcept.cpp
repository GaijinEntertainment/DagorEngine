#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>

#if _TARGET_PC_WIN
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <eh.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <util/dag_stdint.h>
#include <util/limBufWriter.h>
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

struct MinidumpExceptionData
{
  PMINIDUMP_EXCEPTION_INFORMATION excInfo = nullptr;
  uint64_t stackBase = 0; // crashed thread
  uint64_t stackEnd = 0;
  uint64_t baseOfImage = 0; // module containing this file
  uint64_t endOfImage = 0;
  int scannedRegs = 0;

  struct MemoryRange
  {
    uint64_t base;
    uint32_t size;
  };
  uint32_t memRemoveQueueSize = 0;
  MemoryRange memRemoveQueue[32]{};
};

static BOOL dump_memory(uintptr_t addr, uintptr_t bytes_fwd, uintptr_t bytes_back, MinidumpExceptionData *data,
  PMINIDUMP_CALLBACK_OUTPUT callback_output)
{
  bool isStack = addr >= data->stackEnd && addr < data->stackBase;
  bool isInGameModule = addr >= data->baseOfImage && addr < data->endOfImage;
  if (addr > 0x100000 && !(uint64_t(addr) >> 48) && !isStack)
  {
    MEMORY_BASIC_INFORMATION mem;
    if (VirtualQuery((void *)addr, &mem, sizeof(mem)))
    {
      uintptr_t start = max(addr - bytes_back, (uintptr_t)mem.BaseAddress);
      uintptr_t end = min(addr + bytes_fwd, (uintptr_t)mem.BaseAddress + (uintptr_t)mem.RegionSize);
      const int dataProt = PAGE_READONLY | PAGE_READWRITE;
      const int codeProt = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE;
      bool excludeAsKnownCode = (mem.Protect & codeProt) && isInGameModule;
      if ((mem.State & MEM_COMMIT) && (mem.Protect & (dataProt | codeProt)) && !excludeAsKnownCode && end > start)
      {
        callback_output->MemoryBase = int64_t(intptr_t(start));
        callback_output->MemorySize = end - start;
        return TRUE;
      }
    }
  }
  return FALSE;
}

static BOOL minidump_memory_callback(MinidumpExceptionData *data, PMINIDUMP_CALLBACK_OUTPUT callback_output)
{
#if _TARGET_64BIT
  const int regsToScan = 16;
  const uint64_t *regsBase = (uint64_t *)&data->excInfo->ExceptionPointers->ContextRecord->Rax;
#else
  const int regsToScan = 7;
  const uint32_t *regsBase = (uint32_t *)&data->excInfo->ExceptionPointers->ContextRecord->Edi;
#endif
  while (data->scannedRegs < regsToScan)
  {
    uintptr_t regValue = regsBase[data->scannedRegs++];
    if (dump_memory(regValue, 128 /*fwd*/, 32 /*back*/, data, callback_output))
      return TRUE;
  }
  return FALSE;
}

static BOOL CALLBACK minidump_callback(PVOID callback_param, const PMINIDUMP_CALLBACK_INPUT callback_input,
  PMINIDUMP_CALLBACK_OUTPUT callback_output)
{
  MinidumpExceptionData *data = (MinidumpExceptionData *)callback_param;
  switch (callback_input->CallbackType) //-V::1037 Two or more case-branches perform the same actions.
  {
    case ModuleCallback:
      if (uint64_t(&minidump_callback) > callback_input->Module.BaseOfImage &&
          uint64_t(&minidump_callback) < callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage)
      {
        data->baseOfImage = callback_input->Module.BaseOfImage;
        data->endOfImage = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;
      }
      // Here is common filter effective in most cases, but I prefer to have full memory map
      // if (!(callback_output->ModuleWriteFlags & ModuleReferencedByMemory))
      // callback_output->ModuleWriteFlags &= ~ModuleWriteModule;
      return TRUE;

    case IncludeThreadCallback:
    {
      bool saveStack = true;
      if (callback_input->IncludeThread.ThreadId == data->excInfo->ThreadId ||
          callback_input->IncludeThread.ThreadId == get_main_thread_id() ||
          DaThread::isDaThreadWinUnsafe(callback_input->IncludeThread.ThreadId, saveStack))
      {
        callback_output->ThreadWriteFlags = ThreadWriteThread | ThreadWriteContext;
        if (saveStack)
          callback_output->ThreadWriteFlags |= ThreadWriteStack;
        if (callback_input->IncludeThread.ThreadId == data->excInfo->ThreadId)
          callback_output->ThreadWriteFlags |= ThreadWriteInstructionWindow;
        return TRUE;
      }
      return FALSE;
    }

    case MemoryCallback: return minidump_memory_callback(data, callback_output);

    case RemoveMemoryCallback:
      if (data->memRemoveQueueSize)
      {
        MinidumpExceptionData::MemoryRange range = data->memRemoveQueue[--data->memRemoveQueueSize];
        callback_output->MemoryBase = range.base;
        callback_output->MemorySize = range.size;
        return TRUE;
      }
      return FALSE;

    case ThreadCallback:
    case ThreadExCallback:
      if (callback_input->Thread.ThreadId == data->excInfo->ThreadId)
      {
        data->stackBase = callback_input->Thread.StackBase;
        data->stackEnd = callback_input->Thread.StackEnd;
      }
      if (!(callback_output->ThreadWriteFlags & ThreadWriteStack))
      {
        // ThreadWriteContext saves stack, so we need to strip it manually
#if _TARGET_64BIT
        uint64_t sp = callback_input->Thread.Context.Rsp;
#else
        uint64_t sp = int64_t(intptr_t(callback_input->Thread.Context.Esp));
#endif
        uint64_t base = sp + 128;
        uint32_t size = uint32_t(uintptr_t(callback_input->Thread.StackBase) - uintptr_t(base));
        if (int(size) > 0)
        {
          if (data->memRemoveQueueSize < countof(data->memRemoveQueue))
            data->memRemoveQueue[data->memRemoveQueueSize++] = MinidumpExceptionData::MemoryRange{base, size};
          else
            callback_output->ThreadWriteFlags = 0;
        }
      }
      return TRUE;

    case IncludeModuleCallback: return TRUE;

    default: return FALSE;
  }
}

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

    int fn_slen = i_strlen(debug_internal::dbgCrashDumpPath);
    wchar_t *fn_u16 = (wchar_t *)alloca((fn_slen + 1) * sizeof(wchar_t));
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, debug_internal::dbgCrashDumpPath, fn_slen + 1, fn_u16, fn_slen + 1) == 0)
      MultiByteToWideChar(CP_ACP, 0, debug_internal::dbgCrashDumpPath, fn_slen + 1, fn_u16, fn_slen + 1);

    HANDLE hDumpFile = CreateFileW(fn_u16, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE != hDumpFile)
    {
      MINIDUMP_EXCEPTION_INFORMATION minidumpExcInfo = {::GetCurrentThreadId(), eptr, FALSE};
      MINIDUMP_TYPE minidump_type = (MINIDUMP_TYPE)(MiniDumpScanMemory | MiniDumpWithIndirectlyReferencedMemory);
      MinidumpExceptionData param;
      param.excInfo = &minidumpExcInfo;
      MINIDUMP_CALLBACK_INFORMATION mci;
      mci.CallbackRoutine = minidump_callback;
      mci.CallbackParam = (void *)&param;

      if (MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hDumpFile, minidump_type, &minidumpExcInfo, NULL, &mci))
      {
        debug_internal::dbgCrashDumpPath[0] = 0; // no more dumps
      }

      CloseHandle(hDumpFile);
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
void DagorHwException::reportException(DagorException &caught_except, bool terminate, const char * /*add_dump_fname*/
)
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
