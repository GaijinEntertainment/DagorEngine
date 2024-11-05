// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daProfilerInternal.h"
#include "stl/daProfilerStl.h"
#include "daProfilePlatform.h"
#include "daProfilerSamplingUtils.h"
#include "daProfilerDumpServer.h"

// todo: copies are better to go from tail to head, than the opposite
// it prevents negative effect from race in free events (all newer chunks disappearing in dump)

namespace da_profiler
{

void ProfilerData::copyCpuEvents(const uint64_t timeStart, const uint64_t timeEnd, vector<Dump::Thread> &cpu) const
{
  // thread cpu events
  // requires threadsData lock
  cpu.resize(threadsData.size());
  for (size_t i = 0, e = threadsData.size(); i < e; ++i)
  {
    if (!threadsData[i])
      continue;
    auto &thread = threadsData[i];
    auto &saveTo = cpu[i];
    saveTo.threadId = thread->storage.threadId;
    saveTo.description = thread->storage.description;
    saveTo.addedTick = thread->addedTick;
    saveTo.removedTick = thread->removedTick;

    thread->storage.events.forEachChunkStoppable([&](const EventData *ebegin, const EventData *eend) {
      if (ebegin >= eend || eend[-1].start < timeStart) // chunk is not yet in scope
        return false;

      if (ebegin->start > timeEnd) // chunk is too fat out of scope
        return true;

      for (; eend > ebegin; --eend) // can be made binary search, but not sure if it is worth it
        if (eend[-1].start <= timeEnd)
          break;

      for (; ebegin < eend; ++ebegin) // can be made binary search, but not sure if it is worth it
        if (ebegin->start >= timeStart)
          break;

      if (saveTo.events.empty()) // start with root
        for (; ebegin < eend; ++ebegin)
          if (ebegin->depth == 0)
            break;
      saveTo.events.append(ebegin, eend);
      return false;
    });

    thread->storage.stringTags.forEachChunkStoppable([&](const StringTag *begin, const StringTag *end) {
      if (begin >= end || end[-1].end < timeStart) // continue to next chunk
        return false;
      if (begin->end > timeEnd) // too far
        return true;

      for (; begin != end; ++begin) // can be made binary search, but not sure if it is worth it
        if (begin->end >= timeStart)
          break;
      for (; end != begin; --end)   // can be made binary search, but not sure if it is worth it
        if (end[-1].end <= timeEnd) // chunk is after time end
          break;
      saveTo.stringTags.append(begin, end);
      return false;
    });
  }
}

void ProfilerData::copyGpuEvents(const uint64_t timeCpuStart, const uint64_t timeCpuEnd, GpuEventDataStorage &saveTo,
  ClockCalibration &cpu_gpu_clock) const
{
  // thread gpu events
  int64_t timeStart = 0, timeEnd = ~0ULL;
  int gpuThreadsCount = 0;
  for (size_t i = 0, e = threadsData.size(); i < e; ++i)
  {
    if (!threadsData[i])
      continue;
    auto &thread = threadsData[i];

    if (thread->storage.gpuEvents.empty())
      continue;
    if (gpuThreadsCount == 0)
    {
      // todo: calibrate clock not each dump
      int64_t cpuRef, gpuRef;
      if (!gpu_cpu_ticks_ref(cpuRef, gpuRef)) // can't calibrate clock
      {
        report_logerr("can't calibrate gpu clock");
        return;
      }
      cpu_gpu_clock = ClockCalibration{cpuRef, cpu_frequency(), gpuRef, gpu_frequency()};
      timeStart = cpu_gpu_clock.from1to2(timeCpuStart);
      timeEnd = cpu_gpu_clock.from1to2(timeCpuEnd);
    }
    gpuThreadsCount++;
    thread->storage.gpuEvents.forEachChunk([&](const GpuEventData *ebegin, const GpuEventData *eend) {
      if (eend <= ebegin)
        return;
      if (ebegin->start > timeEnd) // first event in chunk hand't even started or started later than latest possible time
        return;

      for (; eend > ebegin; --eend) // can be made binary search, but not sure if it is worth it
        if (eend[-1].start <= timeEnd)
          break;

      for (; ebegin < eend; ++ebegin) // can be made binary search, but not sure if it is worth it
        if (ebegin->start >= timeStart)
          break;

      if (saveTo.empty()) // start with root
        for (; ebegin < eend; ++ebegin)
          if (ebegin->depth == 0)
            break;

      saveTo.append(ebegin, eend);
    });
  }
}

void ProfilerData::copyCallStacks(uint64_t timeStart, uint64_t timeEnd, CallStackDumpStorage &save) const
{
  if (stackSamples.empty())
    return;
  // call stacks
  // simple reverse is good enough, as we sort in tool
  stackSamples.forEachChunk([&](const uint16_t *begin, const uint16_t *end) {
    if (end - begin < sampling_allocated_additional_words)
      return;
    const uint64_t lastTicks = sampling_end_ticks(end);
    if (lastTicks < timeStart) // last CS is sampled earlier than needed
      return;
    if (sampling_saved_ticks(begin) > timeEnd) // first CS is sampled later than needed
      return;
    auto i = begin;
    // we can actually skip samples at the end of chunk (but not at the begining, as we rely on state)
    for (; i < end;)
    {
      const uint32_t csCount = sampling_saved_cs_count(i);
      if (sampling_saved_ticks(i) > timeEnd)
        break;
      i += sampling_allocated_words(csCount);
    }
    save.insert(save.end(), begin, i);
  });
}

size_t ProfilerData::prepareDump(unique_ptr<Dump> &&dump_)
{
  if (!dump_ || dump_->frames.empty())
    return 0;
  Dump &dump = *dump_;
  u64_interlocked_release_store(firstNeededTick, 0); // prevent freeing events. would be set in next addFrame
  const uint64_t timeStart = dump.frames.front().start;
  const uint64_t timeEnd = dump.frames.back().end;
  dump.board = interlocked_increment(boardNumber);
  {
    std::lock_guard<std::mutex> lock(threadsLock);
    copyCpuEvents(timeStart, timeEnd, dump.threads);

    copyGpuEvents(timeStart, timeEnd, dump.gpuEvents, dump.cpuGpuClock);
  }

  uniqueEvents.forEachChunk([&](const auto *begin, const auto *end) { dump.uniqueEvents.append(begin, end); });
  dump.uniqueEventsFrames = uniqueEventsFrames;

  if (dump.type != Dump::Type::Spike)
    dump.stacks.reserve(stackSamples.approximateSize());
  copyCallStacks(timeStart, timeEnd, dump.stacks);
  dump.uniqueProfileRunName = uniqueProfileRunName;
  dump.frameThreadId = frameThreadId;

  const size_t dumpSize = dump_->memoryAllocated();
  add_dump(move(dump_));
  return dumpSize;
}

size_t ProfilerData::dumpRingProfile()
{
  // can be called only from mainThread, otherwise we need frame mutex, see below
  G_ASSERT_RETURN(isFrameThread(), false);
  // if we add mutex for frames/infinite frames, lock it here
  if (!framesInfo.frameCount) // no frames
    return 0;
  unique_ptr<Dump> dump = make_unique<Dump>(Dump::Type::Ring, combine_ring_captures);
  for (uint32_t sf = framesInfo.startFrame(), ef = sf + framesInfo.activeFramesCount(); sf != ef; ++sf)
    dump->frames.push_back(framesInfo.frames[sf & framesInfo.num_frames_mask]);
  // and unlock here
  return prepareDump(move(dump));
}

size_t ProfilerData::dumpContinuosProfile(bool append_to_last)
{
  // can be called only from mainThread, otherwise we need frame mutex, see below
  G_ASSERT_RETURN(isFrameThread(), false);
  // if we add mutex for frames/infinite frames, lock it here
  if (infiniteFrames.empty())
    return 0;
  unique_ptr<Dump> dump = make_unique<Dump>(Dump::Type::Continuous, append_to_last);
  dump->frames = (decltype(infiniteFrames) &&)infiniteFrames;

  // copy gpu frames times!
  uint32_t sf = framesInfo.startFrame(), ef = sf + framesInfo.activeFramesCount();
  const auto firstInfFrameNo = dump->frames.front().frameNo;
  while (sf != ef && firstInfFrameNo > framesInfo.frames[sf & framesInfo.num_frames_mask].frameNo)
    ++sf;

  if (sf != ef)
  {
    auto &firstRingFrame = framesInfo.frames[sf & framesInfo.num_frames_mask];
    auto &lastRingFrame = framesInfo.frames[(ef - 1) & framesInfo.num_frames_mask];
    dump->frames.forEachChunkStoppable([&](FrameInfo *fi, FrameInfo *fe) {
      if (fi >= fe)
        return false;
      if (fe[-1].gpuStart != ~0ULL) // already inited, continue;
        return false;
      const auto firstRingFrameNo = firstRingFrame.frameNo;
      if (fe[-1].frameNo < firstRingFrameNo) // nothing can we do, too early
        return false;
      if (fi->frameNo > lastRingFrame.frameNo) // stop iterating. We are already after our ring (how that could happen?)
        return true;
      while (fi != fe && fi->frameNo < firstRingFrameNo)
        ++fi;
      if (fi == fe)
        return false;
      for (; fi != fe && sf != ef; ++fi, ++sf)
        fi->gpuStart = framesInfo.frames[sf & framesInfo.num_frames_mask].gpuStart;
      return sf < ef ? false : true;
    });
  }
  // finish copying
  // and unlock here
  return prepareDump(move(dump));
}

}; // namespace da_profiler