#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_stackHlpEx.h>
#include <osApiWrappers/dag_atomic.h>

#include "stackSize.h"
#if _TARGET_PC_WIN || XBOX_GDK_CALLSTACKS
#include <windows.h>
#include <dbghelp.h>
#include <tlhelp32.h>
#include <util/limBufWriter.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_spinlock.h>

#if _TARGET_64BIT
#define MODULE_BASE_TYPE DWORD64
#else
#define MODULE_BASE_TYPE ULONG
#endif


// One line suppression does not work.
//-V::566,720
#define HAS_GET_SYMBOL 1
OSSpinlock dbghelp_spinlock;

bool stackhlp_get_symbol(void *addr, uint32_t &line, char *filename, size_t max_file_name, char *symbolname, size_t max_symbol_name)
{
  // FileName and Line
  bool ret = true;
  DWORD64 dwAddress = (DWORD64)(addr);

  IMAGEHLP_LINE lineInfo;
  memset(&lineInfo, 0, sizeof(IMAGEHLP_LINE));
  lineInfo.SizeOfStruct = sizeof(lineInfo);
  DWORD dwDisp;
  filename[0] = symbolname[0] = 0;

  // All DbgHelp functions are single threaded.
  HANDLE hProcess = GetCurrentProcess();
  OSSpinlockScopedLock lock(dbghelp_spinlock);
  if (SymGetLineFromAddr(hProcess, dwAddress, &dwDisp, &lineInfo))
  {
    strncpy(filename, lineInfo.FileName, max_file_name);
    filename[max_file_name - 1] = 0;

    line = lineInfo.LineNumber;
  }
  else
    ret = false;

  // Function Name
  struct Sym : public IMAGEHLP_SYMBOL
  {
    char nm[512];
  } sym;
  sym.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
  sym.MaxNameLength = min(sizeof(sym.nm), max_symbol_name);

  ULONG_PTR offset = 0;
  if (SymGetSymFromAddr(hProcess, dwAddress, &offset, &sym))
  {
    strncpy(symbolname, sym.Name, max_file_name);
    symbolname[max_file_name - 1] = 0;
  }
  else
    ret = false;
  return ret;
}

const char *stackhlp_get_call_stack(char *buf, int out_buf_sz, const void *const *stack, unsigned max_size)
{
  LimitedBufferWriter lbw(buf, out_buf_sz);
  if (!stack)
  {
    lbw.aprintf("n/a");
    return buf;
  }

  unsigned stack_size = get_stack_size(stack, max_size);

  lbw.aprintf("Call stack (%d frames):\n", stack_size);

  // All DbgHelp functions are single threaded.
  // https://docs.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-undecoratesymbolname
  OSSpinlockScopedLock lock(dbghelp_spinlock);

  HANDLE ph = GetCurrentProcess();
  for (int i = 0; i < stack_size; i++)
  {
    struct Sym : public IMAGEHLP_SYMBOL
    {
      char nm[256];
    };
    IMAGEHLP_MODULE mod;
    Sym sym;
    ULONG_PTR ofs;
    unsigned long col = 0;
    IMAGEHLP_LINE line;
    bool sym_ok = false, line_ok = false;
    uintptr_t stk_addr = (uintptr_t)stack[i];

    sym.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    sym.MaxNameLength = sizeof(sym.nm);
    mod.SizeOfStruct = sizeof(IMAGEHLP_MODULE);
    line.SizeOfStruct = sizeof(line);

    if (SymGetSymFromAddr(ph, stk_addr, &ofs, &sym))
      sym_ok = true;

    lbw.aprintf("  %-8llX", (int64_t)stk_addr);
    if (SymGetModuleInfo(ph, stk_addr, &mod))
      lbw.aprintf(" %s!", mod.ModuleName);

    if (SymGetLineFromAddr(ph, stk_addr, &col, &line))
      line_ok = true;

    if (sym_ok)
    {
#if XBOX_GDK_CALLSTACKS
      lbw.aprintf(" %s", sym.Name);
#else
      char n[256];
      if (!UnDecorateSymbolName(sym.Name, n, 256,
            UNDNAME_NO_MEMBER_TYPE | UNDNAME_NO_ACCESS_SPECIFIERS | UNDNAME_NO_ALLOCATION_MODEL | UNDNAME_NO_MS_KEYWORDS))
        if (!SymUnDName(&sym, n, 128))
          strcpy(n, sym.Name);

      if (line_ok)
      {
        const char *fn = line.FileName;
        while (*fn == '.' || *fn == '\\' || *fn == '/')
          fn++;
        if (const char *fnp = strstr(fn, "\\dagor"))
          if ((fnp = strchr(fnp + 6, '\\')) != NULL)
            fn = fnp + 1;

        lbw.aprintf(" %s  +%u/%u\n    %s(%d)  +%d", n, ofs, sym.Size, fn, line.LineNumber, col);
      }
      else
        lbw.aprintf(" %s  +%u/%u", n, ofs, sym.Size);
#endif
    }

    if (line_ok && !sym_ok)
      lbw.aprintf("    File=%s\n in Line=%d,Col=%d", line.FileName, line.LineNumber, col);
    else if (!line_ok && !sym_ok)
      lbw.aprintf(" ?");
    lbw.aprintf("\n");
  }

  return buf;
}

