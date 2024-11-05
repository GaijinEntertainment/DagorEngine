//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_info.h>

struct SbufferHeapManager
{
  typedef UniqueBuf Heap;
  SbufferHeapManager(const char *name, uint32_t elem_sz, uint32_t flags_, uint32_t texfmt_ = 0) :
    baseName(name), elementSize(elem_sz), flags(flags_), texfmt(texfmt_)
  {}
  void copy(Heap &to, size_t to_offset, const Heap &from, size_t from_offset, size_t len)
  {
    // todo: if no overlappedCopy flag, do it with intermediary buffer/tempBuffer
    if (!shouldCopy || !to.getBuf() || !from.getBuf())
      return;
    G_ASSERT(from.getBuf() != to.getBuf() || overlappedCopy);
    from.getBuf()->copyTo(to.getBuf(), to_offset, from_offset, len);
  }
  bool canCopyInSameHeap() const { return overlappedCopy; }
  bool canCopyOverlapped() const { return overlappedRegionCopy; }
  bool create(Heap &h, size_t sz)
  {
    overlappedCopy = d3d::get_driver_desc().caps.hasBufferOverlapCopy;              // do it each create. not too often and safer
                                                                                    // than constructor for static variable
    overlappedRegionCopy = d3d::get_driver_desc().caps.hasBufferOverlapRegionsCopy; // do it each create. not too
                                                                                    // often and safer than
                                                                                    // constructor for static
                                                                                    // variable
    h.close();
    if (sz > 0) // resize with 0 closes the buffer on-demand
    {
      eastl::string name;
      name.sprintf("%s_%d", baseName.c_str(), id++);
      h = dag::create_sbuffer(elementSize, (sz + elementSize - 1) / elementSize, flags, texfmt, name.c_str());
      generation++;
    }
    return (bool)h;
  }
  void orphan(Heap &h) { h.close(); }
  const char *getName() const { return baseName.c_str(); }
  void setShouldCopyToNewHeap(bool on) { shouldCopy = on; } // if we have some other way to restore data
  uint32_t getHeapGeneration() const { return generation; }

protected:
  uint32_t elementSize = 0, flags = 0, texfmt = 0;
  uint32_t id = 0, generation = 0;
  bool overlappedCopy = false, overlappedRegionCopy = false, shouldCopy = true;
  eastl::string baseName;
  // Heap tempBuffer;
};

template <class T>
struct LinearHeapAllocator;
typedef LinearHeapAllocator<SbufferHeapManager> LinearHeapAllocatorSbuffer;
