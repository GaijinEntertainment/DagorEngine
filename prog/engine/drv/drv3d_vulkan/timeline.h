// classes for represenation & managment of various work blocks
// with ability to move current execution point
#pragma once
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_critSec.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/array.h>
#include <atomic>
#include <debug/dag_fatal.h>

namespace drv3d_vulkan
{

typedef uint32_t TimelineHistoryIndex;

enum class TimelineHisotryState
{
  INIT,
  ACQUIRED,
  READY,
  IN_PROGRESS,
  DONE,
  SHUTDOWN,
  COUNT
};

// NOTE:
// acquire/submit should be extern synced
// advance/process can be called only from 1 thread

template <typename ElementImpl, typename SyncType, TimelineHistoryIndex HistoryLength>
class Timeline
{
public:
private:
  class HistoryElement
  {
    ElementImpl impl;
    TimelineHisotryState state = TimelineHisotryState::SHUTDOWN;

#if DAGOR_DBGLEVEL > 0
    eastl::array<uint64_t, (uint32_t)TimelineHisotryState::COUNT> timestamps;
#endif

    void setState(TimelineHisotryState new_state)
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
      if (state != TimelineHisotryState::READY)
        return false;

      impl.process();
      setState(TimelineHisotryState::IN_PROGRESS);
      return true;
    }

    void wait()
    {
      G_ASSERT(state == TimelineHisotryState::IN_PROGRESS);
      impl.wait();
      setState(TimelineHisotryState::DONE);
    }

    void cleanup()
    {
      G_ASSERT(state == TimelineHisotryState::DONE);
      impl.cleanup();
      setState(TimelineHisotryState::INIT);
    }

    void acquire(size_t abs_index)
    {
      if (state == TimelineHisotryState::DONE)
        cleanup();

      G_ASSERT(state == TimelineHisotryState::INIT);
      impl.acquire(abs_index);
      setState(TimelineHisotryState::ACQUIRED);
    }

    void submit()
    {
      G_ASSERT(state == TimelineHisotryState::ACQUIRED);
      impl.submit();
      setState(TimelineHisotryState::READY);
    }

    bool isCompleted() const { return state == TimelineHisotryState::DONE; }

    ElementImpl &getImpl()
    {
      G_ASSERT(state == TimelineHisotryState::ACQUIRED);
      return impl;
    }

    const ElementImpl &getImpl() const //-V659
    {
      return impl;
    }

    void shutdown()
    {
      impl.shutdown();
      setState(TimelineHisotryState::SHUTDOWN);
    }

    void init()
    {
      G_ASSERT(state == TimelineHisotryState::SHUTDOWN);
      // optional impl.init() here
      setState(TimelineHisotryState::INIT);
    }
  };

  static constexpr bool keep_history = DAGOR_DBGLEVEL != 0; // TODO: move it to config?

  SyncType advanceSync;
  SyncType submitSync;
  typename SyncType::Lock acquireLock;

  typename SyncType::PendingCounterType pending{0};
  typename SyncType::AcquireCounterType acquireCnt{0};
  size_t absIndex = 0;

  // history ring buffer
  TimelineHistoryIndex acquireIdx = 0;
  TimelineHistoryIndex currentIdx = 0;
  void ringInc(TimelineHistoryIndex &val) { val = (val + 1) % HistoryLength; }
  eastl::array<HistoryElement, HistoryLength> history;

public:
  typedef ElementImpl Element;
  typedef SyncType Sync;

  static constexpr int INVALID_HISTORY_INDEX = -1;

  TimelineHistoryIndex acquire()
  {
    G_ASSERT((acquireCnt + pending) < HistoryLength);
    if (pending >= HistoryLength) // atomic read needed for proper sync
      fatal("vulkan: no element to acquire in timeline");
    SyncType::sync_point_acquire();

    TimelineHistoryIndex ret;
    {
      typename SyncType::AutoLock l(acquireLock);
      history[acquireIdx].acquire(absIndex);
      ret = acquireIdx;
      ringInc(acquireIdx);
    }

    ++absIndex;
    ++acquireCnt;
    return ret;
  }

  void submit(TimelineHistoryIndex idx)
  {
    G_ASSERT(acquireCnt);
    history[idx].submit();
    ++pending; // sync barrier
    --acquireCnt;
    G_ASSERT(pending <= HistoryLength);
    submitSync.signal();
  }

  bool process()
  {
    SyncType::sync_point_process();
    HistoryElement &el = history[currentIdx];
    if (!el.isCompleted())
    {
      if (!el.process())
        return false;
    }
    return true;
  }

  bool advance()
  {
    if (pending == 0)
      return false;

    if (!process())
      return false;
    history[currentIdx].wait();
    if (!keep_history)
      history[currentIdx].cleanup();

    ringInc(currentIdx);
    --pending; // sync barrier

    advanceSync.signal();

    return true;
  }

  bool waitAcquireSpace(uint32_t max_retries)
  {
    // acquire/submit should be extern synced for now,
    // so we need to wait advance in case that all elements are in pending and acquired state
    G_ASSERT(acquireCnt != HistoryLength); // it is logic error if we acquire all elements and wait for space
    return advanceSync.waitCond([this]() { return (this->pending + this->acquireCnt) < HistoryLength; }, max_retries);
  }

  bool waitAdvance(TimelineHistoryIndex max_pending, uint32_t max_retries)
  {
    return advanceSync.waitCond([this, max_pending]() { return this->pending <= max_pending; }, max_retries);
  }

  bool waitSubmit(TimelineHistoryIndex min_pending, uint32_t max_retries)
  {
    return submitSync.waitCond([this, min_pending]() { return this->pending >= min_pending; }, max_retries);
  }

  TimelineHistoryIndex getPendingAmount() const { return pending; }

  ElementImpl &getElement(TimelineHistoryIndex idx)
  {
    G_ASSERT(idx < HistoryLength);
    return history[idx].getImpl();
  }

  template <typename T>
  void enumerate(T cb) const
  {
    for (TimelineHistoryIndex i = 0; i < HistoryLength; ++i)
      cb(i, history[i].getImpl());
  }

  void shutdown()
  {
    for (TimelineHistoryIndex i = 0; i < HistoryLength; ++i)
      history[i].shutdown();

    pending = 0;
    acquireCnt = 0;
    absIndex = 0;
    acquireIdx = 0;
    currentIdx = 0;
  }

  void init()
  {
    for (TimelineHistoryIndex i = 0; i < HistoryLength; ++i)
      history[i].init();
  }

  static TimelineHistoryIndex get_history_length() { return HistoryLength; }
#if DAGOR_DBGLEVEL > 0
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

struct TimelineSyncPartSingleWriterSingleReader
{
  static void sync_point_acquire() { std::atomic_thread_fence(std::memory_order_acquire); }

  static void sync_point_process() { std::atomic_thread_fence(std::memory_order_acquire); }

  typedef std::atomic<TimelineHistoryIndex> PendingCounterType;
  typedef TimelineHistoryIndex AcquireCounterType;
};

struct TimelineSyncPartNonConcurrent
{
  static void sync_point_acquire() {}
  static void sync_point_process() {}

  typedef TimelineHistoryIndex PendingCounterType;
  typedef TimelineHistoryIndex AcquireCounterType;
};

struct TimelineSyncPartNonWaitable
{
  void nonWaitableError() { fatal("vulkan: timeline is not waitable"); }

  void signal(){};
  void wait() { nonWaitableError(); };

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
