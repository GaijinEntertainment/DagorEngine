#include <stdio.h>
#include <stdarg.h>
#include <windows.h>
#include <eh.h>
#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <util/dag_stdint.h>
#include <util/limBufWriter.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/winHwExceptUtils.h>
#include <float.h>
#include <time.h>
#include "../kernel/debugPrivate.h"
#include <osApiWrappers/basePath.h>


const char *WinHwExceptionUtils::getExceptionTypeString(uint32_t code)
{
  switch (code)
  {
    case EXCEPTION_ACCESS_VIOLATION: return "access violation";

    case EXCEPTION_DATATYPE_MISALIGNMENT: return "misaligned data access";

    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "out of bounds access";

    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "floating point /0";

    case EXCEPTION_FLT_INVALID_OPERATION: return "floating point error";

    case EXCEPTION_FLT_OVERFLOW: return "floating point overflow";

    case EXCEPTION_FLT_STACK_CHECK: return "floating point stack over/underflow";

    case EXCEPTION_FLT_UNDERFLOW: return "floating point underflow";

    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer /0";

    case EXCEPTION_INT_OVERFLOW: return "integer overflow";

    case EXCEPTION_PRIV_INSTRUCTION: return "priv instruction";

    case EXCEPTION_IN_PAGE_ERROR: return "page is inaccessible";

    case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal opcode";

    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "attempt to continue after noncontinuable exception";

    case EXCEPTION_STACK_OVERFLOW: return "stack overflow";

    case EXCEPTION_INVALID_DISPOSITION: return "invalid disposition";

    case EXCEPTION_INVALID_HANDLE: return "invalid handle";

    case STATUS_FLOAT_MULTIPLE_TRAPS: return "float trap";

    case STATUS_FLOAT_MULTIPLE_FAULTS: return "float faults";

    default: return "*unknown type*";
  }
}


bool WinHwExceptionUtils::checkException(uint32_t code)
{
  switch (code)
  {
#if DAGOR_DBGLEVEL == 0
    case STATUS_ASSERTION_FAILURE:
#endif
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_GUARD_PAGE: return false;

    case EXCEPTION_SINGLE_STEP:
    case EXCEPTION_BREAKPOINT:
#if DAGOR_DBGLEVEL > 0
      return !::IsDebuggerPresent();
#else
      // fallthrough
#endif
    default: return true;
  }
}


void WinHwExceptionUtils::parseExceptionInfo(EXCEPTION_POINTERS *eptr, const char *title, char *text_buffer, int text_buffer_size,
  const char **call_stack_ptr)
{
  G_ASSERT(eptr);
  G_ASSERT(eptr->ExceptionRecord);


  LimitedBufferWriter lbw(text_buffer, text_buffer_size);

  EXCEPTION_RECORD &rec = *eptr->ExceptionRecord;

  lbw.aprintf("%s\n\nException details: %X at %08llX", title, rec.ExceptionCode, (int64_t)rec.ExceptionAddress);


  const char *name = getExceptionTypeString(rec.ExceptionCode);

  lbw.aprintf(": %s\n", name);

  if (EXCEPTION_ACCESS_VIOLATION == rec.ExceptionCode)
  {
    lbw.aprintf("  attempt to %s memory at %08llX\n", rec.ExceptionInformation[0] ? "write to" : "read",
      (int64_t)rec.ExceptionInformation[1]);
  }

  lbw.aprintf("Exception record: .exr %p (for full dump)\n\n", eptr->ExceptionRecord);

  // Clear FP status as otherwise FP-related code (e.g. within message box) can't work properly
  if (rec.ExceptionCode == EXCEPTION_FLT_DIVIDE_BY_ZERO || rec.ExceptionCode == EXCEPTION_FLT_INVALID_OPERATION ||
      rec.ExceptionCode == EXCEPTION_FLT_OVERFLOW || rec.ExceptionCode == EXCEPTION_FLT_STACK_CHECK ||
      rec.ExceptionCode == EXCEPTION_FLT_UNDERFLOW)
  {
    _clearfp();
  }

  CONTEXT &ctx = *eptr->ContextRecord;
#if !_TARGET_64BIT
  lbw.aprintf("   CS:%8X EIP:%8X  SS:%8X ESP:%8X EBP:%8X\n", ctx.SegCs, ctx.Eip, ctx.SegSs, ctx.Esp, ctx.Ebp);
  lbw.aprintf("  EAX:%8X EBX:%8X ECX:%8X EDX:%8X ESI:%8X EDI:%8X\n", ctx.Eax, ctx.Ebx, ctx.Ecx, ctx.Edx, ctx.Esi, ctx.Edi);
  lbw.aprintf("   DS:%8X  ES:%8X  FS:%8X  GS:%8X   EFLAGS:%08X\n", ctx.SegDs, ctx.SegEs, ctx.SegFs, ctx.SegGs, ctx.EFlags);
#else
  lbw.aprintf("  RIP:%16llX RSP:%16llX RBP:%16llX\n", ctx.Rip, ctx.Rsp, ctx.Rbp);
  lbw.aprintf("  RAX:%16llX RBX:%16llX RCX:%16llX\n  RDX:%16llX RSI:%16llX RDI:%16llX\n  EFLAGS:%08X\n", ctx.Rax, ctx.Rbx, ctx.Rcx,
    ctx.Rdx, ctx.Rsi, ctx.Rdi, ctx.EFlags);
#endif
  lbw.done();

#if DAGOR_FORCE_LOGS || DAGOR_DBGLEVEL > 0
  // immediately report exception (to debug)
  debug_set_log_callback(nullptr); // if we get here, process already in undetermined state, don't do unknown side-effects even more
  logmessage(LOGLEVEL_FATAL, "%s", text_buffer);
  flush_debug_file();
#endif // DAGOR_FORCE_LOGS || DAGOR_DBGLEVEL > 0

#if DAGOR_DBGLEVEL > 0
  // get callstack and dump it to debug
  void *excStack[32];

  ::stackhlp_fill_stack_exact(excStack, 32, &ctx);

  static char stack_info[8192] = {0};
  ::stackhlp_get_call_stack(stack_info, sizeof(stack_info), excStack, 32);
  if (NULL != call_stack_ptr)
    *call_stack_ptr = stack_info;

  logmessage(LOGLEVEL_FATAL, "%s", stack_info);

  flush_debug_file();

#else
  lbw.reset(text_buffer, text_buffer_size);
  lbw.append(title);
  lbw.done();

  if (NULL != call_stack_ptr)
    *call_stack_ptr = "";
#endif
}

// Needed for compatibility with _launcher (or whoever else uses it).
// Should be removed later: it has never been platform-dependent.
void WinHwExceptionUtils::setupCrashDumpFileName()
{
#if !_TARGET_XBOX
  debug_internal::setupCrashDumpFileName();
#endif
}
