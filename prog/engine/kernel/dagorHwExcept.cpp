#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>

#if _TARGET_PC_WIN
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <shlwapi.h>
#include <eh.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
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

#pragma comment(lib, "shlwapi.lib")

static constexpr int EXCEPT_BUF_SZ = 2048;
static char common_buf[EXCEPT_BUF_SZ];

const uintptr_t REGS_DUMP_MEMORY_FORWARD = 224;
const uintptr_t REGS_DUMP_MEMORY_BACK = 32;
const uintptr_t REGS_PTR_DUMP_SIZE = sizeof(uintptr_t) * 10;
const uintptr_t PTR_DUMP_SIZE = sizeof(uintptr_t) * 6;
const uintptr_t REG_SCAN_PTR_DATA_SIZE = sizeof(uintptr_t) * 16;
const uintptr_t STACK_SCAN_PTR_DATA_SIZE = sizeof(uintptr_t) * 6;
const uintptr_t MAX_VECTOR_ELEMENTS = 1024;
const uintptr_t VECTOR_PTR_DUMP_SIZE = 1024;
const uintptr_t VECTOR_MAX_ELEM_SIZE = 96; // enough for phys contacts data
const uintptr_t STRIPPED_STACK_SIZE = 256;

struct MemoryRange
{
  uintptr_t from = 0;
  uintptr_t to = 0;
  bool contain(uint64_t addr) const { return addr >= from && addr < to; }
};

struct MinidumpThreadData
{
  uintptr_t threadId = 0;
  bool needScanMem = false;
  uint32_t scannedRegs = 0;
#if _TARGET_64BIT
  static const uint32_t regsCount = 16;
#else
  static const uint32_t regsCount = 7;
#endif
  uintptr_t regs[16]{};
  uintptr_t stackPtr = 0;
  MemoryRange stack;

  void setRegisters(const CONTEXT &ctx)
  {
#if _TARGET_64BIT
    stackPtr = ctx.Rsp;
    const int64_t *regsSrc = (int64_t *)&ctx.Rax;
#else
    stackPtr = ctx.Esp;
    const int32_t *regsSrc = (int32_t *)&ctx.Edi;
#endif
    for (uint32_t i = 0; i < regsCount; i++)
      regs[i] = regsSrc[i];
  }
};

struct MinidumpExceptionData
{
  PMINIDUMP_EXCEPTION_INFORMATION excInfo = nullptr;
  MinidumpThreadData excThread;
  MinidumpThreadData mainThread;
  MemoryRange image; // main module containing this file
  MemoryRange code;

  MemoryRange ntdll;
  MemoryRange user32;
  MemoryRange kernel32;
  MemoryRange kernelBase;
  MemoryRange videoDriver;

  uint32_t memAddQueueSize = 0;
  MemoryRange memAddQueue[16];
  uint32_t memRemoveQueueSize = 0;
  MemoryRange memRemoveQueue[128];
};

static BOOL get_vector_at_addr(uintptr_t addr, uintptr_t end, uint32_t &out_vector_size, uint32_t &out_data_size)
{
  struct DagVectorData // das::Array has same layout
  {
    uintptr_t data;
    uint32_t size;
    uint32_t capacity;
    bool validateCounters() const { return size > 0 && size <= capacity && capacity <= MAX_VECTOR_ELEMENTS; }
  };
  const DagVectorData *dv = (const DagVectorData *)addr;
  if (addr + sizeof(DagVectorData) <= end && dv->validateCounters())
  {
    out_vector_size = sizeof(DagVectorData);
    out_data_size = min(VECTOR_PTR_DUMP_SIZE, dv->size * VECTOR_MAX_ELEM_SIZE);
    return TRUE;
  }

  struct DagVectorWithAllocData
  {
    uintptr_t data;
    uintptr_t allocator;
    uint32_t size;
    uint32_t capacity;
    bool validateCounters() const { return size > 0 && size <= capacity && capacity <= MAX_VECTOR_ELEMENTS; }
  };
  const DagVectorWithAllocData *va = (const DagVectorWithAllocData *)addr;
  if (addr + sizeof(DagVectorWithAllocData) <= end && va->validateCounters())
  {
    out_vector_size = sizeof(DagVectorWithAllocData);
    out_data_size = min(VECTOR_PTR_DUMP_SIZE, va->size * VECTOR_MAX_ELEM_SIZE);
    return TRUE;
  }

  struct EastlVectorData
  {
    uintptr_t data;
    uintptr_t end;
    uintptr_t capacityEnd;
    bool validateCounters() const { return data < end && end <= capacityEnd && capacityEnd - data < 10000; }
  };
  const EastlVectorData *ev = (const EastlVectorData *)addr;
  if (addr + sizeof(EastlVectorData) <= end && ev->validateCounters())
  {
    out_vector_size = sizeof(EastlVectorData);
    out_data_size = min(VECTOR_PTR_DUMP_SIZE, ev->end - ev->data);
    return TRUE;
  }

  return FALSE;
}

