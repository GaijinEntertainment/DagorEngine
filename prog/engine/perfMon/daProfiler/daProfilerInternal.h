// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#undef DA_PROFILER_ENABLED
#define DA_PROFILER_ENABLED 1
#include "daProfilerDefines.h"
#include <perfMon/dag_daProfiler.h>
#include "stl/daProfilerFwdStl.h"
#include "stl/daProfilerString.h"  //for plugins
#include "stl/daProfilerHashmap.h" //for plugins
#include <mutex>
#include "dag_chunkedStorage.h"
#include "daProfilerUserSettings.h"
#include "daProfilerTypes.h"
#include "daProfilerDescriptions.h"

class IGenSave;
#define DA_PROFILE_NET_IS_POSSIBLE (DAPROFILER_DEBUGLEVEL > 0)
class IGenLoad;

class TimeIntervalNode;
struct TimeIntervalInfo;
typedef void (*ProfilerDumpFunctionPtr)(void *ctx, uintptr_t cNode, uintptr_t pNode, const char *, const TimeIntervalInfo &);
namespace da_profiler
{
static constexpr bool combine_ring_captures = (DAPROFILER_DEBUGLEVEL > 0) ? false : true;
using UniqueEventDataStorage = ChunkedStorage<UniqueEventData *, 1, VirtualPageAllocator>;

struct ThreadStorage
{
  typedef uint32_t local_hash_t;
  EventDataStorage events;
  uint64_t nextFreeEventTick = 0;
  GpuEventDataStorage gpuEvents;
  StringTagDataStorage stringTags;
  uint64_t nextFreeTagTick = 0;
  HashedKeyMap<local_hash_t, uint32_t> hashToDescrId; // for local descriptions
  uint64_t threadId = 0;
  uint32_t description = 0;
  uint32_t depth = 0;
  uint32_t gpuDepth = 0;
  void freeEventChunks(uint64_t olderThan, uint64_t curTime); // OOL
  void freeTagChunks(uint64_t olderThan, uint64_t curTime);   // OOL
  void shrink_to_fit()
  {
    events.shrink_to_fit();
    gpuEvents.shrink_to_fit();
    stringTags.shrink_to_fit();
  }
  uint64_t memAllocated() const { return events.memAllocated() + gpuEvents.memAllocated() + stringTags.memAllocated(); }
  // uint64_t usefulMemory() const { return events.usefulMemory() + gpuEvents.usefulMemory() + stringTags.usefulMemory(); }
  bool shouldShrink() const
  {
    const size_t allocated = events.allocatedBlocks() + gpuEvents.allocatedBlocks();
    if (allocated < 6)
      return false;

    const size_t useful = events.usefulBlocks() + gpuEvents.usefulBlocks();
    return useful * 3 / 2 < allocated;
  }
  void clear()
  {
    events.clear();
    gpuEvents.clear();
    stringTags.clear();
    depth = gpuDepth = 0;
  }
};

struct ProfilerData
{
  static char uniqueProfileRunName[64]; // this name is generated once in executable, and should be different for any different exe/run
  void initUniqueName();
  static constexpr uint32_t NUM_FRAMES = 256;
  struct PerThreadInfo
  {
    ThreadStorage storage;
    ThreadStorage **tlsStorage = NULL;
    uint64_t addedTick = 0;
    uint64_t removedTick = ~0ull;
  };

  struct Frames
  {
    static constexpr uint32_t num_frames = NUM_FRAMES, num_frames_mask = num_frames - 1;
    FramesStorage frames;
    uint32_t curFrameIndex() const { return uint32_t(frameCount - 1) & num_frames_mask; }
    uint32_t nextFrameIndex() const { return frameCount & num_frames_mask; }
    uint32_t activeFramesCount() const { return frameCount >= num_frames ? num_frames : frameCount; }
    uint32_t startFrame() const { return frameCount >= num_frames ? (frameCount & num_frames_mask) : 0; }
    uint32_t maxNumFrames() const { return num_frames; }
    FrameInfo &operator[](size_t i) { return frames[i & num_frames_mask]; }
    const FrameInfo &operator[](size_t i) const { return frames[i & num_frames_mask]; }
    uint64_t frameCount = 0;
  } framesInfo;

  FramesStorage infiniteFrames;
  uint64_t lastCopiedGpuInfiniteFrameId = 0;
  uint64_t currentContinuousDumpSize = 0;

  uint64_t cpuFrameMovingAverageTick = 0;                      // this is cpu
  uint64_t cpuSpikesThresholdTicks = 0, cpuSpikesAddTicks = 0; // user setting
  uint64_t currentCpuSpikesThresholdTicks = 0;                 // calculated each frame
  uint32_t cpuSpikesMulAverage = 3;

  bool profileCpuSpike(uint64_t frame_ticks, uint32_t save_first, uint32_t save_count); // internal for addFrame
  void addFrame();
  static void addLeafEvent(uint32_t description, uint64_t start, uint64_t end);
  static EventData *startEvent(uint32_t description, ThreadStorage *&storage);
  static void endEvent(EventData &event, ThreadStorage &storage);
  static void addTag(uint32_t description, const char *tag);
  static void addTag(uint32_t description, const char *fmt, const TagSingleWordArgument *args, uint32_t ac);
  static void endUniqueEvent(UniqueEventData &ued, uint64_t start);
  void addUniqueEvent(UniqueEventData &ued);
  UniqueEventData *getUniqueEvent(uint32_t i, uint32_t &frames);

