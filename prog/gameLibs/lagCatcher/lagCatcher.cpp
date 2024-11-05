// Copyright (C) Gaijin Games KFT.  All rights reserved.

#ifndef TESTAPP
#include <lagCatcher/lagCatcher.h>
#endif

#if defined(__linux__) && !defined(ANDROID) && !defined(__ANDROID__)
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <execinfo.h>

#include "btCollector.h"
#include "utils.h"

#define PROFILE_SIGNAL           SIGPROF
#define MIN_SAMPLING_INTERVAL_US (25)

extern int g_in_backtrace; // set by stackhlp_fill_stack
namespace lagcatcher
{

struct ErrnoSaver
{
  ErrnoSaver() { e = errno; }
  ~ErrnoSaver() { errno = e; }
  int e;
};

static void sampling_signal_handler(int sig, siginfo_t *sinfo, void *ucontext);
static void deadline_signal_handler(int, siginfo_t *sinfo, void *);

struct Context
{
  Context(int mem_budget) : collector(mem_budget - sizeof(*this)), deadlineTimer(NULL), samplingTimer(NULL) {}

  bool init(int sampling_interval_us, int deadline_signal)
  {
    if (create_timer(PROFILE_SIGNAL, &samplingTimer, this) != 0)
      return false;
    if (create_timer(deadline_signal, &deadlineTimer, this) != 0)
    {
      destroy_timer(&samplingTimer);
      return false;
    }

    deadlineSignal = deadline_signal;
    samplingIntervalUs = sampling_interval_us > MIN_SAMPLING_INTERVAL_US ? sampling_interval_us : MIN_SAMPLING_INTERVAL_US;

    int sigs[] = {deadline_signal, PROFILE_SIGNAL, 0};
    block_signals(sigs, false, &oldSet);

    // on first call backtrace might call malloc/dload which of course not async-signal-safe, so do dummy
    // call here to make sure that glibc initialized all internal data structures
    void *dummy[16];
    (void)backtrace(dummy, sizeof(dummy) / sizeof(dummy[0]));

    enable_signal_handler(PROFILE_SIGNAL, &sampling_signal_handler);
    enable_signal_handler(deadline_signal, &deadline_signal_handler);

    return true;
  }

  ~Context()
  {
    stop();
    disable_signal_handler(deadlineSignal);
    disable_signal_handler(PROFILE_SIGNAL);
    if (deadlineTimer)
      pthread_sigmask(SIG_SETMASK, &oldSet, NULL);
    destroy_timer(&samplingTimer);
    destroy_timer(&deadlineTimer);
  }

  void start(int deadline_ms)
  {
    stop();
    collector.nextGen();
    start_timer(&deadlineTimer, deadline_ms * 1000, true);
  }

  void stop()
  {
    stop_timer(&deadlineTimer);
    stop_timer(&samplingTimer);
  }

  void clear()
  {
    stop();
    collector.clear();
  }

  template <int skip>
  void push_bt()
  {
    void *stack[BTCollector::MAX_STACK_DEPTH + skip];
    if (__atomic_load_n(&g_in_backtrace, __ATOMIC_RELAXED)) // can't use backtrace if it's already called (mutexes that used within are
                                                            // not signal safe)
      return;
    int size = backtrace(stack, sizeof(stack) / sizeof(stack[0]));
    collector.push(stack + skip, size - skip);
  }

  int flush(const char *log_path)
  {
    if (collector.isEmpty())
      return 0;
    stop();
    FILE *log = fopen(log_path, "w");
    if (!log)
      return false;
    int ret = collector.flush(log);
    fclose(log);
    return ret;
  }

  sigset_t oldSet;
  timer_t deadlineTimer, samplingTimer;
  int deadlineSignal;
  int samplingIntervalUs;

  BTCollector collector;
};

static Context *lagcatcher_ctx = NULL;

static void sampling_signal_handler(int, siginfo_t *sinfo, void *)
{
  if (sinfo->si_code != SI_TIMER)
    return;
  ErrnoSaver esave;
  ((Context *)sinfo->si_ptr)->push_bt<2>(); // skip sigaction & this signal handler
}

static void deadline_signal_handler(int, siginfo_t *sinfo, void *)
{
  if (sinfo->si_code != SI_TIMER)
    return;
  ErrnoSaver esave;
  Context *ctx = (Context *)sinfo->si_ptr;
  ctx->push_bt<2>();                                                // skip sigaction & this signal handler
  start_timer(&ctx->samplingTimer, ctx->samplingIntervalUs, false); // just enable sampling timer
}

void init_early(int deadline_signal)
{
  int sigs[] = {deadline_signal, PROFILE_SIGNAL, 0};
  block_signals(sigs, true);
}

bool init(int sampling_interval_us, int mem_budget, int deadline_signal)
{
  delete lagcatcher_ctx;
  lagcatcher_ctx = new Context(mem_budget);
  if (!lagcatcher_ctx->init(sampling_interval_us, deadline_signal))
  {
    delete lagcatcher_ctx;
    lagcatcher_ctx = NULL;
    return false;
  }
  return true;
}

void shutdown()
{
  delete lagcatcher_ctx;
  lagcatcher_ctx = NULL;
}

void start(int deadline_ms)
{
  if (lagcatcher_ctx)
    lagcatcher_ctx->start(deadline_ms);
}

void clear()
{
  if (lagcatcher_ctx)
    lagcatcher_ctx->clear();
}

int flush(const char *log_path) { return lagcatcher_ctx ? lagcatcher_ctx->flush(log_path) : false; }

void stop()
{
  if (lagcatcher_ctx)
    lagcatcher_ctx->stop();
}

}; // namespace lagcatcher

// compile as
// g++ -DTESTAPP -rdynamic -g lagCatcher.cpp -Wl,-lrt
#ifdef TESTAPP
#include <unistd.h>
static void sleep_sec(int sec)
{
  struct timespec req;
  req.tv_sec = sec;
  req.tv_nsec = 0;
  while (nanosleep(&req, &req) < 0 && errno == EINTR)
    ;
}

int main(int arc, char **argv)
{
  lagcatcher::init_early(SIGUSR2);
  lagcatcher::init(100000, 10 << 10, SIGUSR2);

  lagcatcher::start(30);
  sleep_sec(1);

  lagcatcher::start(30);
  sleep_sec(1);

  lagcatcher::flush("1.log");
  lagcatcher::shutdown();
  return 0;
}
#include "btCollector.cpp"
#include "utils.cpp"
#endif

#else // ifdef __linux__
// stubs for other platforms
namespace lagcatcher
{
void init_early(int) {}
bool init(int, int, int) { return false; }
void shutdown() {}
void start(int) {}
void stop() {}
void clear() {}
int flush(const char *) { return 0; }
}; // namespace lagcatcher
#endif
