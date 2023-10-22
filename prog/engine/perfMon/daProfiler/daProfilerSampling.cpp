#include <dag_noise/dag_uint_noise.h>
#include "daProfilerInternal.h"
#include "daProfilePlatform.h"
#include "daProfilerSamplingUtils.h"
#include "stl/daProfilerAlgorithm.h"
#include <osApiWrappers/dag_threads.h> //execute in new thread. we can just use stl threads
#include <osApiWrappers/dag_ttas_spinlock.h>

namespace da_profiler
{

struct ThreadStackUnwinder;
struct ThreadStackUnwindProvider;
int unwind_thread_stack(ThreadStackUnwindProvider &p, ThreadStackUnwinder &s, uint64_t *addresses, size_t max_size);
void stop_unwind_thread_stack(ThreadStackUnwinder *s);
ThreadStackUnwinder *start_unwind_thread_stack(ThreadStackUnwindProvider &, intptr_t thread_id);

ThreadStackUnwindProvider *start_unwind(size_t stack_size = 2 << 20);
void stop_unwind(ThreadStackUnwindProvider *);

struct SamplingThread
{
  struct StopUnwinder
  {
    void operator()(ThreadStackUnwinder *s) { stop_unwind_thread_stack(s); }
  };
  unique_ptr<ThreadStackUnwinder, StopUnwinder> unwinder;
  uint64_t tid = 0;
  uint32_t gen = 0;
  uint16_t ii = 0, lastStackSize = 0;

