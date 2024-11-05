//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_C1 | _TARGET_C2 | _TARGET_C3
#include <pthread.h>
#elif _TARGET_PC_WIN | _TARGET_XBOX
#include <util/dag_stdint.h>
#endif
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_atomic.h>
#include <EASTL/utility.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <debug/dag_assert.h>
#include <debug/dag_log.h>
#include <supp/dag_define_KRNLIMP.h>

#define WORKER_THREADS_AFFINITY_MASK (~0ull) // magic reserved word

#if _TARGET_XBOX | _TARGET_C3
#define MAIN_THREAD_AFFINITY        1ull
#define WORKER_THREADS_AFFINITY_USE (~1ull)
#define NUM_WORKERS_DEFAULT         (cpujobs::get_core_count() - 1)
#elif _TARGET_C1



#elif _TARGET_C2



#elif _TARGET_ANDROID
#define MAIN_THREAD_AFFINITY        (1ull << (cpujobs::get_core_count() - 1)) // Fast cores have higher numbers.
// Reserve two cores for main and Vulkan worker.
#define WORKER_THREADS_AFFINITY_USE (cpujobs::get_core_count() >= 4 ? (1ull << (cpujobs::get_core_count() - 2)) - 1 : ~0ull)
#define NUM_WORKERS_DEFAULT         (max(1, cpujobs::get_core_count() - 2))
#else
#define MAIN_THREAD_AFFINITY \
  (cpujobs::get_core_count() >= 3 ? 0x4ull : 0x1ull) // Core2 - Not the Core0, favorite core of different tools,
// not the Core1 that is affected by HT from Core0,
// and not the last core that Nvidia likes.
#define RESERVED_FOR_MAIN_HT \
  (cpujobs::get_core_count() >= 8 ? 0xCull : MAIN_THREAD_AFFINITY) // Don't reserve an additional thread on 2/4 or 4/4 CPUs,
// and there are no 3/6 CPUs, so don't reserve on 6/6 either.
#define WORKER_THREADS_AFFINITY_USE \
  (cpujobs::get_core_count() >= 3 ? ~RESERVED_FOR_MAIN_HT : ~0ull) // All but reserved for the main thread.
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
//         if (isThreadTerminating())
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
public:
  volatile int terminating = 0;

private:
#if _TARGET_PC_LINUX | _TARGET_ANDROID
  static constexpr int MAX_NAME_LEN = 15;
#else
  static constexpr int MAX_NAME_LEN = 31;
#endif

  volatile int threadState;
  int priority;
  uint64_t affinityMask = 0;
  char name[MAX_NAME_LEN + 1];
  DaThread *nextThread;
  size_t stackSize;

#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | _TARGET_C1 | _TARGET_C2 | _TARGET_C3
  pthread_t id;
  KRNLIMP static void *threadEntry(void *arg);

#elif _TARGET_PC_WIN | _TARGET_XBOX
  uintptr_t id;
  bool minidumpSaveStack = true;
  KRNLIMP static unsigned __stdcall threadEntry(void *arg);

#else
  !error: Unsupported platform
#endif

public:
  static constexpr int DEFAULT_STACK_SZ = 64 << 10;

  KRNLIMP DaThread(const char *threadName = NULL, size_t stack_size = DEFAULT_STACK_SZ, int priority = 0, uint64_t affinity = 0);
  DaThread(DaThread &&) = default;

  KRNLIMP bool start();
  KRNLIMP void terminate(bool wait, int timeout_ms = -1, os_event_t *wake_event = NULL);
  virtual void execute() = 0;

  KRNLIMP const void *getCurrentThreadIdPtr() const { return &id; }
  KRNLIMP void setThreadIdealProcessor(int ideal_processor_no);
  KRNLIMP void stripStackInMinidump();

  KRNLIMP static void terminate_all(bool wait, int timeout_ms = 3000);

  //! explicit destructor
  void destroy() { delete this; }

  KRNLIMP bool isThreadStarted() const;
  KRNLIMP bool isThreadRunnning() const;
  bool isThreadTerminating() const;

  KRNLIMP static const char *getCurrentThreadName();
  KRNLIMP static bool applyThisThreadPriority(int prio, const char *name = nullptr);
  KRNLIMP static void applyThisThreadAffinity(uint64_t affinity);
#if _TARGET_PC | _TARGET_XBOX
  KRNLIMP static bool isDaThreadWinUnsafe(uintptr_t thread_id, bool &minidump_save_stack); // no lock
#endif
  KRNLIMP static void setCurrentThreadName(const char *tname);

protected:
  KRNLIMP virtual ~DaThread();

  void applyThreadPriority();
  void afterThreadExecution();
  void doThread();

  friend void push_thread_to_list(DaThread *);
  friend bool remove_thread_from_list(DaThread *);
  friend void set_current_cpujobs_thread_name(const char *);
};

inline bool DaThread::isThreadTerminating() const { return interlocked_relaxed_load(terminating); }

template <typename F>
inline void execute_in_new_thread(F &&f, const char *thread_name = nullptr, int stk_sz = DaThread::DEFAULT_STACK_SZ, int prio = 0,
  uint64_t affinity = 0)
{
  class BgThread final : public DaThread
  {
  public:
    BgThread(F &&f, const char *thread_name, int stk_sz, int prio, uint64_t aff) :
      DaThread(thread_name, stk_sz, prio, aff ? aff : WORKER_THREADS_AFFINITY_MASK), function(eastl::move(f))
    {}

    void execute() override
    {
      auto is_terminating = [this]() -> bool { return this->isThreadTerminating(); };
      function(is_terminating);
      delete this; // DaThread's implementation allows instance deletion within it's method
    }

  private:
    F function;
  };
  auto thr = new BgThread(eastl::move(f), thread_name, stk_sz, prio, affinity);
  G_ASSERT_DO_AND_LOG(thr->start(), delete thr, "Failed to start thread <%s>", thread_name);
}

#include <supp/dag_undef_KRNLIMP.h>
