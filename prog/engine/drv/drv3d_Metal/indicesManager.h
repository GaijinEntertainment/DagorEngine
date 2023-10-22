
#pragma once

#include <generic/dag_tab.h>
#include <osApiWrappers/dag_critSec.h>

template <class T>
class IndicesManager
{
  IMemAlloc *mmem;

  T **ptrs;
  int count;

  int *free_indices;
  int free_indices_count;

  int reserved;

  int expand_size;

  CritSecStorage critSec;

public:
  IndicesManager(IMemAlloc *mm, int set_expand_size)
  {
    ::create_critical_section(critSec);

    mmem = mm;

    count = 0;
    free_indices_count = 0;

    expand_size = set_expand_size;

    reserved = expand_size;

    ptrs = (T **)mmem->alloc(reserved * sizeof(ptrs));
    free_indices = (int *)mmem->alloc(reserved * sizeof(int));
  }

  ~IndicesManager()
  {
    mmem->free(ptrs);
    mmem->free(free_indices);

    ::destroy_critical_section(critSec);
  }

  int getCount() { return count; }

  T *getObj(int index)
  {
    if (index < 0 || index >= count)
    {
      return 0;
    }

    return ptrs[index];
  }

  int getFreeIndex(T *obj)
  {
    if (free_indices_count > 0)
    {
      free_indices_count--;

      int index = free_indices[free_indices_count];
      ptrs[index] = obj;

      return index;
    }

    count++;

    if (count >= reserved)
    {
      reserved += expand_size;

      ptrs = (T **)mmem->realloc(ptrs, reserved * sizeof(ptrs));
      free_indices = (int *)mmem->realloc(free_indices, reserved * sizeof(int));
    }

    ptrs[count - 1] = obj;

    return count - 1;
  }

  T *freeIndex(int index)
  {
    if (index < 0 || index >= count)
    {
      return NULL;
    }

    free_indices[free_indices_count] = index;
    free_indices_count++;

    T *obj = ptrs[index];
    ptrs[index] = NULL;

    return obj;
  }

  void lock() { ::enter_critical_section(critSec); }

  void unlock() { ::leave_critical_section(critSec); }
};