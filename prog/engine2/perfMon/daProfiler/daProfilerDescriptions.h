#pragma once
#include "daProfilerTypes.h"
#include <perfMon/dag_daProfilerToken.h>

namespace da_profiler
{
struct ProfilerDescriptions
{
  DescriptionStorage storage;
  UniqueHashedNames<uint64_t> uniqueNames;              // assume no collisions for 64 bit hashes
  HashedKeyMap<uint64_t, uint32_t, 0ULL> descToStorage; // we assume no collisions for 64 bit hashes of Descriptions
  mutable std::mutex lock;
  enum
  {
    FrameNo = DescCount,
    Frame,
    Gpu,
    GpuFrame,
    Triangles,
    NestedFrames,
    TotalCount
  };
  uint8_t fixedDescriptions[TotalCount] = {0};
  uint32_t frameNo() const { return fixedDescriptions[FrameNo]; }
  uint32_t frame() const { return fixedDescriptions[Frame]; }
  uint32_t gpu() const { return fixedDescriptions[Gpu]; }
  uint32_t gpuFrame() const { return fixedDescriptions[GpuFrame]; }
  uint32_t tri() const { return fixedDescriptions[Triangles]; }
  uint32_t addFramesCount() const { return fixedDescriptions[NestedFrames]; }
  size_t size() const { return count; }
  bool empty() const { return count == 0; }
  uint32_t create(const char *name, const char *file_name, int line, uint32_t flags, uint32_t color);
  uint32_t createCopy(const char *name, const char *file_name, int line, uint32_t flags, uint32_t color);
  const char *get(uint32_t desc) const;
  size_t memAllocated() const;

protected:
  uint32_t emplaceInternal(const EventDescription &desc);
  uint32_t emplaceNoCheck(const EventDescription &desc);
  void initInternal();
  uint32_t count = 0;
};

inline size_t ProfilerDescriptions::memAllocated() const
{
  std::lock_guard<std::mutex> guard(lock);
  return storage.memAllocated() + uniqueNames.totalAllocated() + descToStorage.bucket_count() * (sizeof(uint64_t) + sizeof(uint32_t));
}

inline uint32_t ProfilerDescriptions::emplaceNoCheck(const EventDescription &desc)
{
  uint64_t hash = wyhash64(wyhash64((uint64_t)desc.name, desc.colorAndFlags), wyhash64((uint64_t)desc.fileName, desc.lineNo));
  hash = hash == 0 ? 0x80000000 : hash;
  auto ret = descToStorage.emplace_if_missing(hash);
  if (ret.second)
  {
    *ret.first = count++;
    storage.push_back(desc);
  }
  return *ret.first;
}

inline uint32_t ProfilerDescriptions::emplaceInternal(const EventDescription &desc)
{
  if (empty())
    initInternal();
  return emplaceNoCheck(desc);
}

inline uint32_t ProfilerDescriptions::create(const char *name, const char *file_name, int line, uint32_t flags, uint32_t color)
{
  if ((flags & 0xFF) == 0xFF) // invalid flags! fix it!
    flags = 0;
  std::lock_guard<std::mutex> guard(lock);
  return emplaceInternal(EventDescription{name, file_name, line, (color & 0xFFFFFF) | (flags << 24)});
}
inline uint32_t ProfilerDescriptions::createCopy(const char *name, const char *file_name, int line, uint32_t flags, uint32_t color)
{
  std::lock_guard<std::mutex> guard(lock);
  file_name = file_name ? uniqueNames.addName(file_name) : nullptr;
  name = uniqueNames.addName(name);
  return emplaceInternal(EventDescription{name, file_name, line, (color & 0xFFFFFF) | (flags << 24)});
}

inline const char *ProfilerDescriptions::get(uint32_t desc) const
{
  auto ret = storage.safeGet(desc);
  return ret ? ret->name : "!unknown!";
}

}; // namespace da_profiler