  GpuEventData *startGPUEvent(uint32_t description, const char *d3d_event_name);
  static void endGPUEvent(GpuEventData &e);

  uint32_t getTLSDescription(const char *name, const char *file_name, int line, uint32_t flags, uint32_t color);

  ProfilerData();
  ~ProfilerData();

  int findThreadIndexUnsafe(uint64_t threadId) const;
  int findThreadIndex(uint64_t threadId) const;

  RegisterThreadResult addThreadData(uint32_t description);
  RegisterThreadResult addThreadData(const char *name, const char *file, uint32_t line);
  bool addFrameThread(const char *name, const char *file, uint32_t line);
  bool removeCurrentThread();
  void shutdown(bool dump_mem_usage);
  ThreadStorage *initializeFrameThread(); // initialize frame_thread

  void closeRemovedThreads();
  static uint64_t firstNeededTick; // everything before that is not needed and has to be removed

  static thread_local ThreadStorage *tls_storage;
  static thread_local bool frame_thread;
  static bool isFrameThread() { return frame_thread; }
  uint64_t frameThreadId = 0;
  EventData *frameEvent = NULL;
  GpuEventData *frameGPUEvent = NULL;
  // UniqueEvents
  UniqueEventDataStorage uniqueEvents;
  uint32_t uniqueEventsFrames = 0;
  // thread data
  mutable std::mutex threadsLock;
  vector<PerThreadInfo> threadsDataStor;
  vector<PerThreadInfo *> threadsData;
  uint32_t threadsGeneration = 0;
  int removedThreadsCount = 0;

  // sampling
  UserSettings settings;
  uint32_t sentSettingsGen = ~0u;

  uint64_t startSpikeSamplingTick = 0; // for high freq sampling spikes, we sample only those who are later than tick
  uint32_t samplingPeriodUs = 10000, spikeSamplingPeriodUs = 1000, threadsSamplingRate = 3;
  uint64_t nextSpikeFrameCheck = 0; // since we save three frames before and after spike, we don't want to save same frame twice
  uint64_t pauseSamplingThreadId = 0;

  uint32_t samplingState = 0;
  uint32_t samplingThreadRun = 0;
  mutable std::mutex samplingStateLock;
  volatile int sampling_spinlock = 0; // to allow emergency stop
  void stackSamplingFunction(const function<bool()> &is_terminating);
  bool startStackSampling(uint32_t new_mode); // should be called from addFrame
  void stopStackSampling() { startStackSampling(0); }
  void pauseSampling();
  void resumeSampling();
  void syncStopSampling();
  // sampling events format (in uint64_t's), for each chunk
  // 0: stack_size. if stack_size > MAX_STACK_SIZE, than everything is invalid till end of chunk, and it is ticks of last event (first
  // event in next chunk) 1: ticks, when event happened 2: thread_id
  //..stack_size.. of stacks
  static constexpr int MAX_STACK_SIZE = 96;
  struct LastRevCallStack // -V730
  {
    uint64_t revStack[MAX_STACK_SIZE]; // -V730
    uint32_t currentStackSize = 0;
  };
  CallStackStorage stackSamples;
  //---

  uint32_t saveDumpProcessed = 0;
  int boardNumber = 0;

  bool isContinuousLimitReached() const
  {
    const uint32_t framesLimit = interlocked_relaxed_load(settings.continuousLimitFrames);
    // if exceed mem limit, dump profile!
    return (framesLimit && framesLimit <= infiniteFrames.approximateSize());
  }
  size_t saveDump(uint32_t requests, bool due_to_limit)
  {
    if (!due_to_limit && interlocked_acquire_load(saveDumpProcessed) == requests)
      return 0;
    interlocked_release_store(saveDumpProcessed, requests);
    return infiniteFrames.empty() ? dumpRingProfile() : dumpContinuosProfile(due_to_limit);
  }
  void copyCpuEvents(const uint64_t timeStart, const uint64_t timeEnd, vector<Dump::Thread> &cpu) const;
  void copyGpuEvents(const uint64_t timeStart, const uint64_t timeEnd, GpuEventDataStorage &gpu,
    ClockCalibration &cpu_gpu_clock) const;
  void copyCallStacks(uint64_t timeStart, uint64_t timeEnd, CallStackDumpStorage &save) const;
  size_t prepareDump(unique_ptr<Dump> &&dump);

  void saveDump(IGenSave &cb, const Dump &dump) const { save_dump(cb, dump, descriptions, symbolsCache); }

  size_t dumpRingProfile();
  size_t dumpContinuosProfile(bool append_to_last);

  // descriptions
  ProfilerDescriptions descriptions;

  void setSampling(uint32_t sampling_rate_freq, uint32_t sampling_spike_rate_freq, uint32_t threads_sampling_rate_mul);
  void setSpikes(uint32_t min_msec, uint32_t average_mul, uint32_t add_msec);
  void shrinkMemory();

