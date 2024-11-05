#if !_TARGET_ANDROID
#error android platform functionality using with wrong platform
#endif
#pragma once

#include <signal.h>
#include <unistd.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <EASTL/utility.h>

static const int android_crash_signals[] = {SIGABRT, SIGTERM, SIGFPE, SIGSEGV, SIGILL, SIGBUS, SIGSTKFLT, SIGTRAP};

class AndroidCrashHandler
{
  bool settedUp = false;
  static bool alreadyTriggered;

  typedef struct sigaction SigActionStruct;
  static SigActionStruct old_signal_handlers[NSIG];

  bool signalRedirected[NSIG] = {0};

  static void signal_get_details(char *descBuffer, int bufferSize, siginfo_t *info)
  {
    char codeBuffer[256] = {0};
    switch (info->si_signo)
    {
      case SIGILL:
        _snprintf(codeBuffer, sizeof(codeBuffer), "Incorrect instruction at %p - ", info->si_addr);
        switch (info->si_code)
        {
          case ILL_ILLOPC: strcat(codeBuffer, "ILL_ILLOPC (Illegal opcode)"); break;
          case ILL_ILLOPN: strcat(codeBuffer, "ILL_ILLOPN (Illegal operand)"); break;
          case ILL_ILLADR: strcat(codeBuffer, "ILL_ILLADR (Illegal addressing mode)"); break;
          case ILL_ILLTRP: strcat(codeBuffer, "ILL_ILLTRP (Illegal trap)"); break;
          case ILL_PRVOPC: strcat(codeBuffer, "ILL_PRVOPC (Privileged opcode)"); break;
          case ILL_PRVREG: strcat(codeBuffer, "ILL_PRVREG (Privileged register.)"); break;
          case ILL_COPROC: strcat(codeBuffer, "ILL_COPROC (Coprocessor error)"); break;
          case ILL_BADSTK: strcat(codeBuffer, "ILL_BADSTK (Internal stack error)"); break;
          default: strcat(codeBuffer, "unknown"); break;
        }
        break;
      case SIGBUS:
        switch (info->si_code)
        {
          case BUS_ADRALN: strcat(codeBuffer, "BUS_ADRALN (Invalid address alignment)"); break;
          case BUS_ADRERR: strcat(codeBuffer, "BUS_ADRERR (Nonexistent physical address)"); break;
          case BUS_OBJERR: strcat(codeBuffer, "BUS_OBJERR (Object-specific hardware error)"); break;
          default: strcat(codeBuffer, "unknown"); break;
        }
        break;
      case SIGFPE:
        switch (info->si_code)
        {
          case FPE_INTDIV: strcat(codeBuffer, "FPE_INTDIV (Integer divide by zero)"); break;
          case FPE_INTOVF: strcat(codeBuffer, "FPE_INTOVF (Integer overflow)"); break;
          case FPE_FLTDIV: strcat(codeBuffer, "FPE_FLTDIV (Floating-point divide by zero)"); break;
          case FPE_FLTOVF: strcat(codeBuffer, "FPE_FLTOVF (Floating-point overflow)"); break;
          case FPE_FLTUND: strcat(codeBuffer, "FPE_FLTUND (Floating-point underflow)"); break;
          case FPE_FLTRES: strcat(codeBuffer, "FPE_FLTRES (Floating-point inexact result)"); break;
          case FPE_FLTINV: strcat(codeBuffer, "FPE_FLTINV (Invalid floating-point operation)"); break;
          case FPE_FLTSUB: strcat(codeBuffer, "FPE_FLTSUB (Subscript out of range)"); break;
          default: strcat(codeBuffer, "unknown"); break;
        }
        break;
      case SIGSEGV:
        _snprintf(codeBuffer, sizeof(codeBuffer), "Incorrect access to %p - ", info->si_addr);
        switch (info->si_code)
        {
          case SEGV_MAPERR: strcat(codeBuffer, "SEGV_MAPERR (Address not mapped to object)"); break;
          case SEGV_ACCERR: strcat(codeBuffer, "SEGV_ACCERR (Invalid permissions for mapped object)"); break;
          default: strcat(codeBuffer, "unknown"); break;
        }
        break;
      default: break;
    }

    if (!codeBuffer[0])
    {
      strcat(codeBuffer, "n/a");
    }
    _snprintf(descBuffer, bufferSize, "code %d (%s)", info->si_code, codeBuffer);
  }

  static void signal_handler(int signal, siginfo_t *info, void *ucontext)
  {
    g_debug_is_in_fatal = true;

    // avoid nested loop if follow up code generates another signal for some reason
    if (alreadyTriggered)
      return;
    alreadyTriggered = true;

    if (dgs_pre_shutdown_handler)
      dgs_pre_shutdown_handler();

    // dump stack to debug log
    // instead of reporting message box to user,
    // because it somehow does not work good on android
    {
      char detailsBuf[512];
      signal_get_details(detailsBuf, sizeof(detailsBuf), info);

      stackhelp::CallStackCaptureStore<32> stack;
      stackhelp::ext::CallStackCaptureStore extStack;
      stack.capture();
      extStack.capture();

      char stackInfo[8192];
      get_call_stack(stackInfo, sizeof(stackInfo), stack, extStack);
      debug("signal %u (SIG%s - %s), details: %s !\n %s", signal, sys_signame[info->si_signo], sys_siglist[info->si_signo], detailsBuf,
        stackInfo);
    }

    // call old signal handler, to properly generate system based crashdumps
    // or other stuff if it was configured
    old_signal_handlers[signal].sa_sigaction(signal, info, ucontext);

    g_debug_is_in_fatal = false;
  }

  void registerSignalHandler(int signal, struct sigaction &dsc)
  {
    signalRedirected[signal] = sigaction(signal, &dsc, &old_signal_handlers[signal]) == 0;
  }

  void unregisterSignalHandler(int signal)
  {
    if (!signalRedirected[signal])
      return;

    sigaction(signal, &old_signal_handlers[signal], nullptr);
  }

public:
  void setup()
  {
    if (settedUp)
      return;

    SigActionStruct handlerSigAct;
    memset(&handlerSigAct, 0, sizeof(handlerSigAct));
    handlerSigAct.sa_flags = SA_SIGINFO;
    handlerSigAct.sa_sigaction = &signal_handler;

    for (int i : android_crash_signals)
      registerSignalHandler(i, handlerSigAct);

    settedUp = true;
  }

  void clear()
  {
    if (!settedUp)
      return;

    for (int i : android_crash_signals)
      unregisterSignalHandler(i);

    settedUp = false;
  }
};

AndroidCrashHandler::SigActionStruct AndroidCrashHandler::old_signal_handlers[NSIG] = {};
bool AndroidCrashHandler::alreadyTriggered = false;