//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_index16.h>
#include <generic/dag_span.h>
#include <EASTL/utility.h>

class GeomNodeTree;
struct RoNameMapEx;

class GeomTreeIdxMap
{
public:
  static constexpr int FLG_INCOMPLETE = 1;
  struct Pair
  {
    uint16_t id;
    dag::Index16 nodeIdx;

    inline Pair(int _id, dag::Index16 _nodeIdx) : id(_id), nodeIdx(_nodeIdx) {}
  };

public:
  GeomTreeIdxMap() = default;
  GeomTreeIdxMap(const GeomTreeIdxMap &) = delete;            // Not implemented
  GeomTreeIdxMap &operator=(const GeomTreeIdxMap &) = delete; // Ditto
  GeomTreeIdxMap(GeomTreeIdxMap &&rhs) :
    entryMap(eastl::exchange(rhs.entryMap, nullptr)),
    entryCount(eastl::exchange(rhs.entryCount, 0)),
    flags(eastl::exchange(rhs.flags, 0))
  {}
  GeomTreeIdxMap &operator=(GeomTreeIdxMap &&rhs)
  {
    entryMap = eastl::exchange(rhs.entryMap, nullptr);
    entryCount = eastl::exchange(rhs.entryCount, 0);
    flags = eastl::exchange(rhs.flags, 0);
    return *this;
  }
  ~GeomTreeIdxMap() { clear(); }

  void clear()
  {
    if (auto prevEntryMap = eastl::exchange(entryMap, nullptr))
      memfree(prevEntryMap, midmem);
    entryCount = flags = 0;
  }
  void init(const GeomNodeTree &tree, const RoNameMapEx &names);
  int size() const { return entryCount; }
  dag::ConstSpan<Pair> map() const { return make_span_const(entryMap, entryCount); }

  Pair &operator[](int idx) { return entryMap[idx]; }
  const Pair &operator[](int idx) const { return entryMap[idx]; }

  void markIncomplete() { flags |= FLG_INCOMPLETE; }
  bool isIncomplete() { return (flags & FLG_INCOMPLETE); }

private:
  Pair *entryMap = nullptr;
  unsigned short entryCount = 0, flags = 0;
#if _TARGET_64BIT
  unsigned _resv = 0;
#endif
};