  uint16_t lastRevStack[3 * ProfilerData::MAX_STACK_SIZE];                                 // -V730
  SamplingThread() = default;                                                              // -V730
  SamplingThread(uint64_t tid_, ThreadStackUnwinder &un, uint32_t gen_, uint32_t i_ = 1) : // -V730
    tid(tid_), unwinder(&un), gen(gen_), ii(i_)                                            // -V730
  {}
};

static inline int64_t ticks_to_usec(int64_t ticks, int64_t freq) { return ticks * 1000000LL / freq; }
static inline int64_t usec_to_ticks(int64_t usec, int64_t freq) { return usec * freq / 1000000LL; }

static inline bool usec_passed(int64_t ticks, int64_t usec, int64_t freq) { return ticks >= usec_to_ticks(usec, freq); }

static inline void sleep_usec_safe(uint32_t cores, int64_t usec)
{
  static constexpr uint32_t MIN_CORES_TO_CONSTANT_PROFILE = 4;
  // if we have less cores we would always call sleep_usec(1msec+) at end of sampling
  // Sleeping for less than 1msec is dangerous, as we can starve all other threads on a current core, and we constantly suspend other
  // thread. as heurestics we assume that if we have less than 5 cores we always sleep for at least 1msec also, not sleep for more than
  // 100msec otherwise changing state is very long (spinlock/mutex)
  usec = clamp((long long int)usec, (cores <= MIN_CORES_TO_CONSTANT_PROFILE) ? 1001ll : 0ll, 100000LL);
  sleep_usec(usec);
}

static uint16_t compare_and_update_stack(uint16_t new_sz, const uint64_t *new_stack, uint16_t &cache_sz, uint16_t *cache_rev_stack)
{
  const uint16_t maxCommonSz = new_sz < cache_sz ? new_sz : cache_sz;
  const uint64_t *new_stack_tail = new_stack + new_sz - 1;
  uint16_t commonSz = 0;
  for (; commonSz < maxCommonSz; ++commonSz, --new_stack_tail, cache_rev_stack += 3)
  {
    if (*new_stack_tail != to_64_bit_pointer_low(cache_rev_stack))
      break;
  }
  cache_sz = new_sz;
  for (; new_stack_tail >= new_stack; --new_stack_tail, cache_rev_stack += 3)
    to_48_bit_pointer(cache_rev_stack, *new_stack_tail);
  return commonSz;
}

void ProfilerData::stackSamplingFunction(const function<bool()> &is_terminating)
{
  uint32_t ourThreadsGeneration = 0;
  vector<SamplingThread> threads;
  vector<uint16_t> threadsOrder;
  threads.reserve(16);
  threadsOrder.reserve(16);
  ThreadStackUnwindProvider *p = start_unwind();
  if (!p)
    return;
  struct TSWPDeleter
  {
    void operator()(ThreadStackUnwindProvider *p) { stop_unwind(p); }
  };
  unique_ptr<ThreadStackUnwindProvider, TSWPDeleter> provider(p);
  {
    std::lock_guard<std::mutex> lock(threadsLock);
    ourThreadsGeneration = interlocked_acquire_load(threadsGeneration);
    uint32_t i = 0;
    for (auto &thread : threadsData)
    {
      if (!thread)
        continue;
      auto threadTlsStorage = thread->tlsStorage;
      if (!threadTlsStorage || !interlocked_acquire_load_ptr(threadTlsStorage)) // removed thread
        continue;
      if (ThreadStackUnwinder *u = start_unwind_thread_stack(*provider.get(), thread->storage.threadId))
        threads.emplace_back(thread->storage.threadId, *u, ourThreadsGeneration, i++);
    }
  }
  const uint32_t cores = get_logical_cores();
  uint64_t expectedRemovalTick = 0;
  const uint64_t freq = cpu_frequency();
  uint64_t lastSamplingHappenedTick = 0;

  std::lock_guard<std::mutex> lock(samplingStateLock);
  for (uint32_t iteration = 0; !is_terminating() && interlocked_acquire_load(samplingThreadRun);)
  {
    const uint32_t threadSampling = interlocked_relaxed_load(threadsSamplingRate);

    if (threadSampling != 0 && ourThreadsGeneration != interlocked_acquire_load(threadsGeneration)) // threads has changed. we need to
                                                                                                    // sample new threads as well
    {
      std::lock_guard<std::mutex> lock(threadsLock);
      ourThreadsGeneration = interlocked_acquire_load(threadsGeneration);
      for (auto &thread : threadsData) // N^2 seach, only reasonable when not so many threads
      {
        if (!thread)
          continue;
        auto threadTlsStorage = thread->tlsStorage;
        if (!threadTlsStorage || !interlocked_acquire_load_ptr(threadTlsStorage)) // removed thread
          continue;
        auto ourIt = find(threads.begin(), threads.end(), thread->storage.threadId,
          [&](const SamplingThread &a, int64_t tid) { return a.tid == tid; });
        if (ourIt != threads.end())
        {
          ourIt->gen = ourThreadsGeneration;
          continue;
        }
        if (ThreadStackUnwinder *u = start_unwind_thread_stack(*provider.get(), thread->storage.threadId))
          threads.emplace_back(thread->storage.threadId, *u, ourThreadsGeneration, 1);
      }
      int ii = 0;
      for (auto ti = threads.begin(); ti != threads.end();)
      {
        if (ti->gen != ourThreadsGeneration)
          threads.erase_unsorted(ti);
        else
        {
          ++ti;
          ti->ii = ii++;
        }
      }
    }

    const uint32_t normalPeriodUs = interlocked_relaxed_load(samplingPeriodUs);
    const int currentMode = interlocked_relaxed_load(active_mode);
    if ((currentMode & SAMPLING) == 0) // sampling is switched off, probably paused during dumping current state
    {
      sleep_usec_safe(0, normalPeriodUs);
      continue;
    }

    const int64_t sampleBeginTicks = cpu_current_ticks();
    const int64_t spikeStartTicks = u64_interlocked_relaxed_load(startSpikeSamplingTick);

    const uint32_t spikePeriodUs =
      spikeStartTicks == 0 ? normalPeriodUs : min(normalPeriodUs, interlocked_relaxed_load(spikeSamplingPeriodUs));

    const bool isInSpike = spikeStartTicks <= sampleBeginTicks;
    // we should wake up not later than spike sampling , though we shouldn't actually sample, if it is not a time
    const uint32_t nextWakeupUsec =
      isInSpike ? spikePeriodUs
                : clamp(uint32_t(ticks_to_usec(spikeStartTicks - sampleBeginTicks, freq)), spikePeriodUs, normalPeriodUs);
    // debug("%p < %p = %d nextWakeupUsec = %d spikePeriodUs = %d normalPeriodUs = %d", spikeStartTicks, sampleBeginTicks, isInSpike,
    // nextWakeupUsec, spikePeriodUs, normalPeriodUs);
    // actual sampling
    const uint32_t currentSamplePeriodUs = isInSpike ? spikePeriodUs : normalPeriodUs;

    if (!usec_passed(sampleBeginTicks - lastSamplingHappenedTick, currentSamplePeriodUs, freq))
    {
      // to avoid starvation of other threads with constant suspends, we enforce sleeping period
      // yes, sampling thread itself will still be in basically busy loop
      // but at least other threads won't be in constantly in suspended mode
      sleep_usec_safe(cores, currentSamplePeriodUs - ticks_to_usec(sampleBeginTicks - lastSamplingHappenedTick, freq));
      continue;
    }

    {
      if (threadSampling != 0)
      {
        // We traverse threads in random order. The resume/suspend thread causes scheduler
        // to re-schedule, and if done in sequence, it'll always schedule the first one to be first.
        // This starves the other N-1 threads. Using a shuffle re-schedules them evenly.
        if (threadsOrder.size() != threads.size())
        {
          threadsOrder.resize(threads.size());
          for (size_t i = 0, e = threadsOrder.size(); i != e; ++i)
            threadsOrder[i] = i;
        }
        for (int n = threadsOrder.size() - 1; n > 0; --n)
          move_swap(threadsOrder[uint32_hash(iteration) % (n + 1)], threadsOrder[n]);
      }
      uint64_t mainThreadId = u64_interlocked_relaxed_load(frameThreadId);
      if (!ttas_spinlock_trylock_fast(sampling_spinlock)) // we call try lock, as if spinlock is locked inside syncStopSampling(), we
                                                          // should skip stack unwinding
        continue;
      struct RAIISpinUnlock
      {
        ~RAIISpinUnlock() { ttas_spinlock_unlock(the_profiler.sampling_spinlock); }
      } unlockMe;
      if ((interlocked_acquire_load(active_mode) & SAMPLING) == 0) // sampling is now switched off
        continue;
      for (auto threadId : threadsOrder)
      {
        auto &thread = threads[threadId];
        if (u64_interlocked_relaxed_load(pauseSamplingThreadId) == thread.tid) // this is lazy pause to prevent useless sampling of
                                                                               // thread during dump
          continue;
        if (thread.tid != mainThreadId && (threadSampling == 0 || ((iteration + uint32_t(thread.ii)) % threadSampling) != 0))
          continue;
        // we can actually replace with reserve/commit logic.
        // downside - we would allocate a bit more in the very end (say, maximum call stack is 32 entries, and typical call stack
        // is 16. We will waste 32*8 =256 bytes in end of chunk) but there would be no memcpy in 64bit
        uint64_t finalStack[MAX_STACK_SIZE];
        const uint64_t ticks = cpu_current_ticks();
        int stackSize;
        stackSize = unwind_thread_stack(*provider.get(), *thread.unwinder, finalStack, MAX_STACK_SIZE);

        if (stackSize > 0)
        {
          uint32_t firstSame = compare_and_update_stack(stackSize, finalStack, thread.lastStackSize, thread.lastRevStack);
          uint8_t differentStack = stackSize - firstSame;

          uint16_t saveCount = differentStack | (firstSame << 8);
          size_t cnt = differentStack * sampling_allocated_address_words + sampling_allocated_additional_words;
          bool newChunk;
          uint16_t *ret = stackSamples.allocContiguous(cnt, newChunk);
          if (newChunk) // we are at the end of chunk, but not completely
          {
            // append allocate to fit whole callstack
            uint16_t *ret2 = stackSamples.allocContiguous((stackSize - differentStack) * 3, newChunk);
            if (!ret2) // not enough space
              ret = nullptr;
            else
            {
              G_FAST_ASSERT(ret2 == ret + cnt);
              // restart state in all threads
              firstSame = 0;
              saveCount = differentStack = stackSize;
              cnt = differentStack * sampling_allocated_address_words + sampling_allocated_additional_words;
              for (auto &t : threads) // invalidate all other threads state
                if (&t != &thread)
                  t.lastStackSize = 0;
            }
          }
          if (ret)
          {
            ret[0] = saveCount;
            // we copy only 48 bits of tid. It is correct on all posix (as tid is pthread*), and it seem to be correct on Windows
            // we can also add increasing numeration of evergrowing chunked array of tids and store just it's id (16bits is ok)/pointer
            // to it (48 bits)
            memcpy(ret + 1, &thread.tid, sizeof(uint16_t) * 3); // -V1086
            memcpy(ret + 1 + 3, thread.lastRevStack + firstSame * 3, differentStack * (sizeof(uint16_t) * 3));
            memcpy(&ret[differentStack * sampling_allocated_address_words + sampling_allocated_additional_words - 4], &ticks,
              sizeof(uint64_t));
          }
          // format used in PrepareDumo, SaveDump, saveVSDump
        }
      }
      ++iteration;
      lastSamplingHappenedTick = sampleBeginTicks;
    }

    const uint64_t sampleEndTicks = cpu_current_ticks();
    const uint64_t removeEarlierTick = u64_interlocked_relaxed_load(firstNeededTick);
    if (removeEarlierTick && removeEarlierTick > expectedRemovalTick)
    {
      const size_t alloc = stackSamples.allocatedBlocks(), useful = stackSamples.usefulBlocks();
      if (alloc > 4 && useful < alloc / 2) //<50%
        stackSamples.shrink_to_fit();
      expectedRemovalTick = sampleEndTicks; // if there is no head
      stackSamples.deleteNonTailChunks([&](const uint16_t *begin, const uint16_t *end) {
        if (end - begin < sampling_allocated_additional_words)
          return false;
        const uint64_t lastTick = sampling_end_ticks(end);
        if (removeEarlierTick <= lastTick)
        {
          expectedRemovalTick = lastTick;
          return false;
        }
        return true;
      });
    }
    const int64_t samplingTookUsec = ticks_to_usec((int64_t)sampleEndTicks - sampleBeginTicks, freq);

    const int64_t sleepFor = max(int64_t(0), int64_t(nextWakeupUsec) - samplingTookUsec);
    sleep_usec_safe(cores, sleepFor);
  };
}

bool ProfilerData::startStackSampling(uint32_t sample) // should be called from addFrame, unless sample is 0 - than can be called from
                                                       // anywhere
{
  sample &= SAMPLING;
  if (!support_sample_other_threads())
    sample = 0;
  if (sample == interlocked_acquire_load(samplingState)) // nothing to change.
    return false;

  interlocked_release_store(samplingState, sample);
  {
    interlocked_release_store(samplingThreadRun, 0);
    std::lock_guard<std::mutex> lock(samplingStateLock); // ensure that sampling thread stopped, because of samplingThreadRun!
  }
  if (sample)
  {
    interlocked_release_store(samplingThreadRun, 1);
    execute_in_new_thread([&](function<bool()> &&is_terminating) { stackSamplingFunction(is_terminating); }, "profiler_stack_thread");
  }
  return true;
}

void ProfilerData::pauseSampling() { interlocked_and(active_mode, ~SAMPLING); }
void ProfilerData::resumeSampling() { interlocked_or(active_mode, interlocked_relaxed_load(samplingState)); }
void ProfilerData::syncStopSampling()
{
  ttas_spinlock_lock(sampling_spinlock);
  interlocked_and(active_mode, ~SAMPLING);
  ttas_spinlock_unlock(sampling_spinlock);
}

}; // namespace da_profiler

DAG_DECLARE_RELOCATABLE(da_profiler::SamplingThread);
