//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_C1 | _TARGET_C2 | _TARGET_C3
#include <pthread.h>
#elif _TARGET_PC_WIN | _TARGET_XBOX
#include <util/dag_stdint.h>
#elif _TARGET_ANDROID
#include <pthread.h>
#include <osApiWrappers/dag_spinlock.h>
#endif
#include <osApiWrappers/dag_events.h>
#include <EASTL/utility.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <debug/dag_assert.h>
#include <supp/dag_define_COREIMP.h>

#if _TARGET_XBOX | _TARGET_C3
#define MAIN_THREAD_AFFINITY         1
#define WORKER_THREADS_AFFINITY_MASK (~1u)
#define NUM_WORKERS_DEFAULT          (cpujobs::get_core_count() - 1)
#elif _TARGET_C1



#elif _TARGET_C2



#else
#define MAIN_THREAD_AFFINITY \
  (cpujobs::get_core_count() >= 3 ? 0x4 : 0x1) // Core2 - Not the Core0, favorite core of different tools,
                                               // not the Core1 that is affected by HT from Core0,
                                               // and not the last core that Nvidia likes.
#define RESERVED_FOR_MAIN_HT \
  (cpujobs::get_core_count() >= 8 ? 0xC : MAIN_THREAD_AFFINITY) // Don't reserve an additional thread on 2/4 or 4/4 CPUs,
                                                                // and there are no 3/6 CPUs, so don't reserve on 6/6 either.
#define WORKER_THREADS_AFFINITY_MASK \
  (cpujobs::get_core_count() >= 3 ? ~RESERVED_FOR_MAIN_HT : ~0u) // All but reserved for the main thread.
#define NUM_WORKERS_DEFAULT                                            \
  min(cpujobs::get_core_count() >= 8   ? cpujobs::get_core_count() - 3 \
      : cpujobs::get_core_count() >= 6 ? cpujobs::get_core_count() - 2 \
      : cpujobs::get_core_count() >= 2 ? cpujobs::get_core_count() - 1 \
                                       : 1,                            \
    8)
#endif

// Usage example:
//
//   ...
//   class MyThread : public DaThread
//   {
//     void execute()
//     {
//       for (;;)
//       {
//         // do something
//         //
//         if (terminating)
//           break;
//       }
//     }
//   } myThread;
//
//   myThread.start();
//   ...
//

class DaThread
{
private:
#if _TARGET_PC_LINUX | _TARGET_ANDROID
  static constexpr int MAX_NAME_LEN = 15;
#else
  static constexpr int MAX_NAME_LEN = 31;
#endif

  volatile int threadState;
  size_t stackSize;
  int priority;
  char name[MAX_NAME_LEN + 1];
  DaThread *nextThread;

#if _TARGET_ANDROID
  int tid = -1;
  int affinityMask = 0;
  OSSpinlock mutex;
#endif

#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | _TARGET_C1 | _TARGET_C2 | _TARGET_C3
  pthread_t id;
  KRNLIMP static void *threadEntry(void *arg);

#elif _TARGET_PC | _TARGET_XBOX
  uintptr_t id;
  KRNLIMP static unsigned __stdcall threadEntry(void *arg);

#else
  !error: Unsupported platform
#endif

public:
  static constexpr int DEFAULT_STACK_SZ = 64 << 10;
  volatile int terminating;

  KRNLIMP DaThread(const char *threadName = NULL, size_t stack_size = DEFAULT_STACK_SZ, int priority = 0);

  KRNLIMP bool start();
  KRNLIMP void terminate(bool wait, int timeout_ms = -1, os_event_t *wake_event = NULL);
  virtual void execute() = 0;

  KRNLIMP const void *getCurrentThreadIdPtr() const { return &id; }
  KRNLIMP void setAffinity(unsigned int affinity); // TODO: make affinity ctor arg and remove this method
  KRNLIMP void setThreadIdealProcessor(int ideal_processor_no);

  KRNLIMP static void terminate_all(bool wait, int timeout_ms = 3000);

  //! explicit destructor
  void destroy() { delete this; }

  KRNLIMP bool isThreadStarted() const;
  KRNLIMP bool isThreadRunnning() const;

  KRNLIMP static const char *getCurrentThreadName();
  KRNLIMP static bool applyThisThreadPriority(int prio, const char *name = nullptr);

protected:
  KRNLIMP virtual ~DaThread();

  void applyThreadPriority();
  static void setCurrentThreadName(const char *tname);
  void afterThreadExecution();
  void applyThreadAffinity(unsigned int affinity);
  void doThread();

  friend void push_thread_to_list(DaThread *);
  friend bool remove_thread_from_list(DaThread *);
  friend void set_current_cpujobs_thread_name(const char *);
};

template <typename F>
inline void execute_in_new_thread(F &&f, const char *thread_name = nullptr, int stk_sz = DaThread::DEFAULT_STACK_SZ, int prio = 0,
  unsigned affinity = 0)
{
  class BgThread final : public DaThread
  {
  public:
    BgThread(F &&f, const char *thread_name, int stk_sz, int prio, unsigned aff) :
      DaThread(thread_name, stk_sz, prio), function(eastl::move(f)), affinity(aff)
    {}

    void execute() override
    {
      this->setAffinity(affinity ? affinity : WORKER_THREADS_AFFINITY_MASK);
      auto is_terminating = [this]() -> bool { return this->terminating; };
      function(is_terminating);
      delete this; // DaThread's implementation allows instance deletion within it's method
    }

  private:
    F function;
    unsigned affinity;
  };
  G_VERIFY((new BgThread(eastl::move(f), thread_name, stk_sz, prio, affinity))->start());
}

#include <supp/dag_undef_COREIMP.h>