static BOOL get_memory_range_for_dump(uintptr_t addr, uintptr_t bytes_fwd, uintptr_t bytes_back, const MinidumpExceptionData *data,
  MemoryRange &range, bool &is_rw_data)
{
  if (addr < 0x100000 || uint64_t(addr) >> 48 || int32_t(addr) == -1)
    return FALSE;
  if (data->excThread.stack.contain(addr) || data->mainThread.stack.contain(addr))
    return FALSE;
  if (data->code.contain(addr)) // exclude game code (assume it's not changed)
    return FALSE;
  if (data->ntdll.contain(addr) || data->user32.contain(addr) || data->kernel32.contain(addr) || data->kernelBase.contain(addr))
    return FALSE;
  if (data->videoDriver.contain(addr))
    return FALSE;
  MEMORY_BASIC_INFORMATION mem;
  if (VirtualQuery((void *)addr, &mem, sizeof(mem)) && mem.State & MEM_COMMIT)
  {
    if (mem.Protect == PAGE_READONLY && data->image.contain(addr)) // exclude known constants
      return FALSE;
    uintptr_t start = max(addr - bytes_back, (uintptr_t)mem.BaseAddress);
    uintptr_t end = min(addr + bytes_fwd, (uintptr_t)mem.BaseAddress + (uintptr_t)mem.RegionSize);
    if (addr >= start && addr < end)
    {
      range.from = start;
      range.to = end;
      is_rw_data = mem.Protect == PAGE_READWRITE;
      return TRUE;
    }
  }
  return FALSE;
}

static BOOL dump_memory(uintptr_t addr, uintptr_t bytes_fwd, uintptr_t bytes_back, MinidumpExceptionData *data,
  PMINIDUMP_CALLBACK_OUTPUT callback_output, uint32_t scan_ptr_data_size, uint32_t scan_ptr_dump_len)
{
  MemoryRange range;
  bool isRwData = false;
  if (get_memory_range_for_dump(addr, bytes_fwd, bytes_back, data, range, isRwData))
  {
    callback_output->MemoryBase = int64_t(intptr_t(range.from)); // sign extend for win32
    callback_output->MemorySize = ULONG(range.to - range.from);
    if (isRwData && !(addr & sizeof(uintptr_t) - 1))
    {
      uintptr_t end = min(addr + scan_ptr_data_size, range.to);
      for (uintptr_t m = addr; m < end; m += sizeof(uintptr_t))
      {
        uintptr_t dumpPtr = *(uintptr_t *)m;
        uint32_t dumpLen = scan_ptr_dump_len;
        uint32_t vectorSize = 0;
        if (get_vector_at_addr(m, end, vectorSize, dumpLen))
          vectorSize -= sizeof(uintptr_t);
        if (get_memory_range_for_dump(dumpPtr, dumpLen, 0, data, range, isRwData))
        {
          m += vectorSize; // skip vector if pointer valid
          if (data->memAddQueueSize >= countof(data->memAddQueue))
          {
            logerr("memAddQueue overflow (%i)", countof(data->memAddQueue));
            break;
          }
          data->memAddQueue[data->memAddQueueSize++] = range;
        }
      }
    }
    return TRUE;
  }
  return FALSE;
}

static BOOL dump_thread_memory(MinidumpExceptionData *data, MinidumpThreadData &thread, PMINIDUMP_CALLBACK_OUTPUT callback_output)
{
  while (thread.scannedRegs < thread.regsCount)
  {
    uintptr_t regValue = thread.regs[thread.scannedRegs++];
    if (dump_memory(regValue, REGS_DUMP_MEMORY_FORWARD, REGS_DUMP_MEMORY_BACK, data, callback_output, REG_SCAN_PTR_DATA_SIZE,
          REGS_PTR_DUMP_SIZE))
      return TRUE;
  }
  if (thread.stackPtr >= thread.stack.from)
  {
    while (thread.stackPtr < thread.stack.to)
    {
      uintptr_t addr = *(uintptr_t *)thread.stackPtr;
      uint32_t len = PTR_DUMP_SIZE;
      uint32_t vectorSize = 0;
      if (get_vector_at_addr(thread.stackPtr, thread.stack.to, vectorSize, len))
        vectorSize -= sizeof(uintptr_t);
      thread.stackPtr += sizeof(uintptr_t);
      if (dump_memory(addr, len, 0 /*back*/, data, callback_output, STACK_SCAN_PTR_DATA_SIZE, PTR_DUMP_SIZE))
      {
        thread.stackPtr += vectorSize; // skip vector if .data valid
        return TRUE;
      }
    }
  }
  return FALSE;
}

