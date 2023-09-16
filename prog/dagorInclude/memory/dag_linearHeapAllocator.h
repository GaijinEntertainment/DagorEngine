//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dag/dag_vector.h>
#include <util/dag_generationRefId.h>
#include <util/dag_generationReferencedData.h>
#include <EASTL/vector_set.h>
#include <EASTL/vector_set.h>
#include <memory/dag_framemem.h>
#include <util/dag_stlqsort.h>
#include <debug/dag_debug.h>

#ifndef VALIDATE_LINEAR_HEAP_ALLOCATOR
#if DAGOR_DBGLEVEL > 1
#define VALIDATE_LINEAR_HEAP_ALLOCATOR 1
#else
#define VALIDATE_LINEAR_HEAP_ALLOCATOR 0
#endif
#endif


// for manual/deferred handling of data inside the heap
struct NullHeapManager
{
public:
  struct Heap
  {};

  NullHeapManager() {}

  void copy(Heap &, size_t, const Heap &, size_t, size_t) {}
  bool canCopyInSameHeap() const { return true; }
  bool canCopyOverlapped() const { return true; }
  bool create(Heap &, size_t) { return true; }
  void orphan(Heap &) {}
};


// LinearHeapAllocator that supports 'lazy' defragmentation (with one buffer move at most per one defragment() call)
// one defragment call can issue several copies though
// this is CPU only structure, it doesn't actually perform gpu mem allocation by itself, but it can
// it is not bi-partioned, i.e. we always try to manage empty space in the end of the buffer
//  that is in assumption, that lifetime of regions is unpredictable
// if it is supposed to be FIFO, than it is better to implement bi-partioned logic and do not defragment first free region (as well as
// last)
template <typename HeapManager>
struct LinearHeapAllocator
{
  struct LinearHeapDummy
  {};
  typedef uint32_t len_t;
  struct Region
  {
    len_t offset, size;
  };
  typedef GenerationRefId<8, LinearHeapDummy> RegionId;
  typedef typename HeapManager::Heap Heap;

  bool resize(len_t capacity);  // false if it can't be done, or sz < allocated. Will do nothing if all memory is defragmented
  bool reserve(len_t capacity); // false if it can't be done. Will do nothing a
  LinearHeapAllocator() = default;
  LinearHeapAllocator(HeapManager &&m) : manager(eastl::move(m)) {}

  RegionId allocateInHeap(len_t size);            // allocate if there is enough contigous space in heap
  RegionId allocate(len_t size, len_t page_size); // can cause memory move. Will only fail if manager fails to create

  bool free(const RegionId &);
  bool free(RegionId &);
  Region get(RegionId) const;
  uint32_t offsetsGeneration() const { return heapGeneration; }
  size_t getHeapSize() const { return totalSize; }
  size_t allocated() const { return totalSize - freeSize; }
  size_t freeMemLeft() const { return freeSize; }
  size_t biggestContigousChunkLeft() const { return biggestFreeChunkSize; }
  bool canAllocate(size_t sz) const { return sz <= biggestContigousChunkLeft(); }
  // returns number of copy calls if defragmented, will ignore "holes" of <= threshold
  // will not copy more, than max_sz_to_copy elements (to optimize cost of said copy calls)
  // it is not a hard limit, as it will at least make _one_ copy, not splitting allocated regions
  size_t defragment(size_t min_process_hole_size = 0, size_t max_sz_to_copy = ~0);
  void dump();
  Heap &getHeap() { return currentHeap; }
  const Heap &getHeap() const { return currentHeap; }
  const HeapManager &getManager() const { return manager; }
  HeapManager &getManager() { return manager; }
  void validate();
  uint32_t regionsCount() const { return active.totalSize(); }
  const Region *getByIdx(uint32_t i) const { return active.getByIdx(i); }
  RegionId getRefByIdx(uint32_t i) const { return active.getRefByIdx(i); }
  void clear();

private:
  size_t defragmentChunk(size_t fi, size_t max_sz_to_copy);
  void validateNonEmptyRegion(Region r);
  void updateBiggestFreeChunk(const Region &r) { biggestFreeChunkSize = eastl::max(r.size, biggestFreeChunkSize); }
  len_t findBiggestFree() const;

