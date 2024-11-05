// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace Forsyth
{

//-----------------------------------------------------------------------------
//  OptimizeFaces
//-----------------------------------------------------------------------------
//  Parameters:
//      indexList
//          input index list
//      indexCount
//          the number of indices in the list
//      vertexCount
//          the largest index value in indexList
//      newIndexList
//          a pointer to a preallocated buffer the same size as indexList to
//          hold the optimized index list
//      lruCacheSize
//          the size of the simulated post-transform cache (max:kMaxVertexCacheSize)
//-----------------------------------------------------------------------------
template <class Indices>
void OptimizeFaces(const Indices *indexList, unsigned indexCount, unsigned max_index, Indices *newIndexList, unsigned lruCacheSize);

class VertexCacheAnalyze
{
protected:
  enum
  {
    EFFECTIVE_CACHE = 24,
    CACHE_ADD = 8,
    CACHE_SIZE = EFFECTIVE_CACHE + CACHE_ADD
  };
  int cache[CACHE_SIZE];
  int misses; // cache miss count

  int FindVertex(int v)
  {
    for (int i = 0; i < EFFECTIVE_CACHE; i++)
      if (cache[i] == v)
        return i;
    return -1;
  }

  void RemoveVertex(int stack_index)
  {
    for (int i = stack_index; i < CACHE_SIZE - 2; i++)
      cache[i] = cache[i + 1];
  }

public:
  // the vertex will be placed on top
  // if the vertex didn't exist previewsly in
  // the cache, then miss count is incermented
  void AddVertex(int v)
  {
    int w = FindVertex(v);
    if (w >= 0)
      // remove the vertex from the cache (to reinsert it later on the top)
      RemoveVertex(w);
    else
      // the vertex was not found in the cache - increment misses
      misses++;

    // shift all vertices down (to make room for the new top vertex)
    for (int i = CACHE_SIZE - 1; i > 0; i--)
      cache[i] = cache[i - 1];

    // add the new vertex on top
    cache[0] = v;
  }

  void Clear()
  {
    for (int i = 0; i < CACHE_SIZE; i++)
      cache[i] = -1;
    misses = 0;
  }

  VertexCacheAnalyze() { Clear(); }

  int GetCacheMissCount() { return misses; }

  int GetCachedVertex(int which) { return cache[which]; }

  int GetCacheMissCount(unsigned short *inds, int tri_count)
  {
    Clear();

    for (int i = 0; i < 3 * tri_count; i++)
      AddVertex(inds[i]);
    return misses;
  }
};
}; // namespace Forsyth