static BOOL minidump_memory_callback(MinidumpExceptionData *data, PMINIDUMP_CALLBACK_OUTPUT callback_output)
{
  if (data->memAddQueueSize)
  {
    MemoryRange range = data->memAddQueue[--data->memAddQueueSize];
    callback_output->MemoryBase = int64_t(intptr_t(range.from));
    callback_output->MemorySize = ULONG(range.to - range.from);
    G_ASSERT(callback_output->MemorySize > 0 && callback_output->MemorySize <= 1024);
    return TRUE;
  }
  if (data->excThread.needScanMem && dump_thread_memory(data, data->excThread, callback_output))
    return TRUE;
  if (data->mainThread.needScanMem && dump_thread_memory(data, data->mainThread, callback_output))
    return TRUE;
  G_ASSERT(!data->memAddQueueSize);
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
        data->image.from = callback_input->Module.BaseOfImage;
        data->image.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;

        const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)data->image.from;
        const IMAGE_NT_HEADERS *pe = (const IMAGE_NT_HEADERS *)((char *)data->image.from + dos->e_lfanew);
        data->code.from = data->image.from + pe->OptionalHeader.BaseOfCode;
        data->code.to = data->code.from + pe->OptionalHeader.SizeOfCode;
      }
      else if (StrStrIW(callback_input->Module.FullPath, L"ntdll.dll"))
      {
        data->ntdll.from = callback_input->Module.BaseOfImage;
        data->ntdll.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;
      }
      else if (StrStrIW(callback_input->Module.FullPath, L"user32.dll"))
      {
        data->user32.from = callback_input->Module.BaseOfImage;
        data->user32.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;
      }
      else if (StrStrIW(callback_input->Module.FullPath, L"kernel32.dll"))
      {
        data->kernel32.from = callback_input->Module.BaseOfImage;
        data->kernel32.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;
      }
      else if (StrStrIW(callback_input->Module.FullPath, L"kernelbase.dll"))
      {
        data->kernelBase.from = callback_input->Module.BaseOfImage;
        data->kernelBase.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;
      }
      else if (StrStrIW(callback_input->Module.FullPath, L"nvwgf2um") || StrStrIW(callback_input->Module.FullPath, L"atidxx"))
      {
        data->videoDriver.from = callback_input->Module.BaseOfImage;
        data->videoDriver.to = callback_input->Module.BaseOfImage + callback_input->Module.SizeOfImage;
      }
      // Here is common filter effective in most cases, but I prefer to have full memory map
      // if (!(callback_output->ModuleWriteFlags & ModuleReferencedByMemory))
      // callback_output->ModuleWriteFlags &= ~ModuleWriteModule;
      return TRUE;

    case IncludeThreadCallback:
    {
      bool saveStack = true;
      bool allThreads = true;
      if (callback_input->IncludeThread.ThreadId == data->excInfo->ThreadId ||
          callback_input->IncludeThread.ThreadId == get_main_thread_id() ||
          DaThread::isDaThreadWinUnsafe(callback_input->IncludeThread.ThreadId, saveStack) || allThreads)
      {
        saveStack &= !is_watchdog_thread(callback_input->IncludeThread.ThreadId);
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
        MemoryRange range = data->memRemoveQueue[--data->memRemoveQueueSize];
        callback_output->MemoryBase = int64_t(intptr_t(range.from));
        callback_output->MemorySize = ULONG(range.to - range.from);
        return TRUE;
      }
      return FALSE;

    case ThreadCallback:
    case ThreadExCallback:
      if (callback_input->Thread.ThreadId == data->excInfo->ThreadId)
      {
        data->excThread.stack.from = callback_input->Thread.StackEnd;
        data->excThread.stack.to = callback_input->Thread.StackBase;
      }
      else if (callback_input->Thread.ThreadId == data->mainThread.threadId)
      {
        data->mainThread.stack.from = callback_input->Thread.StackEnd;
        data->mainThread.stack.to = callback_input->Thread.StackBase;
        data->mainThread.setRegisters(callback_input->Thread.Context);
      }
      if (!(callback_output->ThreadWriteFlags & ThreadWriteStack))
      {
        // ThreadWriteContext saves stack, so we need to strip it manually
#if _TARGET_64BIT
        uintptr_t sp = callback_input->Thread.Context.Rsp;
#else
        uintptr_t sp = int64_t(intptr_t(callback_input->Thread.Context.Esp));
#endif
        uintptr_t from = sp + STRIPPED_STACK_SIZE;
        uintptr_t to = uintptr_t(callback_input->Thread.StackBase);
        if (to > from)
        {
          if (data->memRemoveQueueSize < countof(data->memRemoveQueue))
            data->memRemoveQueue[data->memRemoveQueueSize++] = MemoryRange{from, to};
          else
            callback_output->ThreadWriteFlags = ThreadWriteThread;
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
      MinidumpExceptionData param;
      param.excInfo = &minidumpExcInfo;
      param.excThread.threadId = minidumpExcInfo.ThreadId;
      param.excThread.needScanMem = !is_watchdog_thread(minidumpExcInfo.ThreadId);
      param.excThread.setRegisters(*eptr->ContextRecord);
      param.mainThread.threadId = get_main_thread_id();
      param.mainThread.needScanMem = param.mainThread.threadId != param.excThread.threadId;
      MINIDUMP_CALLBACK_INFORMATION mci;
      mci.CallbackRoutine = minidump_callback;
      mci.CallbackParam = (void *)&param;

      if (MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &minidumpExcInfo, NULL, &mci))
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
