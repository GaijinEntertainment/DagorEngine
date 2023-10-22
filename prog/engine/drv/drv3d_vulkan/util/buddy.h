#pragma once

#include <generic/dag_tab.h>

template <typename T, T MinSize>
class Buddy
{
public:
  inline T init(T sz)
  {
    size = nextPowerOfTwo(sz);
    levels.resize(levelFor(1));
    levels[0].push_back(size);
    return size;
  }

  inline int find(T size) const
  {
    int i = levelFor(size);
    for (; i >= 0; --i)
      if (!levels[i].isEmpty())
        return i;
    return -1;
  }
  inline T allocate(T size) { return allocateFromLevel(levelFor(size)); }
  void free(T offset, T sz)
  {
    int level = levelFor(sz);
    uint32_t index = (offset / (size >> level)) >> 1;
    for (int i = 0; i < levels[level].size(); ++i)
    {
      if (((levels[level][i] / (size >> level)) >> 1) == index)
      {
        offset = offset < levels[level][i] ? offset : levels[level][i];
        levels[level].erase(i, 1);
        free(level - 1, offset);
        return;
      }
    }

    levels[level].push_back(offset);
  }

private:
  inline T getSizeFromLevel(int level) const { return size >> level; }
  inline T allocateFromLevel(int level)
  {
    T result;
    if (levels[level].isEmpty())
    {
      G_ASSERT(level > 0);
      result = allocateFromLevel(level - 1);
      levels[level].push_back(result + (size >> level));
    }
    else
    {
      result = levels[level][levels[level].size() - 1];
      levels[level].pop_back();
    }
    return result;
  }
  static inline uint64_t nextPowerOfTwo(uint64_t u)
  {
    --u;
    u |= u >> 1;
    u |= u >> 2;
    u |= u >> 4;
    u |= u >> 8;
    u |= u >> 16;
    u |= u >> 32;
    return ++u;
  }
  static inline uint32_t nextPowerOfTwo(uint32_t u)
  {
    --u;
    u |= u >> 1;
    u |= u >> 2;
    u |= u >> 4;
    u |= u >> 8;
    u |= u >> 16;
    return ++u;
  }
  inline int levelFor(T sz) const
  {
    if (sz < MinSize)
      sz = MinSize;

    int level = 0;
    for (; sz <= (size >> level); ++level)
      ;
    return level - 1;
  }
  T size;
  Tab<Tab<T>> levels;
};