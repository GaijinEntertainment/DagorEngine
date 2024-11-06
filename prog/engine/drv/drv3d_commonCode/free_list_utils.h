// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "value_range.h"
#include <EASTL/algorithm.h>
#include <EASTL/iterator.h>
#include <util/dag_globDef.h>


// Generic free list insert algorithm. A free list is a container that
// is ValueRange<R> compatible and sorted by increasing 'front' member value.
// The algorithm linearly searches for the position at the container that could
// be merged with the inserted range. If a range is located, that can be merged,
// it merges it with the range(s) (by plugging a hole it can be two) and in the
// case of fusing two adjacent ranges together, the superfluous entry is removed.
// If no suitable range could be found, the new range is inserted to maintain the
// sorted order of the 'front' member.
template <typename R, typename C>
inline void free_list_insert_and_coalesce(C &container, ValueRange<R> range)
{
  using eastl::begin;
  using eastl::end;

  auto at = begin(container);
  auto pat = at;
  auto ed = end(container);
  for (; at != ed; pat = at++)
  {
    if (at->start < range.start)
    {
      if (at->stop < range.start)
      {
        continue;
      }

      at->stop = range.stop;
      auto nxt = at + 1;
      if (nxt != ed)
      {
        if (range.stop == nxt->start)
        {
          at->stop = nxt->stop;
          container.erase(nxt);
        }
      }
      return;
    }
    else
    {
      if (range.stop < at->start)
      {
        break;
      }

      // did we just full in the gap in between pat and at?
      // TODO should be inpossible.
      if (pat->stop == range.start)
      {
        pat->stop = at->stop;
        container.erase(at);
      }
      else
      {
        at->start = range.start;
      }
      return;
    }
  }
  container.insert(at, range);
}

template <typename C, typename S>
inline auto free_list_find_smallest_fit(C &container, S count)
{
  using eastl::begin;
  using eastl::end;
  using eastl::find_if;

  // find first
  auto at = find_if(begin(container), end(container),
    [count](auto range) //
    { return range.size() >= count; });

  if (at == end(container))
  {
    return at;
  }

  // find smallest (may not be the best, though!)
  auto cmp = at++;
  if (at != end(container) && cmp->size() > count)
  {
    do
    {
      at = find_if(at, end(container),
        [cmp, count](auto range) //
        { return range.size() >= count && range.size() < cmp->size(); });
      if (at == end(container))
      {
        break;
      }
      cmp = at++;
    } while (at != end(container) && cmp->size() > count);
  }

  return cmp;
}

template <typename C, typename S>
inline auto free_list_find_smallest_fit_aligned(C &container, S count, S alignment)
{
  using eastl::begin;
  using eastl::end;
  using eastl::find_if;

  // find first
  auto at = find_if(begin(container), end(container),
    [count, alignment](auto range) //
    {
      auto r2 = range.front(((range.front() + alignment - 1) & ~(alignment - 1)) - range.front());
      // alignment offset may push it over stop
      if (!r2.isValidRange())
      {
        return false;
      }
      return r2.size() >= count;
    });

  if (at == end(container))
  {
    return at;
  }

  // find smallest (may not be the best, though!)
  auto cmp = at++;
  if (at != end(container) && cmp->size() > count)
  {
    do
    {
      at = find_if(at, end(container),
        [cmp, count, alignment](auto range) //
        {
          auto r2 = range.front(((range.front() + alignment - 1) & ~(alignment - 1)) - range.front());
          // alignment offset may push it over stop
          if (!r2.isValidRange())
          {
            return false;
          }
          return r2.size() >= count && r2.size() < cmp->size();
        });
      if (at == end(container))
      {
        break;
      }
      cmp = at++;
    } while (at != end(container) && cmp->size() > count);
  }

  return cmp;
}

// Allocation algorithm trying to allocate a range with 'count' elements from
// the given container of ranges. It searches for the smallest suitable range
// and allocates 'count' elements from it. Should the range be empty, after
// the allocation, its element will be removed from the container.
// Should there be no suitable range be in container, a empty range will be
// returned.
template <typename R, typename C, typename S>
inline ValueRange<R> free_list_allocate_smallest_fit(C &container, S count)
{
  using eastl::end;
  auto ref = free_list_find_smallest_fit(container, count);
  if (ref == end(container))
  {
    return {};
  }

  auto r = *ref;
  if (ref->size() == count)
  {
    container.erase(ref);
  }
  else
  {
    ref->start = r.stop = r.start + count;
  }
  return r;
}

