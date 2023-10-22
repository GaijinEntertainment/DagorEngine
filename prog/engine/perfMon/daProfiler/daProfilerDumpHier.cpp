#include "daGpuProfiler.h"
#include "daProfilerInternal.h"
#include "daProfilePlatform.h"
#include <perfMon/dag_statDrv.h>

namespace da_profiler
{
static inline void add_stat(DrawStatSingle &, const EventData &) {}
static inline void add_stat(DrawStatSingle &stat, const GpuEventData &gs)
{
  for (int i = 0; i < DRAWSTAT_NUM; ++i)
    stat.val[i] += gs.ds.val[i];
  stat.tri += gs.ds.tri;
}

static inline void fill_stat(const EventDataStorage &, TimeIntervalInfo &stat, double avgTime, double minTime, double maxTime,
  double self)
{
  stat.tCurrent = avgTime;
  stat.tMin = minTime;
  stat.tSpike = stat.tMax = maxTime;
  stat.unknownTimeMsec = self;
  stat.gpuValid = false;
}

static inline void fill_stat(const GpuEventDataStorage &, TimeIntervalInfo &stat, double avgTime, double minTime, double maxTime,
  double self)
{
  stat.tGpuCurrent = avgTime;
  stat.tGpuMin = minTime;
  stat.tGpuMax = stat.tGpuSpike = maxTime;
  stat.unknownTimeMsec = self;
  stat.gpuValid = true;
}

template <class Storage>
inline void dumpHierEvents(const ProfilerData &pd, uint32_t stream_description, const Storage &events, ProfilerDumpFunctionPtr cb,
  void *ctx, uint64_t startDumpTicks, uint64_t endDumpTicks, uint64_t maxTicks, uint32_t totalFrames, uint64_t freq)
{
  struct HierNode
  {
    uint32_t parentNodeHierId;
    uint32_t descriptionId;
    uint64_t ticks;
    uint64_t childTicks;
    uint64_t uniqueId;
    int depth;
    uint32_t calls;
    uint64_t minTicks, maxTicks;
    DrawStatSingle stat;
  };
  HashedKeyMap<uint64_t, int> uniqueIdToNodeId;
  SmallTab<HierNode> hierNodes;

  struct StackEvent
  {
    uint32_t hierId;
    uint64_t hierUniqueId;
    uint64_t end;
  };
  const uint32_t threadRootHierId = hierNodes.size();
  const uint64_t threadRootUniqueId = wyhash64(stream_description, ~0u);
  uniqueIdToNodeId.emplace(threadRootUniqueId, threadRootHierId);
  // const uint64_t frameEndTicks = (frame.end == ~0ULL ? curTicks : frame.end);
  // hierNodes[0].ticks = frameEndTicks - frame.start;
  hierNodes.push_back(HierNode{~0u, stream_description, 0ULL, 0ULL, threadRootUniqueId, -1, 1, 0, 0});

  SmallTab<StackEvent> stack;
  stack.push_back(StackEvent{threadRootHierId, threadRootUniqueId, ~0ULL});

  events.forEach([&](const typename Storage::value_t &event) {
    if (event.end < startDumpTicks || event.start >= endDumpTicks)
      return;
    const uint64_t endTicks = event.end == ~0ULL ? maxTicks : event.end;
    const uint64_t ticksTaken = endTicks - event.start;
    const int depth = event.depth;
    // debug("%s: start %@ end %@, dur %@ depth %@",
    //   descriptions.storage[event.description].name, event.start, event.end, ticksTaken, depth);
    if (stack.size() > depth + 1) // pop stack
      stack.resize(depth + 1);
    else if (stack.size() < depth + 1) // not finished stack
      return;

    const uint32_t hierParentId = stack.back().hierId;
    const uint64_t hierParentUniqueId = stack.back().hierUniqueId;
    const uint64_t hierUniqueId = wyhash64(event.description, hierParentUniqueId);
    auto emplRet = uniqueIdToNodeId.emplace_if_missing(hierUniqueId);
    if (emplRet.second)
    {
      *emplRet.first = hierNodes.size();
      hierNodes.push_back(HierNode{hierParentId, event.description, ticksTaken, 0ULL, hierUniqueId, depth, 0, ticksTaken, ticksTaken});
    }
    const uint32_t hierId = uint32_t(*emplRet.first);
    auto &h = hierNodes[hierId];
    hierNodes[hierParentId].childTicks += ticksTaken;
    /*auto &hp = hierNodes[hierParentId];
    const char *name = h.descriptionId != ~0u ? descriptions.storage[h.descriptionId].name : "!!";
    const char *parentName = hp.descriptionId != ~0u ? descriptions.storage[hp.descriptionId].name : "!!";
    debug("%s, depth=%d parent = %s %dus (child %dus), parent %dus", name, depth, parentName,
      h.ticks*1000000/freq,
      h.childTicks*1000000/freq,
      hierNodes[hierParentId].childTicks*1000000/freq
    );*/
    h.ticks += ticksTaken;
    h.minTicks = min(h.minTicks, ticksTaken);
    h.maxTicks = max(h.maxTicks, ticksTaken);
    add_stat(h.stat, event);
    h.calls++;
    stack.push_back(StackEvent{hierId, hierUniqueId, endTicks});
  });
  hierNodes[threadRootHierId].ticks = hierNodes[threadRootHierId].childTicks;
  const uint32_t descCounts = pd.descriptions.size();
  const double ticks_to_msec = 1000. / freq;

  vector<vector<uint32_t>> tree;
  tree.resize(hierNodes.size());
  for (uint32_t i = 1, e = hierNodes.size(); i < e; ++i)
  {
    const auto &h = hierNodes[i];
    tree[h.parentNodeHierId].push_back(i);
  }

  function<void(uint32_t)> iterateTree = [&](uint32_t hi) {
    const auto &h = hierNodes[hi];
    if (h.calls)
    {
      TimeIntervalInfo stat;
      {
        for (int i = 0; i < DRAWSTAT_NUM; ++i)
          stat.stat.val[i] = h.stat.val[i] / h.calls;
        stat.stat.tri = h.stat.tri / h.calls;
      }
      stat.depth = h.depth + 1;
      const double avgTimeMsec = (h.ticks * ticks_to_msec) / h.calls, minTimeMsec = h.minTicks * ticks_to_msec,
                   maxTimeMsec = h.maxTicks * ticks_to_msec,
                   selfTimeMsec = max(int64_t(0), int64_t(h.ticks - h.childTicks)) * ticks_to_msec / h.calls;

      fill_stat(events, stat, avgTimeMsec, minTimeMsec, maxTimeMsec, selfTimeMsec);
      stat.lifetimeCalls = h.calls;
      stat.lifetimeFrames = h.calls;
      stat.perFrameCalls = h.calls;
      stat.skippedFrames = max(0, (int)totalFrames - (int)h.calls); // not accurate, as we should analyze frames
      const char *name = h.descriptionId < descCounts ? pd.descriptions.storage[h.descriptionId].name : "!!";
      // debug("hier: %s:%d: unique 0x%X parent 0x%X calls %d skip %d",
      //   name, stat.depth, h.uniqueId, h.parentNodeHierId == ~0u ? 0 : hierNodes[h.parentNodeHierId].uniqueId,
      //   h.calls, stat.skippedFrames);
      cb(ctx, h.uniqueId, (h.parentNodeHierId == ~0u ? 0 : hierNodes[h.parentNodeHierId].uniqueId), name, stat);
    }
    for (auto ti : tree[hi])
      iterateTree(ti);
  };
  iterateTree(0);
}

void dumpHierFrames(const ProfilerData &pd, ProfilerDumpFunctionPtr cb, void *ctx)
{
  if (u64_interlocked_acquire_load(pd.frameThreadId) == 0) // no main thread
    return;
  if (!pd.isFrameThread())
  {
    report_logerr("dump can be called from only one thread");
    return;
  }
  if (pd.framesInfo.frameCount <= 1) // no frames, 1 because we 'dump' inside frame, and last one is incorrect
    return;
  // temporary stop profiling now, globally. This is not to prevent races, but just generally useless to sample such data.
  // lazily stop sampling
  // set ticks to release to 0
  const int oldMode = interlocked_acquire_load(active_mode);
  interlocked_release_store(active_mode, 0);
  u64_interlocked_release_store(pd.firstNeededTick, 0);

  const uint64_t curTicks = cpu_current_ticks();
  const uint32_t endFrame = pd.framesInfo.curFrameIndex();
  const uint32_t totalFrames = pd.framesInfo.activeFramesCount() - 1; //-1 because we 'dump' inside frame, and last one is incorrect
  const uint32_t startFrame = pd.framesInfo.startFrame();
  const uint64_t startDumpTicks = pd.framesInfo.frames[startFrame].start;
  const uint64_t endDumpTicks = pd.framesInfo.frames[endFrame].start; // as long as we dump _not_ from tick itself, latest frame isn't
                                                                      // anything reasonable
  const uint64_t startGpuDumpTicks = pd.framesInfo.frames[startFrame].gpuStart;
  const uint64_t gpuFreq = gpu_frequency();
  const uint64_t cpuFreq = cpu_frequency();
  const uint64_t endGpuDumpTicks = startGpuDumpTicks + ((endDumpTicks - startDumpTicks) * gpuFreq) / cpuFreq;
  const uint64_t curGpuTicks = startGpuDumpTicks + ((curTicks - startDumpTicks) * gpuFreq) / cpuFreq;
  // uint64_t memCPUAllocated = 0, memGPUAllocated = 0;
  {
    std::lock_guard<std::mutex> lock(pd.threadsLock);

    for (auto &thread : pd.threadsData)
    {
      if (!thread->storage.events.empty())
        dumpHierEvents(pd, thread->storage.description, thread->storage.events, cb, ctx, startDumpTicks, endDumpTicks, curTicks,
          totalFrames, cpuFreq);
      // combine all gpu Events to one, if we have multi-threaded gpu
      if (startGpuDumpTicks != ~0ULL && thread->storage.threadId == pd.frameThreadId && !thread->storage.gpuEvents.empty())
        dumpHierEvents(pd, pd.descriptions.gpu(), thread->storage.gpuEvents, cb, ctx, startGpuDumpTicks, endGpuDumpTicks, curGpuTicks,
          totalFrames, gpuFreq);
    }
  }
  interlocked_release_store(active_mode, oldMode);
}


void dumpHierFrames(const ProfilerData &pd, ProfilerDumpFunctionPtr cb, void *ctx);
void dump_frames(ProfilerDumpFunctionPtr cb, void *ctx) { dumpHierFrames(the_profiler, cb, ctx); }

}; // namespace da_profiler