  GenerationReferencedData<RegionId, Region> active;
  dag::Vector<Region> freeRegions; // freed regions, sorted from begining by offset
  Heap currentHeap;
  HeapManager manager;
  len_t biggestFreeChunkSize = 0;
  len_t totalSize = 0;
  len_t freeSize = 0;
  uint32_t heapGeneration = 0;
};

template <typename HeapManager>
void LinearHeapAllocator<HeapManager>::clear()
{
  biggestFreeChunkSize = freeSize = totalSize;
  active.clear();
  freeRegions.clear();
  freeRegions.push_back(Region{0, freeSize});
}

template <typename HeapManager>
inline bool LinearHeapAllocator<HeapManager>::resize(len_t nextCapacity)
{
  if (nextCapacity < allocated()) // we can't fit current data
    return false;
  if (nextCapacity == totalSize && freeRegions.size() == 1 && freeRegions[0].offset + freeRegions[0].size == totalSize) // nothing can
                                                                                                                        // be done
    return true;
  Heap oldHeap = eastl::move(currentHeap);
  if (!manager.create(currentHeap, nextCapacity))
  {
    currentHeap = eastl::move(oldHeap);
    return false;
  }
  heapGeneration++;
  totalSize = nextCapacity;

  // and copy old heap to current
  // we sort by order, and copy in biggest possible contiguos blocks, to minimize number of copy calls
  dag::Vector<Region *, framemem_allocator> validActiveRegions;
  validActiveRegions.reserve(active.totalSize());
  for (uint32_t i = 0, e = active.totalSize(); i != e; ++i) // copy regions to new heap, while performing an defragmentation
    if (auto r = active.getByIdx(i))
      validActiveRegions.push_back(r);

  // minimize copy calls, by sorting offsets
  stlsort::sort(validActiveRegions.begin(), validActiveRegions.end(), [](auto a, auto b) { return a->offset < b->offset; });
  len_t curDestOffset = 0, lastCopiedSrcOffset = 0, sizeToCopy = 0;

  for (auto r : validActiveRegions) // copy regions to new heap, while performing an defragmentation
  {
    if (lastCopiedSrcOffset != r->offset)
    {
      if (sizeToCopy != 0)
      {
        manager.copy(currentHeap, curDestOffset - sizeToCopy, oldHeap, lastCopiedSrcOffset, sizeToCopy);
        sizeToCopy = 0;
      }
      lastCopiedSrcOffset = r->offset;
    }
    r->offset = curDestOffset;
    curDestOffset += r->size;
    sizeToCopy += r->size;
  }
  if (sizeToCopy != 0)
    manager.copy(currentHeap, curDestOffset - sizeToCopy, oldHeap, lastCopiedSrcOffset, sizeToCopy);
  manager.orphan(oldHeap); // all copied

  freeSize = biggestFreeChunkSize = totalSize - curDestOffset;
  freeRegions.clear();
  freeRegions.push_back(Region{curDestOffset, freeSize});
  return true;
}

template <typename HeapManager>
inline bool LinearHeapAllocator<HeapManager>::reserve(len_t nextCapacity)
{
  if (nextCapacity <= totalSize)
    return true;
  return resize(nextCapacity);
}

