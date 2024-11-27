//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_geomTree.h>
#include <util/dag_roNameMap.h>
#include <memory/dag_framemem.h>


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
  ~GeomTreeIdxMap() { clear(); }

  void clear()
  {
    if (auto prevEntryMap = eastl::exchange(entryMap, nullptr))
      memfree(prevEntryMap, midmem);
    entryCount = flags = 0;
  }
  void init(const GeomNodeTree &tree, const RoNameMapEx &names)
  {
    clear();

    Tab<Pair> map(framemem_ptr());
    map.reserve(names.nameCount());
    for (dag::Index16 i(0), ie(tree.nodeCount()); i != ie; ++i)
      if (int id = names.getNameId(i.index() == 0 ? "@root" : tree.getNodeName(i)); id != -1)
        map.push_back(Pair(id, i));

    entryMap = (Pair *)memalloc(data_size(map), midmem);
    entryCount = map.size();
    mem_copy_to(map, entryMap);
  }

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
