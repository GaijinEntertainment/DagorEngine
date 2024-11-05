// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <perfMon/dag_drawStat.h>
#include <util/dag_uniqueHashedNames.h>
#include <mutex>
#include "daProfilePageAllocator.h"
#include "stl/daProfilerVector.h"
#include "dag_chunkedStorage.h"
#undef DA_PROFILER_ENABLED
#define DA_PROFILER_ENABLED 1
#include <perfMon/dag_daProfiler.h>


class IGenSave;
class IGenLoad;

namespace da_profiler
{
struct EventData;
struct GpuEventData;
struct EventDescription;
struct StringTag;
} // namespace da_profiler

DAG_DECLARE_RELOCATABLE(::da_profiler::StringTag);
DAG_DECLARE_RELOCATABLE(::da_profiler::EventData);
DAG_DECLARE_RELOCATABLE(::da_profiler::GpuEventData);

namespace da_profiler
{

struct EventData
{
  uint64_t start = ~0ULL;
  uint64_t end = ~0ULL;
  uint32_t description = ~0u;
  uint32_t depth = 0;
};

struct GpuEventData : EventData
{
  DrawStatSingle ds;
};

struct EventDescription
{
  const char *name = nullptr, *fileName = nullptr;
  int lineNo = 0;
  uint32_t colorAndFlags = 0u;
  uint32_t getColor() const { return colorAndFlags & 0xFFFFFF; }
  uint32_t getFlags() const { return colorAndFlags >> 24; }
};

struct FrameInfo
{
  uint64_t start = 0, end = 0, gpuStart = ~0ULL;
  uint64_t frameNo = 0;
  uint32_t addFrameCount = 0; // amount of addFrame called within a frame
  int64_t duration() const { return (int64_t)end - (int64_t)start; }
}; //, gpuEnd = ~0ULL

struct StringTag
{
  uint64_t end;
  uint16_t description;
  char str[54];
};

struct ClockCalibration
{
  ClockCalibration() = default;
  // ClockCalibration(uint64_t ref_from, uint64_t freq_from, uint64_t ref_to, uint64_t freq_to):
  ClockCalibration(int64_t ref1_, uint64_t freq1, int64_t ref2_, uint64_t freq2) :
    ref1(ref1_), ref2(ref2_), freq2_div_freq1(double(freq2) / double(freq1))
  {}
  int64_t ref1 = 0, ref2 = 0;
  double freq2_div_freq1 = 1.0;
  bool isInited() const { return ref1 != 0 || ref2 != 0; }
  int64_t from1to2(int64_t from) const { return ref2 + int64_t(int64_t(from - ref1) * freq2_div_freq1); }
  int64_t from2to1(int64_t from) const { return ref1 + int64_t(int64_t(from - ref2) / freq2_div_freq1); }
};

struct VirtualPageAllocator
{
  static constexpr size_t page_size = 65536;
  static void *alloc(size_t bytes) { return allocate_page(((bytes + page_size - 1) / page_size) * page_size, page_size); }
  static void free(void *p) { free_page(p, page_size); }
};

using StringTagDataStorage = ChunkedStorage<StringTag, 1, VirtualPageAllocator>;
using EventDataStorage = ChunkedStorage<EventData, 3, VirtualPageAllocator>;
using GpuEventDataStorage = ChunkedStorage<GpuEventData, 3, VirtualPageAllocator>;
using CallStackStorage = ChunkedStorage<uint16_t, 1, VirtualPageAllocator>;
using FramesStorage = ChunkedStorage<FrameInfo, 256, NoPageAllocator>;
using DescriptionStorage = ChunkedStorage<EventDescription, 1, VirtualPageAllocator>;

using CallStackDumpStorage = vector<uint16_t>; // todo: another allocator for dump storage as well!
// using CallStackDumpStorage = CallStackStorage;
struct Dump // full memory copy, saving spikes, etc
{
  FramesStorage frames;
  struct Thread
  {
    EventDataStorage events;
    StringTagDataStorage stringTags;
    uint64_t threadId = ~0ULL;
    uint64_t addedTick = 0, removedTick = ~0ULL;
    uint32_t description = 0;
  };
  vector<Thread> threads;
  const char *uniqueProfileRunName = nullptr;
  ChunkedStorage<UniqueEventData *, 1, VirtualPageAllocator> uniqueEvents;
  uint32_t uniqueEventsFrames = 0;
  GpuEventDataStorage gpuEvents;
  CallStackDumpStorage stacks;
  uint32_t board = 0;
  uint32_t dumpAtMs = 0;
  uint64_t dateTime = 0;
  uint64_t frameThreadId = 0;
  ClockCalibration cpuGpuClock;
  enum class Type
  {
    Ring,
    Continuous,
    Spike,
    Unknown
  } type;
  bool appendToCurrent = false;
  Dump(Type tp, bool append_to_current_if_exist);
  size_t memoryAllocated() const
  {
    size_t mem = stacks.capacity() * sizeof(uint64_t) + gpuEvents.memAllocated();
    for (auto &t : threads)
      mem += t.events.memAllocated() + t.stringTags.memAllocated();
    return mem;
  }
};

struct ProfilerDescriptions;
struct SymbolsCache;
struct UserSettings;
void save_dump(IGenSave &cb_, const Dump &dump, const ProfilerDescriptions &, SymbolsCache &symbols); // this is actually like static.
                                                                                                      // It is not static, because it
                                                                                                      // uses descriptions, which are
                                                                                                      // are global (but never
                                                                                                      // released)

void response_finish(IGenSave &cb);                                                                 // thread safe
void response_handshake(IGenSave &cb, uint16_t compression = 0);                                    // thread safe
void response_heartbeat(IGenSave &cb);                                                              // thread safe
void response_live_frames(IGenSave &cb, uint32_t available, const uint64_t *times, uint32_t count); // thread safe
void response_settings(IGenSave &cb, const UserSettings &s);                                        // thread safe

typedef HashedKeySet<uint64_t, 0ULL, oa_hashmap_util::MumStepHash<uint64_t>> SymbolsSet;
struct SymbolsCache
{
  void initModules();
  uint32_t addSymbol(uint64_t addr);
  uint32_t resolveUnresolved(const SymbolsSet &set, uint32_t &was_missing); // returns resolved
  typedef HashedKeyMap<uint64_t, uint32_t, 0ULL, oa_hashmap_util::MumStepHash<uint64_t>> SymbolsMap;
  SymbolsMap symbolMap;

  struct SymbolInfo
  {
    uint32_t fun = ~0u, file = ~0u, line = 0;
  };
  vector<SymbolInfo> symbols;

  struct ModuleInfo
  {
    uint64_t base, size;
    const char *name;
  };
  vector<ModuleInfo> modules;

  UniqueHashedNames<uint32_t> strings; // name, path, module
  mutable std::mutex lock;
  size_t memAllocated() const;
};

void write_modules(IGenSave &cb, SymbolsCache &s);                        // should be under lock
void write_symbols(IGenSave &cb, const SymbolsSet &set, SymbolsCache &s); // should be under lock

}; // namespace da_profiler