template <typename C, typename S>
inline auto free_list_find_reverse_smallest_fit(C &container, S count)
{
  using eastl::find_if;
  using eastl::rbegin;
  using eastl::rend;

  // find first
  auto at = find_if(rbegin(container), rend(container),
    [count](auto range) //
    { return range.size() >= count; });

  if (at == rend(container))
  {
    return at;
  }

  // find smallest (may not be the best, though!)
  auto cmp = at++;
  if (at != rend(container) && cmp->size() > count)
  {
    do
    {
      at = find_if(at, rend(container),
        [cmp, count](auto range) //
        { return range.size() >= count && range.size() < cmp->size(); });
      if (at == rend(container))
      {
        break;
      }
      cmp = at++;
    } while (at != rend(container) && cmp->size() > count);
  }

  return cmp;
}

template <typename R, typename C, typename S>
inline ValueRange<R> free_list_allocate_reverse_smallest_fit(C &container, S count)
{
  using eastl::rend;
  auto ref = free_list_find_reverse_smallest_fit(container, count);
  if (ref == rend(container))
  {
    return {};
  }

  auto r = *ref;
  if (ref->size() == count)
  {
    container.erase(ref);
  }
  else
  {
    ref->stop = r.start = r.stop - count;
  }
  return r;
}

template <typename C, typename S>
inline auto free_list_find_reverse_first_fit(C &container, S count)
{
  using eastl::find_if;
  using eastl::rbegin;
  using eastl::rend;

  return find_if(rbegin(container), rend(container),
    [count](auto range) //
    { return range.size() >= count; });
}

template <typename R, typename C, typename S>
inline ValueRange<R> free_list_allocate_reverse_first_fit(C &container, S count)
{
  using eastl::rend;
  auto ref = free_list_find_reverse_first_fit(container, count);
  if (ref == rend(container))
  {
    return {};
  }

  auto r = *ref;
  if (ref->size() == count)
  {
    container.erase(ref);
  }
  else
  {
    ref->start = r.stop = r.start + count;
  }
  return r;
}

template <typename R, typename C, typename S>
inline bool free_list_try_resize(C &container, const ValueRange<R> &range, S new_count)
{
  if (range.size() == new_count)
    return true;

  if (new_count > range.size())
  {
    auto next_free_list =
      eastl::find_if(begin(container), end(container), [range](auto free_range) { return free_range.start == range.stop; });

    int32_t size_increase = new_count - range.size();
    if (end(container) != next_free_list && (next_free_list->size() >= size_increase))
    {
      if (size_increase == next_free_list->size())
      {
        container.erase(next_free_list);
      }
      else
      {
        next_free_list->start += size_increase;
      }
      return true;
    }

    return false;
  }
  else
  {
    free_list_insert_and_coalesce(container, ValueRange<R>(range.start + new_count, range.stop));
    return true;
  }
}

class FragmentationCalculatorContext
{
  size_t maxValue = 0;
  size_t totalValue = 0;

public:
  template <typename C>
  void fragments(C &container)
  {
    for (auto &&range : container)
    {
      fragment(range.size());
    }
  }

  void fragment(size_t value)
  {
    maxValue = max(maxValue, value);
    totalValue += value;
  }

  uint32_t valueRough() const
  {
    if (totalValue > 0)
    {
      return static_cast<uint32_t>(100 - (maxValue * 100) / totalValue);
    }
    return 0;
  }

  float valuePrecise() const
  {
    if (totalValue > 0)
    {
      return 100.f - (maxValue * 100.f) / totalValue;
    }
    return 0.f;
  }

  size_t totalSize() const { return totalValue; }

  size_t maxSize() const { return maxValue; }
};

template <typename C>
inline uint32_t free_list_calculate_fragmentation(C &container)
{
  FragmentationCalculatorContext ctx;
  ctx.fragments(container);
  return ctx.valueRough();
}

template <typename C>
inline float free_list_calculate_fragmentation_precise(C &container)
{
  FragmentationCalculatorContext ctx;
  ctx.fragments(container);
  return ctx.valuePrecise();
}
