// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

//
// A timeline intuitively is an infinite sequence of events or work items
// emitted by one thread and processed by another thread. It looks something
// like this:
//
//      processing  submitted    acquired
//             |   __________  _____________
//             V  |         | |            |
//  . . . w5  w6  w7  w8  w9  w10  w11  w12  w13  w14  w15 . . .
//
// So each work item (w) has a "time" attached to it, and it's lifetime looks like this:
// 1) wi is `acquire`d by the producer thread
// 2) producer thread fills in wi with a hefty batch of work to be done
// 3) producer thread `submit`s wi
// 4) ... time passes ...
// 5) consumer thread processes wi via `advance`
//

#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_critSec.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/array.h>
#include <atomic>
#include <debug/dag_fatal.h>

namespace drv3d_vulkan
{

typedef uint32_t TimelineHistoryIndex;

enum class TimelineHistoryState
{
  INIT,
  ACQUIRED,
  READY,
  IN_PROGRESS,
  DONE,
  SHUTDOWN,
  COUNT
};

template <typename T>
class TimelineSpan;

// NOTE:
// acquire/submit should be called as-if from a single thread
// advance should also be called as-if from a single thread
// but acquire/submit and advance can be called called concurrently
template <typename ElementImpl, typename SyncType, TimelineHistoryIndex HistoryLength>
class Timeline
{
  class HistoryElement
  {
    ElementImpl impl;
    TimelineHistoryState state = TimelineHistoryState::SHUTDOWN;

#if DAGOR_DBGLEVEL > 0
    eastl::array<uint64_t, (uint32_t)TimelineHistoryState::COUNT> timestamps;
#endif

    void setState(TimelineHistoryState new_state)
    {
#if DAGOR_DBGLEVEL > 0
      timestamps[(uint32_t)new_state] = ref_time_ticks();
#endif
      state = new_state;
    }

  public:
#if DAGOR_DBGLEVEL > 0
    const uint64_t *getTimestamps() const { return timestamps.data(); }
#endif

    bool process()
    {
      if (state != TimelineHistoryState::READY)
        return false;

      impl.process();
      setState(TimelineHistoryState::IN_PROGRESS);
      return true;
    }

    void wait()
    {
      G_ASSERT(state == TimelineHistoryState::IN_PROGRESS);
      impl.wait();
      setState(TimelineHistoryState::DONE);
    }

    void cleanup()
    {
      G_ASSERT(state == TimelineHistoryState::DONE);
      impl.cleanup();
      setState(TimelineHistoryState::INIT);
    }

    void acquire(size_t abs_index)
    {
      if (state == TimelineHistoryState::DONE)
        cleanup();

      G_ASSERT(state == TimelineHistoryState::INIT);
      impl.acquire(abs_index);
      setState(TimelineHistoryState::ACQUIRED);
    }

    void submit()
    {
      G_ASSERT(state == TimelineHistoryState::ACQUIRED);
      impl.submit();
      setState(TimelineHistoryState::READY);
    }

    bool isCompleted() const { return state == TimelineHistoryState::DONE; }

    ElementImpl &getImpl()
    {
      G_ASSERT(state == TimelineHistoryState::ACQUIRED);
      return impl;
    }

    const ElementImpl &getImpl() const //-V659
    {
      return impl;
    }

    void shutdown()
    {
      impl.shutdown();
      setState(TimelineHistoryState::SHUTDOWN);
    }

    void init()
    {
      G_ASSERT(state == TimelineHistoryState::SHUTDOWN);
      // optional impl.init() here
      setState(TimelineHistoryState::INIT);
    }
  };

  static constexpr bool keep_history = DAGOR_DBGLEVEL != 0; // TODO: move it to config?

  SyncType advanceSync;
  SyncType submitSync;
  typename SyncType::Lock acquireLock;

  typename SyncType::PendingCounterType pending;
  typename SyncType::AcquireCounterType acquiredButNotSubmitted;
  size_t time = 0;

  // history ring buffer
  TimelineHistoryIndex nextAcquireIdx = 0;
  TimelineHistoryIndex currentProcessIdx = 0;
  void ringInc(TimelineHistoryIndex &val) { val = (val + 1) % HistoryLength; }
  eastl::array<HistoryElement, HistoryLength> history;

  friend class TimelineSpan<Timeline>;

  ElementImpl &getElement(TimelineHistoryIndex idx)
  {
    G_ASSERT(idx < HistoryLength);
    return history[idx].getImpl();
  }

  static constexpr int INVALID_HISTORY_INDEX = -1;

