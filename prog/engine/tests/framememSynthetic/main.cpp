// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_mainCon.inc.cpp>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_log.h>

#include <memory/dag_framemem.h>
#include <math/random/dag_random.h>

#include <windows.h>


template <class T>
inline __forceinline void do_not_optimize(T &value)
{
  asm volatile("" : "+r,m"(value) : : "memory");
}

int DagorWinMain(bool /*debugmode*/)
{
  ::symhlp_init_default();
  set_debug_console_handle((intptr_t)::GetStdHandle(STD_OUTPUT_HANDLE));

  ::SetThreadAffinityMask(::GetCurrentThread(), 1);
  ::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);

  logdbg("Running synthetic test...");

  auto framemem = framemem_ptr();

  const auto avg = [](const auto &data) {
    double result = 0;
    for (auto i : data)
      result += i;
    result /= data.size();
    return result;
  };

  const auto err = [&avg](const auto &data) {
    const double mean = avg(data);
    double var = 0;
    for (auto i : data)
      var += (i - mean) * (i - mean);
    var /= data.size();
    return 100 * sqrt(var) / mean;
  };

  static constexpr int AVERAGE_OVER = 20000;

  {
    static constexpr int TEST_SIZE = 1 << 14; // will fit in default 1mb framemem

    dag::Vector<uint64_t> allocTimes;
    allocTimes.reserve(AVERAGE_OVER);
    dag::Vector<uint64_t> freeTimes;
    freeTimes.reserve(AVERAGE_OVER);

    for (int iter = 0; iter < AVERAGE_OVER; ++iter)
    {
      FRAMEMEM_REGION;

      dag::Vector<void *> alloced(TEST_SIZE);

      {
        const auto start = profile_ref_ticks();
        for (int i = 0; i < TEST_SIZE; ++i)
          alloced[i] = framemem->alloc(16);
        allocTimes.push_back(profile_ref_ticks() - start);
      }

      {
        const auto start = profile_ref_ticks();
        for (int i = TEST_SIZE - 1; i >= 0; --i)
          framemem->free(alloced[i]);
        freeTimes.push_back(profile_ref_ticks() - start);
      }
    }

    logdbg("%d allocs take: %f +- %f%%", TEST_SIZE, avg(allocTimes), err(allocTimes));
    logdbg("%d frees take: %f +- %f%%", TEST_SIZE, avg(freeTimes), err(freeTimes));
  }

  {
    static constexpr int TEST_SIZE = 1 << 14; // will fit in default 1mb framemem

    dag::Vector<uint64_t> times;
    times.reserve(AVERAGE_OVER);

    for (int iter = 0; iter < AVERAGE_OVER; ++iter)
    {
      FRAMEMEM_REGION;

      dag::Vector<void *> alloced(TEST_SIZE);

      static constexpr char BYTES[16]{0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xAD, 0xF0, 0x0D, 0xCA, 0xFE, 0xBA, 0xBE, 0xFA, 0xCE, 0xFA, 0x11};

      {
        const auto start = profile_ref_ticks();
        for (int i = 0; i < TEST_SIZE; ++i)
        {
          alloced[i] = framemem->alloc(16);
          memcpy(alloced[i], BYTES, 16);
          do_not_optimize(alloced[i]);
        }
        times.push_back(profile_ref_ticks() - start);
      }
    }

    logdbg("%d alloc/write pairs take: %f +- %f%%", TEST_SIZE, avg(times), err(times));
  }

  {
    static constexpr int TEST_SIZE = 1 << 14;


    dag::Vector<uint64_t> times;
    times.reserve(AVERAGE_OVER);

    for (int iter = 0; iter < AVERAGE_OVER; ++iter)
    {
      FRAMEMEM_REGION;

      const auto start = profile_ref_ticks();
      for (int i = 0; i < TEST_SIZE; ++i)
      {
        auto p = framemem->alloc(42);
        do_not_optimize(p);
        framemem->free(p);
      }
      times.push_back(profile_ref_ticks() - start);
    }
    logdbg("%d alloc/free pairs take: %f +- %f%%", TEST_SIZE, avg(times), err(times));
  }

  {
    static constexpr int TEST_SIZE = 1 << 14; // fit into 1mb


    dag::Vector<uint64_t> times;
    times.reserve(AVERAGE_OVER);

    for (int iter = 0; iter < AVERAGE_OVER; ++iter)
    {
      FRAMEMEM_REGION;

      void *p = framemem->alloc(16);

      const auto start = profile_ref_ticks();
      for (int i = 0; i < TEST_SIZE; ++i)
      {
        auto res = framemem->resizeInplace(p, 16 * i);
        G_FAST_ASSERT(res);
        do_not_optimize(res);
      }
      times.push_back(profile_ref_ticks() - start);

      framemem->free(p);
    }


    logdbg("%d inplace resizes take: %f +- %f%%", TEST_SIZE, avg(times), err(times));
  }

  logdbg("Done.");

  return 0;
}