void *stackhlp_get_bp() { return (void *)GetModuleHandle(NULL); }

intptr_t sampling_open_thread(intptr_t thread_id)
{
  return (intptr_t)OpenThread(THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
    FALSE, thread_id);
};

void sampling_close_thread(intptr_t target_thread) { CloseHandle((HANDLE)target_thread); };

bool can_unwind_callstack_for_other_thread() { return true; }

extern bool get_thread_handle_is_walking_stack(intptr_t thread_handle);

extern unsigned fill_stack_walk_naked(void **stack, unsigned size, CONTEXT &ctx, int frames_to_skip = 0);

int get_thread_handle_callstack(intptr_t target_thread, void **stack, uint32_t max_stack_size)
{
  HANDLE thread = (HANDLE)target_thread;
  int curStackIn = 0;
  CONTEXT ctx;
  if (SuspendThread(thread) == ~((DWORD)0)) //-V720
    return -1;
  ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
  const bool ctxCaptured = ::GetThreadContext(thread, &ctx) != 0;
  if (ctxCaptured)
  {
#if _TARGET_64BIT
    if (!get_thread_handle_is_walking_stack(target_thread))
#endif
      curStackIn = fill_stack_walk_naked(stack, max_stack_size, ctx);
  }
  ResumeThread(thread);
  return curStackIn;
}

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

static int get_thread_callstack(intptr_t thread_id, void **stack, uint32_t max_stack_size)
{
  intptr_t thread = sampling_open_thread(thread_id);
  if (thread == 0)
    return -1;
  const int ret = get_thread_handle_callstack((intptr_t)thread, stack, max_stack_size);
  sampling_close_thread(thread);
  return ret;
}

void dump_thread_callstack(intptr_t thread_id)
{
  void *stack[32];
  int curStackIn = get_thread_callstack(thread_id, stack, countof(stack));
  if (curStackIn > 0)
  {
    char buf[4096];
    debug("thread 0x%x(%d) - %s", (int)thread_id, (int)thread_id, stackhlp_get_call_stack(buf, sizeof(buf), stack, curStackIn));
  }
}


void dump_all_thread_callstacks()
{
#if !XBOX_GDK_CALLSTACKS
  HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (h != INVALID_HANDLE_VALUE)
  {
    DWORD cpid = GetCurrentProcessId();
    DWORD ctid = GetCurrentThreadId();
    int num_threads = 0;
    debug("\n\n\n=====begin threads dump=====\n");
    THREADENTRY32 te;
    te.dwSize = sizeof(te);
    if (Thread32First(h, &te))
    {
      // da_profiler::sync_stop_sampling();//todo: we have to stop sampling!
      do
      {
        if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID) && ctid != te.th32ThreadID &&
            cpid == te.th32OwnerProcessID)
        {
          debug_("#%d: ", num_threads);
          dump_thread_callstack(te.th32ThreadID);
          num_threads++;
        }
        te.dwSize = sizeof(te);
      } while (Thread32Next(h, &te));
    }
    CloseHandle(h);
    debug("\n\n\n=====end threads dump (%d total)=====\n", num_threads);
  }