  typedef ElementImpl Element;
  typedef SyncType Sync;

public:
  void init()
  {
    for (TimelineHistoryIndex i = 0; i < HistoryLength; ++i)
      history[i].init();
  }

  void shutdown()
  {
    for (TimelineHistoryIndex i = 0; i < HistoryLength; ++i)
      history[i].shutdown();

    pending.reset();
    acquiredButNotSubmitted.reset();
    time = 0;
    nextAcquireIdx = 0;
    currentProcessIdx = 0;
  }

  // Acquires the next element in the timeline for preparing it for submission.
  // NOTE: it is the user's responsibility to not acquire more than HistoryLength elements,
  // call `advance()` eventually, and call waitAcquireSpace if they cannot guarantee free space
  // for the acquire call through external synchronization.
  [[nodiscard]] TimelineHistoryIndex acquire()
  {
    const TimelineHistoryIndex observedPending = pending.get();
    const TimelineHistoryIndex observedAcquiredButNotSubmitted = acquiredButNotSubmitted.fetchAdd();

    // User has to call waitAcquireSpace to avoid this branch
    if (observedAcquiredButNotSubmitted + observedPending >= HistoryLength)
    {
      acquiredButNotSubmitted.fetchDone();
      G_ASSERTF(false,
        "vulkan: total amount of non-completed timeline elements %d exceeded the ring buffer size %d!"
        " No elements available to acqurie!",
        observedAcquiredButNotSubmitted + observedPending, HistoryLength);
      return -1;
    }

    if (observedPending >= HistoryLength)
    {
      DAG_FATAL("vulkan: total amount of pending timeline elements %d exceeded the ring buffer size %d!"
                " No elements available to acqurie!",
        observedPending, HistoryLength);
      return -1; // tell compiler that we will not proceed down when we FATAL
    }

    // We have synchronized-with the `pending.fetchDone()` call in `advance()` that has
    // last touched history[nextAcquireIdx] and so we can now re-use it without
    // a data race.
    TimelineHistoryIndex ret;
    {
      typename SyncType::AutoLock l(acquireLock);
      history[nextAcquireIdx].acquire(time);
      ret = nextAcquireIdx;
      ringInc(nextAcquireIdx);
      time++;
    }

    return ret;
  }

  // Submits an element that was acquired earlier for further processing in a different thread.
  void submit(TimelineHistoryIndex idx)
  {
    history[idx].submit();
    // This synchronizes-with the read in `advance()`
    const TimelineHistoryIndex observedPending = pending.fetchAdd();
    G_UNUSED(observedPending);
    G_ASSERT(observedPending < HistoryLength);

    const TimelineHistoryIndex observedAcquiredButNotSubmitted = acquiredButNotSubmitted.fetchDone();
    G_ASSERT_RETURN(observedAcquiredButNotSubmitted > 0, );

    submitSync.signal();
  }

private:
  bool process()
  {
    HistoryElement &el = history[currentProcessIdx];
    if (!el.isCompleted())
    {
      if (!el.process())
        return false;
    }
    return true;
  }

public:
  // Processes the next submitted element and advances the timeline.
  // Must only be called from a single thread.
  bool advance()
  {
    const TimelineHistoryIndex observedPending = pending.get();
    if (observedPending == 0)
      return false;

    // `pending.get()` synchronized with all previous RMW operations (due to release sequences).
    // We have also observed it to be >0, which means that the `pending.fetchAdd()`
    // which happened in the `submit()` call that last touched history[currentProcessIdx]
    // is included among those RMW operations that have "already happened".
    // Hence, we can touch this history element without a data race.
    if (!process())
      return false;

    history[currentProcessIdx].wait();
    if (!keep_history)
      history[currentProcessIdx].cleanup();

    ringInc(currentProcessIdx);

    // This synchronizes-with the `pending.get()` in `acquire()` that "loops around" and
    // re-uses this element for a new timeline element, preventing a data race.
    pending.fetchDone();

    advanceSync.signal();

    return true;
  }

  bool waitAcquireSpace(uint32_t max_retries)
  {
    // acquire/submit should be extern synced for now,
    // so we need to wait advance in case that all elements are in pending and acquired state

    // it is logic error if we acquire all elements and wait for space
    G_ASSERT(acquiredButNotSubmitted.get() != HistoryLength);
    return advanceSync.waitCond(
      [this]() {
        const TimelineHistoryIndex observedPending = this->pending.get();
        const TimelineHistoryIndex observedAcquiredButNotSubmitted = acquiredButNotSubmitted.get();
        return observedPending + observedAcquiredButNotSubmitted < HistoryLength;
      },
      max_retries);
  }

