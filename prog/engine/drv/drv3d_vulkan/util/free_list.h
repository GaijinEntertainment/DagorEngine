// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>

#include "vulkan_api.h"

template <typename T>
class FreeList
{
public:
  inline void init(T size) { free(0, size); }

  inline T getFreeSize()
  {
    T ret = 0;
    for (int i = 0; i < sections.size(); ++i)
      ret += sections[i].size;
    return ret;
  }

  inline int findFirst(T size, T alignment) const
  {
    for (int i = 0; i < sections.size(); ++i)
      if (sections[i].checkSize(alignment, size))
        return i;
    return -1;
  }
  inline int findBest(T size, T alignment) const
  {
    int i;
    for (i = 0; i < sections.size(); ++i)
    {
      if (sections[i].checkSize(alignment, size))
        break;
    }

    if (i < sections.size())
    {
      T os = sections[i].overSize(alignment, size);

      for (int s = i + 1; s < sections.size() && os; ++s)
      {
        if (!sections[s].checkSize(alignment, size))
          continue;
        T osa = sections[s].overSize(alignment, size);
        if (osa < os)
        {
          i = s;
          os = osa;
        }
      }
      return i;
    }
    return -1;
  }

  inline T getSize(int block) const { return sections[block].size; }

  inline T allocate(int index, T size, T alignment)
  {
    VkDeviceSize offset = (sections[index].offset + alignment - 1) & ~(alignment - 1);
    VkDeviceSize frontSize = offset - sections[index].offset;
    VkDeviceSize backOffset = offset + size;
    VkDeviceSize backSize = (sections[index].offset + sections[index].size) - backOffset;
    if (frontSize)
    {
      sections[index].size = frontSize;
      if (backSize)
      {
        Section s = {backOffset, backSize};
        insert_item_at(sections, index + 1, s);
      }
    }
    else if (backSize)
    {
      sections[index].offset = backOffset;
      sections[index].size = backSize;
    }
    else
    {
      erase_items(sections, index, 1);
    }

    return offset;
  }
  inline void free(T offset, T size)
  {
    Section s = {offset, size};
    for (int i = 0; i < sections.size(); ++i)
    {
      if (sections[i].offset > offset)
      {
        insert_item_at(sections, i, s);
        merge();
        return;
      }
    }

    sections.push_back(s);
    merge();
  }

  inline bool checkEmpty(T total) const { return sections.size() == 1 && sections[0].offset == 0 && sections[0].size == total; }

  void reset() { clear_and_shrink(sections); }

  template <typename Cb>
  void iterate(Cb callback)
  {
    for (int i = 0; i < sections.size(); ++i)
      callback(sections[i].offset, sections[i].size);
  }

private:
  struct Section
  {
    T offset;
    T size;

    bool checkSize(T align, T sz) const
    {
      T o = (offset + align - 1) & ~(align - 1);
      T a = o - offset;
      if (a >= size)
        return false;
      T s = size - a;
      return s >= sz;
    }

    T overSize(T align, T sz) const
    {
      T o = (offset + align - 1) & ~(align - 1);
      T a = o - offset;
      T s = size - a - sz;
      return a + (s - size);
    }
  };

  // expects l.offset < r.offset
  static inline bool merge(Section &l, const Section &r)
  {
    if (l.offset + l.size == r.offset)
    {
      l.size += r.size;
      return true;
    }
    return false;
  }

  inline void merge()
  {
    for (int i = sections.size() - 1; i > 0; --i)
      if (merge(sections[i - 1], sections[i]))
        erase_items(sections, i, 1);
  }

  Tab<Section> sections;

  friend inline void swap(FreeList &left, FreeList &right) { left.sections.swap(right.sections); }
};