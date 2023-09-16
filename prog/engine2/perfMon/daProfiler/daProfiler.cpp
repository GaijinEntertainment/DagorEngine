#include <osApiWrappers/dag_atomic.h>
#include <mutex>
#include "daGpuProfiler.h"
#include "daProfilerInternal.h"
#include "daProfilePlatform.h"
#include "daProfilerDumpServer.h"
#include "stl/daProfilerStl.h"
#include "stl/daProfilerAlgorithm.h"

#define ISSUE_D3D_EVENTS (DAPROFILER_DEBUGLEVEL > 0)
namespace da_profiler
{
#if DA_PROFILE_MORE_THREAD_SAFE
uint32_t global_events_started = 0;
#endif

int active_mode = 0; // active_mode changed only in addFrame
static uint32_t next_dump_requested = 0;

uint64_t ProfilerData::firstNeededTick = 0;
thread_local ThreadStorage *ProfilerData::tls_storage = NULL;
thread_local bool ProfilerData::frame_thread = false;

char ProfilerData::uniqueProfileRunName[64]; // this name is generated once in executable, and should be different for any different
                                             // exe/run

void ProfilerData::initUniqueName()
{
  const uint64_t stamp = global_timestamp();
  const char *exe = get_executable_name(); // can not be called in constructor
  if (!exe)
    exe = "";
  snprintf(uniqueProfileRunName, sizeof(uniqueProfileRunName), "%llx_%x",
    (unsigned long long)wyhash(exe, strlen(exe), cpu_current_ticks()), (uint32_t)stamp);
}

ProfilerData::ProfilerData()
{
  snprintf(uniqueProfileRunName, sizeof(uniqueProfileRunName), "%x", (uint32_t)global_timestamp());
  init_cpu_frequency();

  cpuSpikesThresholdTicks = cpu_frequency() / (1000 / 100); // 100 msec
  cpuSpikesAddTicks = cpu_frequency() / (1000 / 5);         // 5 msec
}

ProfilerData::~ProfilerData()
{
  // switch off profiling
  shutdown(false);

  while (threadsData.size() > threadsDataStor.size())
  {
    delete threadsData.back();
    threadsData.pop_back();
  }
}

static __forceinline EventData *start_cpu_event(uint32_t description, ThreadStorage &storage)
{
  const uint32_t currentDepth = more_safe_post_inc(storage.depth);
  auto &e = storage.events.push_back(EventData{~0ull, ~0ull, description, currentDepth});
  u64_interlocked_relaxed_store(e.start, cpu_current_ticks()); // as last as possible, to make more accurate measurement of actual
                                                               // event
  return &e;
}

__forceinline void ProfilerData::addLeafEvent(uint32_t description, uint64_t start, uint64_t end)
{
  SCOPED_THREAD_SAFETY;
  ThreadStorage *storage = interlocked_relaxed_load_ptr(tls_storage);
  if (DAGOR_UNLIKELY(!storage))
    return;

  storage->events.push_back(EventData{start, end, description, more_safe_get(storage->depth)});
}

__forceinline EventData *ProfilerData::startEvent(uint32_t description, ThreadStorage *&storage)
{
  //! documented data-race!
  // here we have data race during shutdown/destructor/dump.
  //    dumping is more or less OK - in a worst case we skip some thread data (and very unlikely to happen)
  //    destructor/thread removal can really cause UB - writing to freed memory
  //   the reason we use relaxed loads/increments, rather than acquire
  //  to avoid data race to really practicaly happen, we assume:
  //   shutdown(), is supposed to be happening long before destructors
  //   we call sleep(1) in shutdown
  //   we do check for several times. that isn't 100% safety, because depth is not atomic
  // avoiding this data-race is possible with
  // atomic counter on all started events
  // #define DA_PROFILE_MORE_THREAD_SAFE 1
  //   obviously, this will slow down profiling, and we don't really want that

  SCOPED_THREAD_SAFETY;
  storage = interlocked_relaxed_load_ptr(tls_storage);
  if (DAGOR_UNLIKELY(!storage))
    return nullptr;
  PLATFORM_EVENT_START(get_active_mode() & PLATFORM_EVENTS, the_profiler.descriptions.get(description)); // per platform event start
  // this is data-race here still during shutdown, if no DA_PROFILE_MORE_THREAD_SAFE
  // 1) storage is not null
  // 2) we sleep
  // 3) destroying thread actually set tls_storage to nullptr, than checks depth (and it is 0). storage is destroyed
  // 4) we wake up and write to destroyed storage
  return start_cpu_event(description, *storage);
}

__forceinline void ProfilerData::endEvent(EventData &event, ThreadStorage &storage)
{
  const uint64_t end = cpu_current_ticks(); // as early as possible, to make more accurate measurement of actual event
  u64_interlocked_relaxed_store(event.end, end);
  if (DAGOR_UNLIKELY(storage.depth == 1)) // root event
  {
    const uint64_t freeTick = safe_first_needed_tick();
    if (DAGOR_UNLIKELY(storage.nextFreeEventTick < freeTick)) // last event
      storage.freeEventChunks(freeTick, end);
  }
  more_safe_dec(storage.depth);                             // only now decrease depth
  PLATFORM_EVENT_STOP(get_active_mode() & PLATFORM_EVENTS); // per platform event start
}

// just as strncpy, but doesn't pad with zeroes whole destination
static inline char *simple_strncpy(char *to, const char *src, size_t s)
{
  for (const char *e = src + s; src != e; ++src, ++to)
  {
    *to = *src;
    if (*src == 0)
      return to;
  }
  return to;
}

__forceinline void ProfilerData::addTag(uint32_t description, const char *tag)
{
  SCOPED_THREAD_SAFETY;
  ThreadStorage *storage = interlocked_relaxed_load_ptr(tls_storage);
  if (DAGOR_UNLIKELY(!storage))
    return;
  auto &e = storage->stringTags.push_back();
  // use relaxed_store on 64 bit arch, to avoid tsan data-race reports
  e.description = description;
  simple_strncpy(e.str, tag, sizeof(e.str));
  const uint64_t end = cpu_current_ticks();
  u64_interlocked_relaxed_store(e.end, end);
  const uint64_t freeTick = safe_first_needed_tick();
  if (DAGOR_UNLIKELY(storage->nextFreeTagTick < freeTick)) // last event
    storage->freeTagChunks(freeTick, end);
}

__forceinline void ProfilerData::addTag(uint32_t description, const char *fmt, const TagSingleWordArgument *args, uint32_t ac)
{
  SCOPED_THREAD_SAFETY;
  ThreadStorage *storage = interlocked_relaxed_load_ptr(tls_storage);
  if (DAGOR_UNLIKELY(!storage))
    return;
  auto &e = storage->stringTags.push_back();
  // use relaxed_store on 64 bit arch, to avoid tsan data-race reports
  e.description = description;
  static constexpr size_t minCount = 4; // at least 4 arguments will be saved, even if string will be truncated
  uint32_t minAc = ac < minCount ? ac : minCount;
  const size_t minArgsSize = minAc * sizeof(TagSingleWordArgument);
  const size_t strSize = sizeof(e.str) - minArgsSize - 1;
  char *argsAt = simple_strncpy(e.str, fmt, strSize);
  *argsAt = 0;
  argsAt++;
  size_t argsLeft = (sizeof(e.str) - (argsAt - e.str)) / sizeof(TagSingleWordArgument); // data left
  memcpy(argsAt, args, (argsLeft < ac ? argsLeft : ac) * sizeof(TagSingleWordArgument));

  const uint64_t end = cpu_current_ticks();
  u64_interlocked_relaxed_store(e.end, end);
  const uint64_t freeTick = safe_first_needed_tick();
  if (DAGOR_UNLIKELY(storage->nextFreeTagTick < freeTick)) // last event
    storage->freeTagChunks(freeTick, end);
}


GpuEventData *ProfilerData::startGPUEvent(uint32_t description, const char *d3d_event_name)
{
  // gpu only event
  if (!isFrameThread()) // for simplicity, profile only main thread. It wouldn't matter, except requirement for free_event_chunks in
                        // endGPUEvent
    return nullptr;
  // since we are in main thread anyway
  ThreadStorage *storage = interlocked_relaxed_load_ptr(tls_storage);
  if (DAGOR_UNLIKELY(!storage))
    return nullptr;
  const uint32_t currentDepth = storage->gpuDepth++;
  (void)(d3d_event_name);
#if ISSUE_D3D_EVENTS
  if (DAGOR_UNLIKELY(!d3d_event_name))
    d3d_event_name = descriptions.get(description);
  gpu_profiler::begin_event(d3d_event_name);
#endif
  auto &e = storage->gpuEvents.push_back();
  e.start = e.end = ~0ULL;
  e.description = description;
  e.depth = currentDepth;
  gpu_profiler::start_ds(e.ds);
  gpu_profiler::insert_time_stamp(&e.start); // this is only safe, because we never free data which isnt finished (i.e. as long as end
                                             // is ~0ull)
  return &e;
}

__forceinline void ProfilerData::endGPUEvent(GpuEventData &e)
{
#if ISSUE_D3D_EVENTS
  gpu_profiler::end_event();
#endif
  ThreadStorage *storage = interlocked_relaxed_load_ptr(tls_storage);
  if (storage)
  {
    gpu_profiler::stop_ds(e.ds);
    gpu_profiler::insert_time_stamp(&e.end);
    storage->gpuDepth--;
  }
}

void ProfilerData::shutdown(bool pre_destructor)
{
  ThreadStorage *storage = interlocked_acquire_load_ptr(tls_storage);
  if (storage && storage->depth > 0 && !isFrameThread())
  {
    report_logerr("we can't shutdown from inside scoped event!");
    return;
  }
  if (pre_destructor)
    shutdown_dump_server();
  unregisterPlugins();
  // should be called only from same thread as addFrame
  // disable all profiling first
  u64_interlocked_release_store(firstNeededTick, 0); // avoid freeing memory
  syncStopSampling();                                // first let's stop current unwinding
  stopStackSampling();
  uint64_t memAllocated = 0, memGpuAllocated = 0;
  interlocked_release_store(active_mode, 0);
  settings.setMode(0);

  gpu_profiler::shutdown();

  u64_interlocked_release_store(frameThreadId, 0);
  if (storage)
  {
    frame_thread = false;
    storage->clear();
  }
  {
    std::lock_guard<std::mutex> lock(threadsLock);
    auto thread_is_running = [](int64_t /*tid*/) {
      return true; // fixme! to be implemented!
    };
    for (auto &thread : threadsData)
    {
      if (!thread || !thread->tlsStorage)
        continue;
      if (pre_destructor)
      {
        // we can't do that if we are in destructor. Threads can be _terminated_ suddenly, without notice
        if (thread_is_running(thread->storage.threadId))
          interlocked_release_store_ptr(*thread->tlsStorage, (ThreadStorage *)nullptr); // ensure no events will start any more
        memAllocated += thread->storage.memAllocated();
        memGpuAllocated += thread->storage.gpuEvents.memAllocated();
      }
    }

#if !DA_PROFILE_MORE_THREAD_SAFE
    sleep_msec(1); // wait a bit, so we wont start event between tls_storage checked for nullptr and depth increased. poor man's data
                   // race protection
#endif
    for (int i = 0; i < 16; ++i) // 16 attempts to free all threads
    {
      if (is_safe_to_shutdown())
      {
        auto ti = find_if(threadsData.begin(), threadsData.end(), [&](auto &t) {
          return bool(t) && thread_is_running(t->storage.threadId) && interlocked_acquire_load(t->storage.depth) != 0;
        });
        if (ti == threadsData.end()) // no active threads left
          break;
      }
      if (pre_destructor)
        report_debug("wait another %d msec to shutdown", i);
      sleep_msec(i + 1); // wait a bit for other threads
    }

    for (auto &thread : threadsData) // prevent threads to be used/freed after this free.
    {
      if (!thread || !thread->tlsStorage || interlocked_acquire_load(thread->storage.depth) != 0)
        continue;
      thread->tlsStorage = nullptr; // as we have shut down anyway
    }
  }

  frameEvent = NULL;
  frameGPUEvent = NULL;


  {
    if (pre_destructor)
      memAllocated += descriptions.memAllocated();
    // do not free memory, makes safer to continue.
    // descriptions.storage.clear();
    // uniqueNames.reset();
  }
  if (pre_destructor)
    report_debug("profiler used %gkb (%gKb gpu) infinite frames %gKb, ringBuf %gKb, call Stacks %gKb, symbols %gKb",
      memAllocated / 1024.0, memGpuAllocated / 1024.0, infiniteFrames.memAllocated() / 1024.0,
      framesInfo.frames.memAllocated() / 1024.0, stackSamples.memAllocated() / 1024.0, symbolsCache.memAllocated() / 1024.0);
}

bool ProfilerData::profileCpuSpike(uint64_t frame_ticks, uint32_t save_first, uint32_t save_count)
{
  // check if it is spiked frame
  const uint32_t frameCount = framesInfo.activeFramesCount();
  const uint64_t avg = cpuFrameMovingAverageTick / frameCount; // count
  // const uint64_t spikeThreshold = (avg*cpuSpikesMulAverage) + cpuSpikesAddTicks;//3 average + fixed threshold
  // todo: we can actually use running average for variance
  // from https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Welford's_online_algorithm
  // ofc, it would requires slight modifications, but is generally faster
  double variance = 0;
  framesInfo.frames.forEach([&](const FrameInfo &frame) {
    const double dif = frame.duration() - int64_t(avg);
    variance += dif * dif;
  });

  const uint64_t std = sqrt(variance / (frameCount - 1));
  // the spike is a spike only if exceeds 3*average + max(3 sigma, threshold)
  if (DAGOR_LIKELY(frame_ticks < (avg * cpuSpikesMulAverage) + max(cpuSpikesAddTicks, 3 * std)))
    return false;
  unique_ptr<Dump> dump = make_unique<Dump>(Dump::Type::Spike, true);
  for (uint32_t f = save_first, e = f + save_count; f != e; ++f)
    dump->frames.push_back(framesInfo[f]);
  prepareDump(move(dump));
  // send it to saver thread then
  return true;
}

struct PauseSampling
{
  volatile uint64_t &id;
  PauseSampling(volatile uint64_t &i, uint64_t to) : id(i) { u64_interlocked_relaxed_store(id, to); }
  ~PauseSampling() { u64_interlocked_relaxed_store(id, 0); }
};

void ProfilerData::setSampling(uint32_t sampling_rate_freq, uint32_t sampling_spike_rate_freq, uint32_t threads_sampling_rate_mul)
{
  if (sampling_rate_freq > 0)
  {
    sampling_spike_rate_freq = max(sampling_spike_rate_freq, sampling_rate_freq);
    const uint32_t normal_period_us = clamp(uint32_t(1000000 / sampling_rate_freq), 1u, 1000000u); // we limit sleep to 1000msec, once
                                                                                                   // per second. actuall sleep is
                                                                                                   // always <100msec
    const uint32_t spike_period_us = clamp(uint32_t(1000000 / sampling_spike_rate_freq), 1u, 1000000u); // we limit sleep to 1000msec,
                                                                                                        // once per second. actuall
                                                                                                        // sleep is always <100msec
    interlocked_relaxed_store(samplingPeriodUs, normal_period_us); // immediately set current period
    interlocked_relaxed_store(spikeSamplingPeriodUs, spike_period_us);
    interlocked_relaxed_store(threadsSamplingRate, threads_sampling_rate_mul);
  }
}

void ProfilerData::setSpikes(uint32_t min_msec, uint32_t average_mul, uint32_t add_msec)
{
  u64_interlocked_relaxed_store(cpuSpikesThresholdTicks, min_msec * cpu_frequency() / 1000);
  u64_interlocked_relaxed_store(cpuSpikesAddTicks, add_msec * cpu_frequency() / 1000);
  interlocked_relaxed_store(cpuSpikesMulAverage, average_mul);
}

void ProfilerData::addFrame() // can only be called from one frame, and only from one frame
{
  const uint64_t ticks = cpu_current_ticks(); // as early as possible to not measure profiler overhead

// move me out
#if DA_PROFILE_NET_IS_POSSIBLE
  if (settings.generation != sentSettingsGen)
  {
    sentSettingsGen = settings.generation;
    send_settings(settings);
  }
  if (pluginsSent != interlocked_relaxed_load(pluginGeneration))
  {
    pluginsSent = interlocked_acquire_load(pluginGeneration);
    sendPlugins();
  }
#endif

  uint32_t newMode = settings.getMode(); // fixme: GPU should be also part of next_mode
  if (!(newMode & EVENTS))
    newMode = 0;
  const int oldMode = interlocked_relaxed_load(active_mode); // this is changed only here and in shutdown
  if (!newMode && !oldMode)
  {
    if (interlocked_relaxed_load(samplingState)) // we could pauseSampling, then and(settings.mode, 0), and thread would continue to
                                                 // run
      stopStackSampling();
    return;
  }

  ThreadStorage *storage = interlocked_relaxed_load_ptr(tls_storage);
  if (!storage || !frame_thread)
    storage = initializeFrameThread();

  if (!storage || !isFrameThread()) // use thread local bool - 'frame_thread'
  {
    //! isFrameThread() can be replaced with mutex for addFrame
    report_logerr("addFrame should be called from only one thread. otherwise gpu frames will become broken (can be fixed)");
    return;
  }
  if (DAGOR_UNLIKELY(storage->depth > (frameEvent ? 1 : 0)))
  {
    // we are inside other frame event. Do not call spike profiling, and save this info
    if ((oldMode & GPU))
      gpu_profiler::finish_queries();
    if (currentCpuSpikesThresholdTicks != 0)
    {
      int64_t ticksPassdFromFrameStart = (ticks - frameEvent->start);
      const uint64_t spikeSamplingTick = u64_interlocked_relaxed_load(startSpikeSamplingTick);
      if (spikeSamplingTick < ticks)
        u64_interlocked_relaxed_store(startSpikeSamplingTick, spikeSamplingTick + ticksPassdFromFrameStart); // increase spike sampling
                                                                                                             // threshold, as frame has
                                                                                                             // passed
    }
    if (oldMode && framesInfo.frameCount)
      framesInfo.frames[framesInfo.curFrameIndex()].addFrameCount++;
    return;
  }

  PauseSampling noSampleAddFrame(pauseSamplingThreadId, frameThreadId);
  // closing old frame
  uint64_t ringFrameTicksTaken = 0;

  FrameInfo *lastFrame = NULL;
  if (oldMode)
  {
    if (framesInfo.frameCount)
    {
      const uint32_t cfi = framesInfo.curFrameIndex();
      lastFrame = &framesInfo.frames[cfi];
      ringFrameTicksTaken = ticks - lastFrame->start;
      G_ASSERT(frameEvent);
    }
    if (lastFrame)
      lastFrame->end = ticks; // finish previous frame
    if (frameGPUEvent)
      endGPUEvent(*frameGPUEvent);

    // if (lastFrame && (oldMode&GPU))
    //   gpu_profiler::insert_time_stamp(&lastFrame->gpuEnd);//this is only safe, because we never free data which isnt finished (i.e.
    //   as long as end is ~0ull)

    if (frameEvent) // manual end of event - for performance and precision. could be just endEvent(*frameEvent);
    {
      frameEvent->end = ticks;
      more_safe_dec(storage->depth);
      G_ASSERTF(storage->depth == 0, "%d", storage->depth);
    }
    interlocked_relaxed_store_ptr(tls_storage, (ThreadStorage *)nullptr); // avoid storing events during work of profiler
    frameEvent = NULL;
    frameGPUEvent = NULL;
#if DA_PROFILE_NET_IS_POSSIBLE
    size_t infCount = infiniteFrames.approximateSize();
    report_frame_time(ringFrameTicksTaken, infCount ? infCount + 1 : framesInfo.activeFramesCount());
#endif
  }
  cpuFrameMovingAverageTick += ringFrameTicksTaken; // we add new ticks to sum

  if (lastFrame && (oldMode & CONTINUOUS)) // copy profiled frames
  {
    if (infiniteFrames.empty())
      lastCopiedGpuInfiniteFrameId = 0;
    infiniteFrames.push_back(*lastFrame);
  }


  if ((newMode & GPU) && !(oldMode & GPU))
  {
    if (!gpu_profiler::init())
    {
      newMode &= ~GPU;
      settings.removeMode(GPU);
    }
  }

  if (oldMode != newMode)
  {
    interlocked_release_store(active_mode, newMode);
    // or something like
    // interlocked_and(active_mode, ~newMode);interlocked_or(active_mode, newMode);
  }

  startStackSampling(newMode & SAMPLING);

  closeRemovedThreads();

  if ((oldMode & GPU))
    gpu_profiler::finish_queries();

  const size_t dumpSize = saveDump(interlocked_relaxed_load(next_dump_requested), isContinuousLimitReached());
  if (dumpSize && (newMode & CONTINUOUS))
  {
    const uint32_t limitMb = interlocked_relaxed_load(settings.continuousLimitSizeMb);
    currentContinuousDumpSize += dumpSize;
    if ((currentContinuousDumpSize >> 20) > limitMb && limitMb)
    {
      remove_mode(CONTINUOUS);
      report_debug("stops profiling due to DumpSize %d being bigger than %dmb", int(currentContinuousDumpSize >> 20),
        int(interlocked_relaxed_load(settings.continuousLimitSizeMb)));
    }
  }
  if (oldMode && storage->shouldShrink())
    shrinkMemory();

  if ((oldMode & CONTINUOUS) && !(newMode & CONTINUOUS))
    currentContinuousDumpSize = 0;
  if (!(newMode & EVENTS))
  {
    interlocked_relaxed_store_ptr(tls_storage, storage);
    return;
  }
  const uint32_t nfi = framesInfo.nextFrameIndex();

  if (DAGOR_UNLIKELY(framesInfo.frameCount < framesInfo.num_frames))
    framesInfo.frames.push_back();
  FrameInfo &newFrame = framesInfo.frames[nfi];
  const bool overwrittenRingFrame = framesInfo.frameCount >= framesInfo.num_frames;
  const int64_t overwrittenFrameTicksTaken = (overwrittenRingFrame ? newFrame.duration() : 0LL);
  if (overwrittenRingFrame) // frame is going out of window, so we reduce moving average
    cpuFrameMovingAverageTick -= max(0ll, (long long int)overwrittenFrameTicksTaken); // reducing moving average by frame going out of
                                                                                      // history window

  // no sense in saving spikes if we have infinite profiling
  if (framesInfo.activeFramesCount() > 0)
    currentCpuSpikesThresholdTicks = max(cpuSpikesThresholdTicks,
      (cpuFrameMovingAverageTick * cpuSpikesMulAverage) / framesInfo.activeFramesCount() + cpuSpikesAddTicks);
  else
    currentCpuSpikesThresholdTicks = 0;

  if (((newMode & (SAVE_SPIKES | CONTINUOUS)) == SAVE_SPIKES) && currentCpuSpikesThresholdTicks != 0 && overwrittenRingFrame &&
      framesInfo.frameCount >= nextSpikeFrameCheck)
  {
    const uint32_t framesBefore = min(framesInfo.num_frames - 4, interlocked_relaxed_load(settings.framesBeforeSpike)),
                   saveFramesCount =
                     min(framesBefore + interlocked_relaxed_load(settings.framesAfterSpike) + 1, framesInfo.activeFramesCount());

    const uint32_t checkFrame = nfi + framesBefore;
    const auto duration = framesInfo[checkFrame].duration();
    if (duration > currentCpuSpikesThresholdTicks && framesInfo[checkFrame].addFrameCount == 0 &&
        profileCpuSpike(duration, nfi, saveFramesCount))
      nextSpikeFrameCheck = framesInfo.frameCount + saveFramesCount; // all this frames are already saved
  }

  const uint64_t overwrittenFrameCpuStart = overwrittenRingFrame ? newFrame.start : 0;
  const uint64_t overwrittenFrameGpuStart = overwrittenRingFrame ? newFrame.gpuStart : 0;
  uint64_t nextFirstNeededTick = overwrittenFrameCpuStart;
  uint64_t nextFirstNeededGpuTick = overwrittenFrameGpuStart;

  if ((oldMode & CONTINUOUS) && infiniteFrames.approximateSize() >= framesInfo.activeFramesCount())
  {
    auto &firstInf = infiniteFrames.front();
    nextFirstNeededTick = firstInf.start;
    nextFirstNeededGpuTick = firstInf.gpuStart;
  }

  u64_interlocked_release_store(firstNeededTick, nextFirstNeededTick);

  if (nextFirstNeededTick != 0)
  {
    storage->freeEventChunks(nextFirstNeededTick, ticks); // to reduce it's happening during active frame
    storage->freeTagChunks(nextFirstNeededTick, ticks);   // to reduce it's happening during active frame
  }

  if (nextFirstNeededGpuTick != 0 && nextFirstNeededGpuTick != ~0ULL)
    free_event_chunks(storage->gpuEvents, nextFirstNeededGpuTick);

  if ((oldMode & CONTINUOUS) && overwrittenRingFrame && !infiniteFrames.empty())
  {
    if (lastCopiedGpuInfiniteFrameId >= infiniteFrames.approximateSize())
      lastCopiedGpuInfiniteFrameId = 0;
    auto &infFrame = infiniteFrames[lastCopiedGpuInfiniteFrameId];
    if (newFrame.frameNo == infFrame.frameNo)
    {
      infFrame.gpuStart = newFrame.gpuStart;
      // infFrame.gpuEnd = newFrame.gpuEnd;
      lastCopiedGpuInfiniteFrameId++;
    }
  }

  newFrame.start = ticks;
  newFrame.end = ~0ULL;
  newFrame.gpuStart = ~0ULL;
  // newFrame.gpuEnd = ~0ULL;
  newFrame.frameNo = framesInfo.frameCount;
  newFrame.addFrameCount = 0;
  framesInfo.frameCount++;

  // we should call flip once per frame, and now it is both from MP and our profiler
  gpu_profiler::flip(&newFrame.gpuStart);

  if (currentCpuSpikesThresholdTicks == 0)
    u64_interlocked_relaxed_store(startSpikeSamplingTick, 0); // set sampling threshold

  if (newMode & GPU)
    frameGPUEvent = startGPUEvent(descriptions.gpuFrame(), "frame");

  interlocked_relaxed_store_ptr(tls_storage, storage);
  frameEvent = start_cpu_event(descriptions.frame(), *storage); // put as far as possible, to avoid measuring profiler overhead
  if (currentCpuSpikesThresholdTicks != 0)
    u64_interlocked_relaxed_store(startSpikeSamplingTick, frameEvent->start + currentCpuSpikesThresholdTicks);
  newFrame.start = frameEvent->start;
}

DAGOR_NOINLINE
void ProfilerData::shrinkMemory()
{
  // we can always free memory in main thread
  // storage->shrink_to_fit();//reduce at least main frame.
  std::lock_guard<std::mutex> lock(threadsLock);
  for (auto &thread : threadsData)
    if (thread)
      thread->storage.shrink_to_fit();
}

ProfilerData the_profiler;

void add_short_string_tag(desc_id_t description, const char *string) { return the_profiler.addTag(description, string); }
void add_short_string_tag(desc_id_t description, const char *fmt, const TagSingleWordArgument *a, uint32_t ac)
{
  return the_profiler.addTag(description, fmt, a, ac);
}

EventData *start_event(desc_id_t description, ThreadStorage *&storage) { return the_profiler.startEvent(description, storage); }
void end_event(EventData &e, ThreadStorage &storage) { the_profiler.endEvent(e, storage); }
void create_leaf_event_raw(desc_id_t description, uint64_t start, uint64_t end) { the_profiler.addLeafEvent(description, start, end); }

GpuEventData *start_gpu_event(desc_id_t description, const char *d3d_event_name)
{
  return the_profiler.startGPUEvent(description, d3d_event_name);
}

void end_gpu_event(GpuEventData &e) { the_profiler.endGPUEvent(e); }

void tick_frame() { the_profiler.addFrame(); }
void shutdown() { the_profiler.shutdown(true); }

void set_sampling_parameters(uint32_t sampling_rate_freq, uint32_t sampling_spike_rate_freq, uint32_t threads_sampling_rate_mul)
{
  the_profiler.settings.setSampling(sampling_rate_freq, sampling_spike_rate_freq, threads_sampling_rate_mul);
  the_profiler.setSampling(sampling_rate_freq, sampling_spike_rate_freq, threads_sampling_rate_mul);
}

void get_sampling_parameters(uint32_t &sampling_rate_freq, uint32_t &sampling_spike_rate_freq, uint32_t &threads_sampling_rate_mul)
{
  sampling_rate_freq = interlocked_acquire_load(the_profiler.settings.samplingFreq);
  sampling_spike_rate_freq = interlocked_acquire_load(the_profiler.settings.spikeSamplingFreq);
  threads_sampling_rate_mul = interlocked_acquire_load(the_profiler.settings.threadsSamplingRate);
}

void add_mode(uint32_t mask) { the_profiler.settings.addMode(mask); }

void remove_mode(uint32_t mask) { the_profiler.settings.removeMode(mask); }

uint32_t get_current_mode() { return the_profiler.settings.getMode(); }
void set_mode(uint32_t new_mode) { the_profiler.settings.setMode(new_mode); }

void set_spike_parameters(uint32_t min_msec, uint32_t average_mul, uint32_t add_msec)
{
  the_profiler.settings.setSpikes(min_msec, average_mul, add_msec);
  the_profiler.setSpikes(min_msec, average_mul, add_msec);
}

void set_spike_save_parameters(uint32_t frames_before, uint32_t frames_after)
{
  the_profiler.settings.setSpikeSave(frames_before, frames_after);
}


void get_spike_parameters(uint32_t &min_msec, uint32_t &average_mul, uint32_t &add_msec)
{
  min_msec = interlocked_acquire_load(the_profiler.settings.spikeMinMsec);
  average_mul = interlocked_acquire_load(the_profiler.settings.spikeAvgMul);
  add_msec = interlocked_acquire_load(the_profiler.settings.spikeAddMsec);
}

void get_spike_save_parameters(uint32_t &frames_before, uint32_t &frames_after)
{
  frames_before = interlocked_acquire_load(the_profiler.settings.framesBeforeSpike);
  frames_after = interlocked_acquire_load(the_profiler.settings.framesAfterSpike);
}

void set_continuous_limits(uint32_t frames, uint32_t size_mb) { the_profiler.settings.setContinuousLimits(frames, size_mb); }

void sync_stop_sampling() { the_profiler.syncStopSampling(); }

void pause_sampling() { the_profiler.pauseSampling(); }
void resume_sampling() { the_profiler.resumeSampling(); }
void request_dump() { interlocked_increment(next_dump_requested); }

}; // namespace da_profiler