  bool waitAdvance(TimelineHistoryIndex max_pending, uint32_t max_retries)
  {
    return advanceSync.waitCond(
      [this, max_pending]() {
        const TimelineHistoryIndex observedPending = this->pending.get();
        return observedPending <= max_pending;
      },
      max_retries);
  }

  bool waitSubmit(TimelineHistoryIndex min_pending, uint32_t max_retries)
  {
    return submitSync.waitCond(
      [this, min_pending]() {
        const TimelineHistoryIndex observedPending = this->pending.get();
        return observedPending >= min_pending;
      },
      max_retries);
  }

  // NOTE: completely not thread safe. Use at your own risk.
  template <typename T>
  void enumerate(T cb) const
  {
    for (TimelineHistoryIndex i = 0; i < HistoryLength; ++i)
      cb(i, history[i].getImpl());
  }

#if DAGOR_DBGLEVEL > 0
  // NOTE: completely not thread safe. Use at your own risk.
  template <typename T>
  void enumerateTimestamps(T cb) const
  {
    for (TimelineHistoryIndex i = 0; i < HistoryLength; ++i)
      // ignore data race here, just simply read as is
      cb(i, history[i].getTimestamps());
  }
#endif
};

struct TimelineSyncPartLockFree
{
  struct Lock
  {};
  struct AutoLock
  {
    AutoLock(const Lock &) {}
  };
};

class TimelineSyncPartEventWaitable
{
  static constexpr size_t EV_TIMEOUT_MS = 10;
  os_event_t ev; //-V730_NOINIT

public:
  TimelineSyncPartEventWaitable() { os_event_create(&ev); }
  ~TimelineSyncPartEventWaitable() { os_event_destroy(&ev); }

  void signal() { os_event_set(&ev); }
  void wait() { os_event_wait(&ev, EV_TIMEOUT_MS); }

  template <typename T>
  bool waitCond(T cb, uint32_t max_retries)
  {
    while (!cb() && max_retries)
    {
      wait();
      --max_retries;
    }
    return cb();
  }
};

struct ConcurrentWorkCounter
{
  // Synchronizes-with all `fetchAdd` and `fetchDone` calls (through release sequences)
  [[nodiscard]] TimelineHistoryIndex get() { return value.load(std::memory_order_acquire); }
  TimelineHistoryIndex fetchAdd() { return value.fetch_add(1, std::memory_order_release); }
  TimelineHistoryIndex fetchDone() { return value.fetch_sub(1, std::memory_order_release); }

  void reset() { value.store(0, std::memory_order_relaxed); }

private:
  std::atomic<TimelineHistoryIndex> value = 0;
};

struct WorkCounter
{
  [[nodiscard]] TimelineHistoryIndex get() { return value; }
  TimelineHistoryIndex fetchAdd() { return value++; }
  TimelineHistoryIndex fetchDone() { return value--; }

  void reset() { value = 0; }

private:
  TimelineHistoryIndex value = 0;
};

struct TimelineSyncPartSingleWriterSingleReader
{
  using PendingCounterType = ConcurrentWorkCounter;
  using AcquireCounterType = WorkCounter;
};

struct TimelineSyncPartNonConcurrent
{
  using PendingCounterType = WorkCounter;
  using AcquireCounterType = WorkCounter;
};


struct TimelineSyncPartNonWaitable
{
  void nonWaitableError() { DAG_FATAL("vulkan: timeline is not waitable"); }

  void signal() {}
  void wait() { nonWaitableError(); }

  template <typename T>
  bool waitCond(T, uint32_t)
  {
    nonWaitableError();
    return false;
  };
};

template <typename T>
class TimelineSpan
{
  T &timelineRef;
  TimelineHistoryIndex idx = T::INVALID_HISTORY_INDEX;

  TimelineSpan(const TimelineSpan &) = delete;

public:
  template <typename Manager>
  TimelineSpan(Manager &tl_man) : timelineRef(tl_man.template get<T>())
  {}

  void start()
  {
    G_ASSERT(idx == T::INVALID_HISTORY_INDEX);
    idx = timelineRef.acquire();
  }

  void end()
  {
    G_ASSERT(idx != T::INVALID_HISTORY_INDEX);
    timelineRef.submit(idx);
    idx = -1;
  }

  void restart()
  {
    end();
    start();
  }

  ~TimelineSpan() { G_ASSERT(idx == T::INVALID_HISTORY_INDEX); }

  typename T::Element *operator->() { return &timelineRef.getElement(idx); }

  typename T::Element &get() { return timelineRef.getElement(idx); }
};

} // namespace drv3d_vulkan