#endif
}

#else
void dump_thread_callstack(intptr_t /*thread_id*/) {}
void dump_all_thread_callstacks() {}
#endif // DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

#elif _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID
#include <signal.h>
#include <util/limBufWriter.h>
#include <dlfcn.h>
#include <unistd.h>
#include <cxxabi.h>
#include <cstdlib>
#include <debug/dag_debug.h>
#include <pthread.h>
#include <osApiWrappers/dag_miscApi.h>
#if _TARGET_APPLE
#include <stdlib.h>
#include <mach/mach.h>
#elif _TARGET_PC_LINUX
#include <malloc.h>
#endif

#define HAS_GET_SYMBOL 1

bool stackhlp_get_symbol(void *addr, uint32_t &line, char *filename, size_t max_file_name, char *symbolname, size_t max_symbol_name)
{
  line = 0; // file_name and line no can also be provided, using address, see
            // https://github.com/bombela/backward-cpp/blob/master/backward.hpp
  // however, seems to exhausting now
  filename[0] = symbolname[0] = '\0';

  const char *symbol = "";

  Dl_info info;
  if (dladdr(addr, &info) && info.dli_sname)
    symbol = info.dli_sname;
  else
    return false;
  int status = -1;
  char *demangledName = __cxxabiv1::__cxa_demangle(symbol, nullptr, nullptr, &status);

  if (demangledName != nullptr)
    symbol = demangledName;

  if (symbol != nullptr)
  {
    strncpy(symbolname, symbol, max_symbol_name);
    symbolname[max_symbol_name - 1] = '\0';
  }

  if (demangledName != nullptr)
    std::free(demangledName);
  G_UNUSED(max_file_name); // file_name and line no can also be provided, using address, see
                           // https://github.com/bombela/backward-cpp/blob/master/backward.hpp
  return true;
}


const char *stackhlp_get_call_stack(char *buf, int out_buf_sz, const void *const *stack, unsigned max_size)
{
  LimitedBufferWriter lbw(buf, out_buf_sz);
  if (!stack)
  {
    lbw.aprintf("n/a");
    return buf;
  }

  unsigned stack_size = get_stack_size(stack, max_size);

  lbw.aprintf("Call stack (%d frames):\n", stack_size);

  for (size_t idx = 0; idx < stack_size; ++idx)
  {
    const void *addr = stack[idx];
    const char *symbol = "";

    Dl_info info;
    if (dladdr(addr, &info) && info.dli_sname)
      symbol = info.dli_sname;

    int status = -1;
    char *demangledName = __cxxabiv1::__cxa_demangle(symbol, nullptr, nullptr, &status);

    if (demangledName != nullptr)
      symbol = demangledName;

    lbw.aprintf(" %d: %p (%p) - %s", idx, addr, (uintptr_t)addr - (uintptr_t)info.dli_fbase, symbol);
    lbw.aprintf("\n");

    if (demangledName != nullptr)
      std::free(demangledName);
  }

  return buf;
}

void *stackhlp_get_bp() { return NULL; }

#if _TARGET_ANDROID
static uint32_t g_stacks_requested = 0;
static uint32_t g_stacks_processed = 0;

static void dump_thread_callstack_signal_handler(int, siginfo_t *, void *)
{
  dump_thread_callstack(get_current_thread_id());
  interlocked_add(g_stacks_processed, 1);
  interlocked_add(g_stacks_requested, -1);
}
#endif

