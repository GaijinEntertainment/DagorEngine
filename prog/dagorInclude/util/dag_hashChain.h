//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <util/dag_hashedKeyMap.h>

// Items by equal h >> 1, newest first
struct HashChain
{
  HashedKeyMap<uint64_t, uint32_t, ~uint64_t(0), oa_hashmap_util::MumStepHash<uint64_t>> head; // by h >> 1: never the empty key
  dag::Vector<uint32_t> next;

  uint32_t first(uint64_t h) const { return head.findOr(h >> 1, ~0u); } // ~0u: none
  void add(uint64_t h, uint32_t item)                                   // item above every item added before
  {
    const auto slot = head.emplace_if_missing(h >> 1);
    next.resize(item + 1, ~0u);
    next[item] = slot.second ? ~0u : *slot.first;
    *slot.first = item;
  }
};
