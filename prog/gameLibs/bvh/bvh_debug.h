// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <bvh/bvh.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>

namespace bvh
{

enum class DebugMode
{
  Unknown,
  None,
  Lit,
  DiffuseColor,
  Normal,
  Texcoord,
  SecTexcoord,
  CamoTexcoord,
  VertexColor,
  GI,
  Paint,
  IntersectionCount,
  Instances,
  NaN,
  Lod
};

// Real RT-only memory overhead, computed from the ground up (every GPU resource the BVH context
// allocates), categorized logically.
struct RtMemoryOverhead
{
  struct Item
  {
    int64_t bytes;
    eastl::string category;
    eastl::string sub;
    eastl::string note; // extra, non-additive detail (count, free headroom, sub-splits); shown in brackets
  };
  eastl::vector<Item> items;
  int64_t total = 0;
  int64_t blasTotalBytes = 0;
  int blasCount = 0;
  int64_t lastLodBlasBytes = 0; // this is practically a constant overhead, since they are loaded no matter what
  int lastLodBlasCount = 0;

  void add(const char *category, const char *sub, int64_t b, const char *note = nullptr)
  {
    items.push_back(Item{b, category, sub, note ? note : ""});
    total += b;
  }

  template <typename CategoryStartFn, typename ItemFn, typename CategoryEndFn>
  void forEachCategory(CategoryStartFn on_category_start, ItemFn on_item, CategoryEndFn on_category_end) const
  {
    const eastl::string *curCat = nullptr;
    int64_t catSum = 0;
    for (const auto &it : items)
    {
      if (!curCat || *curCat != it.category)
      {
        if (curCat)
          on_category_end(*curCat, catSum);
        curCat = &it.category;
        catSum = 0;
        on_category_start(*curCat);
      }
      catSum += it.bytes;
      on_item(it);
    }
    if (curCat)
      on_category_end(*curCat, catSum);
  }
};

RtMemoryOverhead get_rt_memory_overhead(ContextId context_id);

} // namespace bvh
