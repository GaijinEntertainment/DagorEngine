#if !_TARGET_ANDROID
#error android platform functionality using with wrong platform
#endif
#pragma once

#include <signal.h>
#include <unistd.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_threads.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_logSys.h>
#include <debug/dag_debug.h>
#include <EASTL/utility.h>
#include <EASTL/optional.h>

static const int android_crash_signals[] = {SIGABRT, SIGTERM, SIGFPE, SIGSEGV, SIGILL, SIGBUS, SIGSTKFLT, SIGTRAP};

class AndroidCrashHandler
{
  class UnwinderThread final : public DaThread
  {
  public:
    UnwinderThread() : DaThread("UnwinderThread", DaThread::DEFAULT_STACK_SZ)
    {
      {
        ctx = SignalCtx{};
        capture_stack(); // just a blank shot
        ctx = eastl::nullopt;
      }
      os_event_create(&sleepEvent);
    }

    ~UnwinderThread() override
    {
      terminate(true, -1, &sleepEvent);
      os_event_destroy(&sleepEvent); // -V779
    }

    void execute() override
    {
      while (!isThreadTerminating())
      {
        if (os_event_wait(&sleepEvent, OS_WAIT_INFINITE) != OS_WAIT_OK)
          continue;

        process_signal();
      }
    }

    void set_context(int sig, siginfo_t *info)
    {
      ctx = SignalCtx{.signal = sig, .sigNo = info->si_signo, .sigCode = info->si_code, .sigAddr = info->si_addr};
      capture_stack();
      os_event_set(&sleepEvent);
    }

  private:
    struct SignalCtx
    {
      int signal;
      int sigNo;
      int sigCode;
      void *sigAddr;

      stackhelp::CallStackCaptureStore<32> stack;
      stackhelp::ext::CallStackCaptureStore extStack;
    };
    eastl::optional<SignalCtx> ctx;
    os_event_t sleepEvent;

    void capture_stack()
    {
      ctx->stack.capture();
      ctx->extStack.capture();
    }

    void print_signal_details()
    {
      char codeBuffer[256] = {0};
      switch (ctx->sigNo)
      {
        case SIGILL:
          _snprintf(codeBuffer, sizeof(codeBuffer), "Incorrect instruction at %p - ", ctx->sigAddr);
          switch (ctx->sigCode)
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
          switch (ctx->sigCode)
          {
            case BUS_ADRALN: strcat(codeBuffer, "BUS_ADRALN (Invalid address alignment)"); break;
            case BUS_ADRERR: strcat(codeBuffer, "BUS_ADRERR (Nonexistent physical address)"); break;
            case BUS_OBJERR: strcat(codeBuffer, "BUS_OBJERR (Object-specific hardware error)"); break;
            default: strcat(codeBuffer, "unknown"); break;
          }
          break;
        case SIGFPE:
          switch (ctx->sigCode)
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
          _snprintf(codeBuffer, sizeof(codeBuffer), "Incorrect access to %p - ", ctx->sigAddr);
          switch (ctx->sigCode)
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

      debug("signal %u (SIG%s - %s), details: code %d (%s)", ctx->signal, sys_signame[ctx->sigNo], sys_siglist[ctx->sigNo],
        ctx->sigCode, codeBuffer);
    }

    void process_signal()
    {
      if (!ctx)
        return;

      print_signal_details();

      char stackInfo[8192];
      get_call_stack(stackInfo, sizeof(stackInfo), ctx->stack, ctx->extStack);
      debug("signal %u (SIG%s - %s)!\n %s \n end of captured stack", ctx->signal, sys_signame[ctx->sigNo], sys_siglist[ctx->sigNo],
        stackInfo);
      ctx = eastl::nullopt;
    }
  };

  static UnwinderThread *unwinder;
  static bool is_unwinder_inited() { return interlocked_acquire_load_ptr(unwinder); }

  static void unwinder_init()
  {
    UnwinderThread *thread = new UnwinderThread();
    thread->start();
    interlocked_release_store_ptr(unwinder, thread);
  }

  static void unwinder_shutdown()
  {
    if (auto *thread = interlocked_exchange_ptr(unwinder, (UnwinderThread *)nullptr))
    {
      del_it(thread);
    }
  }

  static bool alreadyTriggered;

  typedef struct sigaction SigActionStruct;
  static SigActionStruct old_signal_handlers[NSIG];

  bool signalRedirected[NSIG] = {0};

  static void signal_handler(int signal, siginfo_t *info, void *ucontext)
  {
    g_debug_is_in_fatal = true;

    // avoid nested loop if follow up code generates another signal for some reason
    if (alreadyTriggered)
      return;
    alreadyTriggered = true;

    if (dgs_pre_shutdown_handler)
      dgs_pre_shutdown_handler();

    auto *thread = interlocked_acquire_load_ptr(unwinder);
    G_ASSERT(thread);
    thread->set_context(signal, info);

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
    if (is_unwinder_inited())
      return;

    unwinder_init();

    SigActionStruct handlerSigAct;
    memset(&handlerSigAct, 0, sizeof(handlerSigAct));
    handlerSigAct.sa_flags = SA_SIGINFO;
    handlerSigAct.sa_sigaction = &signal_handler;

    for (int i : android_crash_signals)
      registerSignalHandler(i, handlerSigAct);
  }

  void clear()
  {
    if (!is_unwinder_inited())
      return;

    for (int i : android_crash_signals)
      unregisterSignalHandler(i);

    unwinder_shutdown();
  }
};
AndroidCrashHandler::UnwinderThread *AndroidCrashHandler::unwinder = nullptr;
AndroidCrashHandler::SigActionStruct AndroidCrashHandler::old_signal_handlers[NSIG] = {};
bool AndroidCrashHandler::alreadyTriggered = false;