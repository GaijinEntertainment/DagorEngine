//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>
#include <generic/dag_span.h>
#include <dag/dag_vector.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_rwLock.h>
#include <3d/dag_resPtr.h>


namespace frx
{

struct UnifiedGeomBuffers
{
  ReadWriteLock bufLock;
  ReadWriteLock defragLock;

  enum class BufferType : uint8_t
  {
    VB,
    IB
  };

  struct BufferProps
  {
    uint16_t elemSize = 0;
    BufferType type = BufferType::VB;

    explicit operator uint32_t() const { return (uint32_t(elemSize) << 16) | uint32_t(type); }
    bool operator<(const BufferProps &rhs) const { return uint32_t(*this) < uint32_t(rhs); }
    bool operator==(const BufferProps &rhs) const { return uint32_t(*this) == uint32_t(rhs); }
  };

  struct AllocRequest
  {
    uint32_t elemCnt = 0;
    BufferProps bufProp;
    int allocId = -1;
    const uint8_t *data = nullptr;
  };

  void allocate(dag::Span<AllocRequest> requests);
  void free(int alloc_id);

  struct AllocInfo
  {
    Sbuffer *sb;
    uint32_t ofs, cnt, elemSz;
  };
  // must be called under bufLock locked for read
  __forceinline AllocInfo get(int id) const
  {
    const Allocation &a = allocations[id];
    const Page &pg = pages[a.page];
    return AllocInfo{.sb = pg.buf.getBuf(), .ofs = a.reg.ofs, .cnt = a.reg.cnt, .elemSz = pg.prop.elemSize};
  }

private:
  struct Region
  {
    uint32_t ofs, cnt;
  };

  struct Page
  {
    BufferProps prop;
    UniqueBuf buf;
    dag::Vector<Region> freeRegions;
  };
  dag::Vector<Page> pages;

  struct Allocation
  {
    uint16_t page = uint16_t(~0);
    Region reg = {};
  };
  dag::Vector<Allocation> allocations;
  dag::Vector<int> freeAllocSlots;

  eastl::string bufferBaseName = "frx_geom";
  uint32_t minPageSize = 4 << 20;
  uint32_t nextPageId = 0;
};

} // namespace frx