template <typename HeapManager>
inline typename LinearHeapAllocator<HeapManager>::RegionId LinearHeapAllocator<HeapManager>::allocateInHeap(len_t size)
{
  if (!size || !canAllocate(size))
    return LinearHeapAllocator::RegionId{};
  // linear search for smallest acceptable free chunk
  // we can store separate ordered map, sorted by size, for performance
  size_t memLeft = size_t(~size_t(0)), best = size_t(~size_t(0));
  for (size_t i = 0, e = freeRegions.size(); i < e; ++i)
  {
    const size_t mem = size_t(size_t(freeRegions[i].size) - size_t(size));
    if (mem < memLeft)
    {
      memLeft = mem;
      best = i;
    }
  }
  G_ASSERTF(freeRegions[best].size >= size, "%d %d %d", best, freeRegions[best].size, size);
  auto ret = active.emplaceOne(Region{freeRegions[best].offset, len_t(size)});
  freeRegions[best].offset += size;
  const bool shouldRefind = freeRegions[best].size == biggestFreeChunkSize;
  freeRegions[best].size -= size;
  if (freeRegions[best].size == 0)
    freeRegions.erase(freeRegions.begin() + best);
  if (shouldRefind) // otherwise biggest was some other chunk, so we don't need to refind
    biggestFreeChunkSize = findBiggestFree();
  freeSize -= size;
  validate();
  return ret;
}

template <typename HeapManager>
inline typename LinearHeapAllocator<HeapManager>::RegionId LinearHeapAllocator<HeapManager>::allocate(len_t size, len_t page_size)
{
  if (canAllocate(size))
    return allocateInHeap(size);

  // we need to allocate new heap
  const len_t nextCapacity = (totalSize + size - freeSize + page_size - 1) & ~(page_size - 1); // todo: some add some better strategy
                                                                                               // here?
  if (!resize(nextCapacity))
    return LinearHeapAllocator::RegionId{};
  G_ASSERT(biggestFreeChunkSize >= size);
  return allocateInHeap(size);
}

template <typename HeapManager>
inline typename LinearHeapAllocator<HeapManager>::Region LinearHeapAllocator<HeapManager>::get(RegionId id) const
{
  auto r = active.cget(id);
  return r ? *r : Region{~len_t(0), 0};
}

template <typename HeapManager>
inline typename LinearHeapAllocator<HeapManager>::len_t LinearHeapAllocator<HeapManager>::findBiggestFree() const
{
  len_t csz = 0;
  for (auto &r : freeRegions)
    csz = eastl::max(r.size, csz);
  return csz;
}

template <typename HeapManager>
inline void LinearHeapAllocator<HeapManager>::validateNonEmptyRegion(Region r)
{
  G_UNUSED(r);
#if VALIDATE_LINEAR_HEAP_ALLOCATOR
  for (auto &i : freeRegions)
    G_ASSERT(i.offset <= r.offset + r.size || r.offset <= i.offset + i.size);
#endif
}

template <typename HeapManager>
inline void LinearHeapAllocator<HeapManager>::validate()
{
#if VALIDATE_LINEAR_HEAP_ALLOCATOR
  len_t freeSize2 = 0;
  for (size_t i = 0, e = freeRegions.size(); i != e; ++i)
  {
    freeSize2 += freeRegions[i].size;
    G_ASSERT(freeRegions[i].size > 0);
    G_ASSERT(freeRegions[i].offset + freeRegions[i].size <= totalSize);
    if (i + 1 < e)
      G_ASSERTF(freeRegions[i].offset + freeRegions[i].size < freeRegions[i + 1].offset, "%d: %d + %d >= %d", i, freeRegions[i].offset,
        freeRegions[i].size, freeRegions[i + 1].offset); // otherwise it is overlapped, or not-coalesced
  }
  if (biggestFreeChunkSize != findBiggestFree())
  {
    debug("biggestFreeChunkSize = %d, %d", biggestFreeChunkSize, findBiggestFree());
    dump();
  }
  G_ASSERT(biggestFreeChunkSize == findBiggestFree());
  G_ASSERT(freeSize2 == freeSize);
  len_t sz = 0;
  for (size_t i = 0, e = active.totalSize(); i != e; ++i)
  {
    if (auto r = active.getByIdx(i))
    {
      validateNonEmptyRegion(*r);
      G_ASSERT(r->size + r->offset <= totalSize);
      sz += r->size;
    }
  }
  G_ASSERT(sz == allocated());
#endif
}