  mutable SymbolsCache symbolsCache;
  // plugins
  volatile int pluginSpinlock = 0;
  hash_map<string, ProfilerPlugin *> plugins;
  volatile int pluginGeneration = 0, pluginsSent = 0;
  bool registerPlugin(const char *name, ProfilerPlugin *p);
  bool unregisterPlugin(const char *name);
  ProfilerPluginStatus setPluginEnabled(const char *name, bool enabled);
  void sendPlugins();
  void unregisterPlugins();
};

extern ProfilerData the_profiler;

#include <osApiWrappers/dag_atomic.h>
#if _TARGET_64BIT
// on 64 bit platform firstTick is atomic.
__forceinline void u64_interlocked_release_store(volatile uint64_t &i, uint64_t val) { interlocked_release_store(i, val); }
__forceinline void u64_interlocked_relaxed_store(volatile uint64_t &i, uint64_t val) { interlocked_relaxed_store(i, val); }
// we use relaxed load, as it is much faster. And it is not really dangeours - worst case we loose some thread data during dump
__forceinline uint64_t u64_interlocked_relaxed_load(volatile const uint64_t &i) { return interlocked_relaxed_load(i); }
__forceinline uint64_t u64_interlocked_acquire_load(volatile const uint64_t &i) { return interlocked_acquire_load(i); }
__forceinline uint64_t u64_interlocked_add(volatile uint64_t &i, uint64_t v) { return interlocked_add(i, v); }
#else
// on 32bit platform it is data-race.
// it is not very dangerous, as in a worst case scenario it causes free of some useful thread data profiling. No crash or anything like
// that 32-bit is dying platform anyway
__forceinline void u64_interlocked_release_store(volatile uint64_t &i, uint64_t val) { i = val; }
__forceinline void u64_interlocked_relaxed_store(volatile uint64_t &i, uint64_t val) { i = val; }
__forceinline uint64_t u64_interlocked_relaxed_load(volatile const uint64_t &i) { return i; }
__forceinline uint64_t u64_interlocked_acquire_load(volatile const uint64_t &i) { return i; }
__forceinline uint64_t u64_interlocked_add(volatile uint64_t &i, uint64_t v) { return i += v; }
#endif

#if DA_PROFILE_MORE_THREAD_SAFE
extern uint32_t global_events_started;
struct ScopedThreadSafety
{
  ScopedThreadSafety() { interlocked_increment(global_events_started); }
  ~ScopedThreadSafety() { interlocked_decrement(global_events_started); }
} static inline bool is_safe_to_shutdown() { return interlocked_acquire_load(global_events_started) == 0; }
#define SCOPED_THREAD_SAFETY \
  ScopedThreadSafety someTs; \
  (void)someTs;
// actually safe
static __forceinline uint64_t more_safe_u64_interlocked_load(const uint64_t &tick) { return u64_interlocked_acquire_load(tick); }
static __forceinline uint32_t more_safe_post_inc(uint32_t &v) { return interlocked_increment(v) - 1; }
static __forceinline void more_safe_dec(uint32_t &v) { interlocked_decrement(v); }
static __forceinline uint32_t more_safe_get(const uint32_t &v) { return interlocked_acquire_load(v); }
#else

#define SCOPED_THREAD_SAFETY
static inline bool is_safe_to_shutdown() { return true; }

static __forceinline uint64_t more_safe_u64_interlocked_load(const uint64_t &tick) { return u64_interlocked_relaxed_load(tick); }
#if DAGOR_THREAD_SANITIZER
// just to shut tsan
static __forceinline uint32_t more_safe_post_inc(uint32_t &v)
{
  uint32_t cv = interlocked_relaxed_load(v);
  interlocked_relaxed_store(v, cv + 1);
  return cv;
}
static __forceinline void more_safe_dec(uint32_t &v) { interlocked_relaxed_store(v, interlocked_relaxed_load(v) - 1); }
static __forceinline uint32_t more_safe_get(const uint32_t &v) { return interlocked_relaxed_load(v); }
#else
static __forceinline uint32_t more_safe_get(const uint32_t &v) { return v; }
static __forceinline uint32_t more_safe_post_inc(uint32_t &v) { return v++; }
static __forceinline void more_safe_dec(uint32_t &v) { --v; }
#endif
#endif

static __forceinline uint64_t safe_first_needed_tick() { return more_safe_u64_interlocked_load(ProfilerData::firstNeededTick); }

template <class T>
__forceinline void free_event_chunks(T &storage, uint64_t delete_older_than_that)
{
  storage.deleteNonTailChunks([&](const typename T::value_t *start, const typename T::value_t *end) {
    if (end == start || end[-1].end >= delete_older_than_that) // latest event in chunk is older than threshold
      return false;
    // unfortunately, oldest isn't always latest. iterate all to find if we should remove
    for (; start != end; ++start)
      if (u64_interlocked_relaxed_load(start->end) >= delete_older_than_that)
        return false;
    return true;
  });
}

}; // namespace da_profiler