// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "daProfilerTypes.h"
#include <perfMon/dag_daProfilerToken.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_rwSpinLock.h>

namespace da_profiler
{
struct ProfilerDescriptions
{
  DescriptionStorage storage;
  UniqueHashedNames<uint64_t> uniqueNames;              // assume no collisions for 64 bit hashes
  HashedKeyMap<uint64_t, uint32_t, 0ULL> descToStorage; // we assume no collisions for 64 bit hashes of Descriptions
  mutable SpinLockReadWriteLock lock;
  enum
  {
    FrameNo = DescCount,
    Frame,
    Gpu,
    GpuFrame,
    Triangles,
    NestedFrames,
    GpuClock,
    TotalCount
  };
  uint8_t fixedDescriptions[TotalCount] = {0};
  uint32_t frameNo() const { return fixedDescriptions[FrameNo]; }
  uint32_t frame() const { return fixedDescriptions[Frame]; }
  uint32_t gpu() const { return fixedDescriptions[Gpu]; }
  uint32_t gpuFrame() const { return fixedDescriptions[GpuFrame]; }
  uint32_t tri() const { return fixedDescriptions[Triangles]; }
  uint32_t addFramesCount() const { return fixedDescriptions[NestedFrames]; }
  uint32_t gpuClock() const { return fixedDescriptions[GpuClock]; }
  size_t size() const { return count; }
  bool empty() const { return count == 0; }
  uint32_t create(const char *name, const char *file_name, int line, uint32_t flags, uint32_t color);
  uint32_t createCopy(const char *name, const char *file_name, int line, uint32_t flags, uint32_t color);
  const char *getName(uint32_t desc) const;
  const char *get(uint32_t desc, uint32_t &color_and_flags) const;
  size_t memAllocated() const;

protected:
  using ScopedReadLock = ScopedLockReadTemplate<SpinLockReadWriteLock>;
  using ScopedWriteLock = ScopedLockWriteTemplate<SpinLockReadWriteLock>;
  static uint64_t descHash(const EventDescription &desc)
  {
    uint64_t hash = wyhash64(wyhash64((uint64_t)desc.name, desc.colorAndFlags), wyhash64((uint64_t)desc.fileName, desc.lineNo));
    return hash == 0 ? 0x80000000 : hash;
  }
  uint32_t find(uint64_t hash) const { return descToStorage.findOr(hash, ~0u); }
  uint32_t emplaceInternal(const EventDescription &desc, uint64_t hash);
  uint32_t emplaceNoCheck(const EventDescription &desc, uint64_t hash);
  uint32_t emplaceNoCheck(const EventDescription &desc) { return emplaceNoCheck(desc, descHash(desc)); }
  void initInternal();
  uint32_t count = 0;
};

inline size_t ProfilerDescriptions::memAllocated() const
{
  ScopedReadLock guard(lock);
  return storage.memAllocated() + uniqueNames.totalAllocated() + descToStorage.bucket_count() * (sizeof(uint64_t) + sizeof(uint32_t));
}

inline uint32_t ProfilerDescriptions::emplaceNoCheck(const EventDescription &desc, uint64_t hash)
{
  auto ret = descToStorage.emplace_if_missing(hash);
  if (ret.second)
  {
    *ret.first = count++;
    storage.push_back(desc);
  }
  return *ret.first;
}

inline uint32_t ProfilerDescriptions::emplaceInternal(const EventDescription &desc, uint64_t hash)
{
  if (empty())
    initInternal();
  return emplaceNoCheck(desc, hash);
}

inline uint32_t ProfilerDescriptions::create(const char *name, const char *file_name, int line, uint32_t flags, uint32_t color)
{
  if ((flags & 0xFF) == 0xFF) // invalid flags! fix it!
    flags = 0;
  const EventDescription desc{name, file_name, line, (color & 0xFFFFFF) | (flags << 24)};
  const uint64_t hash = descHash(desc);
  {
    ScopedReadLock guard(lock);
    if (uint32_t id = find(hash); id != ~0u)
      return id;
  }
  ScopedWriteLock guard(lock);
  return emplaceInternal(desc, hash);
}
inline uint32_t ProfilerDescriptions::createCopy(const char *name, const char *file_name, int line, uint32_t flags, uint32_t color)
{
  const uint32_t colorAndFlags = (color & 0xFFFFFF) | (flags << 24);
  {
    ScopedReadLock guard(lock);
    const char *uname = uniqueNames.getName(name);
    const char *ufileName = file_name ? uniqueNames.getName(file_name) : nullptr;
    if (uname && (ufileName || !file_name))
      if (uint32_t id = find(descHash(EventDescription{uname, ufileName, line, colorAndFlags})); id != ~0u)
        return id;
  }
  ScopedWriteLock guard(lock);
  file_name = file_name ? uniqueNames.addName(file_name) : nullptr;
  name = uniqueNames.addName(name);
  const EventDescription desc{name, file_name, line, colorAndFlags};
  return emplaceInternal(desc, descHash(desc));
}

inline const char *ProfilerDescriptions::getName(uint32_t desc) const
{
  auto ret = storage.safeGet(desc);
  return ret ? ret->name : "!unknown!";
}

inline const char *ProfilerDescriptions::get(uint32_t desc, uint32_t &color_and_flags) const
{
  auto ret = storage.safeGet(desc);
  if (!ret)
    return nullptr;
  color_and_flags = ret->colorAndFlags;
  return ret->name;
}

}; // namespace da_profiler