template <typename HeapManager>
inline bool LinearHeapAllocator<HeapManager>::free(const RegionId &id)
{
  auto r = active.cget(id);
  if (!r)
    return false;
  validateNonEmptyRegion(*r);
  // debug("free %d %d", r->offset, r->size);
  freeSize += r->size;
  intptr_t at = 0;
  auto it = eastl::lower_bound(freeRegions.begin(), freeRegions.end(), *r, [&](auto a, auto b) { return a.offset < b.offset; });
  if (it == freeRegions.end())
  {
    // debug("add at end %d %d", r->offset, r->size);
    at = freeRegions.size();
    freeRegions.push_back(*r);
  }
  else
  {
    // fast coalescing of free regions, there is already a region that is
    at = it - freeRegions.begin();
    if (r->offset + r->size == it->offset)
    {
      // debug("update at coalesce %d %d + %d %d", r->offset, r->size, it->offset, it->size);
      it->offset = r->offset;
      it->size += r->size;
    }
    else
    {
      // debug("add at %d %d + %d %d", r->offset, r->size, it->offset, it->size);
      freeRegions.insert(it, *r);
    }
  }
  updateBiggestFreeChunk(freeRegions[at]);
  active.destroyReference(id);

  for (--at; at >= 0; --at) // free regions coalescing after inserted offset (we could fill a hole)
  {
    // debug("check %d %d + %d %d", freeRegions[at].offset, freeRegions[at].size, freeRegions[at+1].offset, freeRegions[at+1].size);
    if (freeRegions[at].offset + freeRegions[at].size == freeRegions[at + 1].offset)
    {
      freeRegions[at + 1].offset = freeRegions[at].offset;
      freeRegions[at + 1].size += freeRegions[at].size;
      updateBiggestFreeChunk(freeRegions[at + 1]);
      freeRegions.erase(freeRegions.begin() + at);
    }
    else
      break; // if it is not a hole, than no need to iterate other - everything is already coalesced
  }
  validate();
  return true;
}
template <typename HeapManager>
inline bool LinearHeapAllocator<HeapManager>::free(RegionId &id)
{
  bool ret = free((const RegionId &)id);
  id = RegionId{};
  return ret;
}

template <typename HeapManager>
inline void LinearHeapAllocator<HeapManager>::dump()
{
  debug("dumping HeapManager of totalSize = %d freeSize %d", totalSize, freeSize);
  for (auto &f : freeRegions)
    debug("free region at %d sz=%d", f.offset, f.size);

  size_t count = 0, allocatedSize = 0;
  for (size_t i = 0, e = active.totalSize(); i != e; ++i) // copy regions to new heap, while performing an defragmentation
  {
    if (auto r = active.getByIdx(i))
    {
      validateNonEmptyRegion(*r);
      debug("region at %d sz=%d", r->offset, r->size);
      allocatedSize += r->size;
      ++count;
    }
  }
  debug("totalRegions = %d, totalAllocatedSize = %d", count, allocatedSize);
  G_ASSERT(allocatedSize == allocated());
  validate();
}

