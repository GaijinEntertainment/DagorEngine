// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <gameLibs/daCSG/solid_api.h>
#include <dag/dag_vector.h>

namespace dacsg
{
// disjoint sets over 0 .. n - 1: a find that halves its path, a join of the roots
struct UnionFind
{
  dag::Vector<uint32_t> parent;

  void init(uint32_t n)
  {
    parent.resize(n);
    for (uint32_t i = 0; i < n; ++i)
      parent[i] = i;
  }
  uint32_t find(uint32_t v)
  {
    while (parent[v] != v)
      v = parent[v] = parent[parent[v]];
    return v;
  }
  void join(uint32_t a, uint32_t b) { parent[find(a)] = find(b); }
  // keeps the lower root; false: already one set
  bool joinLower(uint32_t a, uint32_t b)
  {
    a = find(a);
    b = find(b);
    parent[a < b ? b : a] = a < b ? a : b;
    return a != b;
  }
  // the set of each element, numbered in first-seen order; returns the count
  uint32_t compress(dag::Vector<uint32_t> &id)
  {
    id.assign(parent.size(), NO_INDEX);
    uint32_t count = 0;
    for (uint32_t i = 0; i < parent.size(); ++i)
    {
      const uint32_t root = find(i);
      if (id[root] == NO_INDEX)
        id[root] = count++;
      id[i] = id[root];
    }
    return count;
  }
};
} // namespace dacsg