void dump_thread_callstack(intptr_t thread_id)
{
  intptr_t currentId = get_current_thread_id();
  if (thread_id != 0 && thread_id != intptr_t(-1) && currentId != thread_id)
  {
#if _TARGET_ANDROID
    while (interlocked_acquire_load(g_stacks_requested) != 0)
      sleep_msec(10);

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = dump_thread_callstack_signal_handler;
    act.sa_flags = SA_RESTART | SA_SIGINFO;

    interlocked_release_store(g_stacks_processed, 0);
    interlocked_release_store(g_stacks_requested, 1);

    struct sigaction origAct;
    sigaction(SIGURG, &act, &origAct);

    if (tgkill(getpid(), pthread_gettid_np(thread_id), SIGURG) == 0)
    {
      while (interlocked_acquire_load(g_stacks_processed) != 1)
        sleep_msec(10);
    }

    sigaction(SIGURG, &origAct, nullptr);
#else
    debug("Can dump only current thread's stack!");
#endif
    return;
  }

#if _TARGET_ANDROID
  const int tid = pthread_gettid_np(thread_id);
#else
  const int tid = thread_id;
#endif

  stackhelp::CallStackCaptureStore<32> stack;
  stackhelp::ext::CallStackCaptureStore extStack;
  stack.capture();
  extStack.capture();
  char buf[4096];
  debug("======> thread 0x%x(%d) callstack: %s", currentId, tid, get_call_stack(buf, sizeof(buf), stack, extStack));
}

#if _TARGET_APPLE

static uint32_t g_stacks_processed = 0;

static void dump_current_thread(int type)
{
  dump_thread_callstack(get_current_thread_id());
  interlocked_add(g_stacks_processed, 1);
}

void dump_all_thread_callstacks()
{
  // debugger catches signals so it doesn't work
  if (is_debugger_present())
    return;

  signal(SIGINT, &dump_current_thread);

  thread_act_array_t threads = nullptr;
  mach_msg_type_number_t thread_count = 0, total_threads = 0;
  kern_return_t ret = task_threads(current_task(), &threads, &thread_count);
  if (ret != KERN_SUCCESS)
    return;

  g_stacks_processed = 0;
  total_threads = thread_count;
  for (size_t i = 0; i < thread_count; ++i)
  {
    pthread_t th = pthread_from_mach_thread_np(threads[i]);
    if (pthread_kill(th, SIGINT) != 0)
      total_threads--;
  }

  while (interlocked_acquire_load(g_stacks_processed) != total_threads)
    ;
}
#else
void dump_all_thread_callstacks() {}
#endif

#elif _TARGET_C3

#else
#include <string.h>
const char *stackhlp_get_call_stack(char *out_buf, int out_sz, const void *const * /*stack*/, unsigned /*max_size*/)
{
  if (out_sz >= 4)
  {
    memcpy(out_buf, "n/a", 4);
  }
  else if (out_sz > 0)
    *out_buf = 0;
  return out_buf;
}

void *stackhlp_get_bp() { return NULL; }

void dump_thread_callstack(intptr_t /*thread_id*/) {}
void dump_all_thread_callstacks() {}

#endif

#if _TARGET_PC_WIN

static BOOL CALLBACK enum_modules_cb(PCSTR module_name, MODULE_BASE_TYPE module_base, ULONG module_size, void *ctx)
{
  return (*(const enum_stack_modules_cb *)ctx)(module_name, module_base, module_size) ? TRUE : FALSE;
}

bool stackhlp_enum_modules(const enum_stack_modules_cb &cb)
{
  OSSpinlockScopedLock lock(dbghelp_spinlock);
  return EnumerateLoadedModules(GetCurrentProcess(), &enum_modules_cb, (void *)&cb);
}

#else
bool stackhlp_enum_modules(const enum_stack_modules_cb &) { return false; }
#endif


#if !HAS_GET_SYMBOL
bool stackhlp_get_symbol(void *, uint32_t &, char *filename, size_t, char *symbolname, size_t)
{
  filename[0] = symbolname[0] = 0;
  return false;
}
#endif

#if !(_TARGET_PC_WIN | XBOX_GDK_CALLSTACKS)
bool can_unwind_callstack_for_other_thread() { return false; }
int get_thread_handle_callstack(intptr_t, void **, uint32_t) { return 0; }
intptr_t sampling_open_thread(intptr_t) { return 0; }
void sampling_close_thread(intptr_t) {}
#endif

#if _TARGET_XBOX

void xbox_init_dbghelp()
{
#if XBOX_GDK_CALLSTACKS
  OSSpinlockScopedLock lock(dbghelp_spinlock);
  static bool initialized = false;
  if (initialized)
    return;
  initialized = true;
  if (SymInitialize(GetCurrentProcess(), ".", true))
    SymSetOptions(SYMOPT_DEFERRED_LOADS);
#endif
}
#endif