template <typename HeapManager>
inline size_t LinearHeapAllocator<HeapManager>::defragmentChunk(size_t fi, size_t max_sz_to_copy)
{
  G_ASSERT(freeRegions.size() > fi);
  const len_t holeSz = freeRegions[fi].size;
  const len_t dstOffset = freeRegions[fi].offset;
  if (dstOffset + holeSz == totalSize) // nothing to defragment, it is last one
    return 0;
  const len_t srcOffset = dstOffset + holeSz;
  const len_t nextFreeOffset = freeRegions.size() > fi + 1 ? freeRegions[fi + 1].offset : totalSize;
  len_t tillOffset = nextFreeOffset;
  if (nextFreeOffset - srcOffset > max_sz_to_copy) // target is out of ideal solution
  {
    // next free offset is too far, we need to copy less data.
    // So, find the minimax: maximum copy size, that is still smaller than max_sz_to_copy
    size_t bestNextFreePosition = tillOffset;
    for (size_t i = 0, e = active.totalSize(); i != e; ++i) // update moved regions
    {
      if (auto r = active.getByIdx(i))
      {
        if (r->offset >= srcOffset && r->offset < nextFreeOffset) // within the block
        {
          const len_t endPos = r->offset + r->size;
          const len_t thisCopySize = endPos - srcOffset;
          const len_t prevCopySize = bestNextFreePosition - srcOffset;
          if (prevCopySize <= max_sz_to_copy) // we are already within limits
          {
            // maximize copy size, but stay within the limits
            if (thisCopySize > prevCopySize && thisCopySize <= max_sz_to_copy) // it is better than previous option, and within limits
              bestNextFreePosition = endPos;
          }
          else // we are out of limits now
          {
            // minimize copy size, as we are out of limits anyway
            if (thisCopySize < prevCopySize) // at least it is better than previous option
              bestNextFreePosition = endPos;
          }
          if (thisCopySize == max_sz_to_copy) // ideal solution
            break;
        }
      }
    }
    if (bestNextFreePosition != 0) // we were not able to find it. That means that allocated blocks after hole are bigger than allowed
                                   // size
      tillOffset = bestNextFreePosition;
  }

  len_t at = srcOffset, end = tillOffset;
  size_t calls = 0;
  if (manager.canCopyOverlapped()) // manager supports memmmove, so make one call
  {
    manager.copy(currentHeap, at - holeSz, currentHeap, at, end - at);
    at = end;
    calls = 1;
  }
  else // manager doesn't support memmove-style, only memcpy-style
  {
    for (; at < end;)
    {
      const len_t sz = eastl::min(end - at, holeSz);
      G_ASSERT(at - holeSz + sz <= at); // not overlapped!
      manager.copy(currentHeap, at - holeSz, currentHeap, at, sz);
      at += sz;
      ++calls;
    }
  }

  freeRegions[fi].offset = tillOffset - holeSz;
  if (freeRegions.size() > fi + 1 && freeRegions[fi + 1].offset == freeRegions[fi].offset + holeSz) // coalesce with next free chunk
  {
    G_ASSERTF(at == freeRegions[fi + 1].offset && freeRegions[fi + 1].offset >= holeSz, "at=%d, nextFree %d, holeSz %d", at,
      freeRegions[fi + 1].offset, holeSz);
    freeRegions[fi].offset = freeRegions[fi + 1].offset - holeSz;
    freeRegions[fi].size += freeRegions[fi + 1].size;
    updateBiggestFreeChunk(freeRegions[fi]);
    freeRegions.erase(freeRegions.begin() + fi + 1);
  }

  for (size_t i = 0, e = active.totalSize(); i != e; ++i) // update moved regions
  {
    if (auto r = active.getByIdx(i))
    {
      if (r->offset >= srcOffset && r->offset < tillOffset)
      {
        G_ASSERT(r->offset + r->size <= tillOffset);
        G_ASSERT(r->offset >= holeSz);
        r->offset -= holeSz;
      }
    }
  }
  validate();
  heapGeneration++;
  return calls;
}

template <typename HeapManager>
inline size_t LinearHeapAllocator<HeapManager>::defragment(size_t min_hole_size, size_t max_sz_to_copy)
{
  if (!manager.canCopyInSameHeap()) // defragmentation is not even possible, HeapManager is unable to perform copy within same heap
    return 0;
  if (biggestFreeChunkSize <= min_hole_size) // nothing to defragment
    return 0;
  if (freeRegions.size() == 1)
  {
    G_ASSERT(freeSize == freeRegions[0].size);
    if (freeRegions[0].offset + freeRegions[0].size == totalSize) // nothing to defragment, it is last one
      return 0;
  }
  for (size_t i = 0, e = freeRegions.size(); i != e; ++i)
  {
    if (freeRegions[i].size > min_hole_size)
    {
      if (size_t calls = defragmentChunk(i, max_sz_to_copy))
        return calls;
    }
  }
  return 0;
}


using LinearHeapAllocatorNull = LinearHeapAllocator<NullHeapManager>;
