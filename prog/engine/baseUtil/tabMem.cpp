// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <util/dag_tabHlp.h>

void *dag_tab_insert2(void *ptr, uint32_t &total, uint32_t &used, IMemAlloc *mem, intptr_t at, intptr_t n, intptr_t sz, intptr_t step,
  uint32_t &out_idx)
{
  if (n <= 0)
  {
    out_idx = at;
    return ptr;
  }
  if (at > used)
    at = used;
  int nn = used + n;
  if (nn > total)
  {
    if (step)
      nn = ((nn + step - 1) / step) * step;
    else
    {
      int newtotal = total ? (total + (total + 1) / 2) : (16 / sz);
      if (newtotal > nn)
        nn = newtotal;
    }

    const size_t atSize = at * sz;
    const size_t addSize = n * sz;
    const size_t newSize = nn * sz;
    if (at == used)
    {
      total = nn;
      ptr = mem->realloc(ptr, newSize);
    }
    else // it is not append. sometimes it still make sense to reallocate, but heuristics are tricky, so just fallback to alloc/free
    {
      size_t asz;
      void *nt = mem->alloc(newSize, &asz);
      if (at)
        memcpy(nt, ptr, atSize);
      if (used - at)
        memcpy((char *)nt + atSize + addSize, (char *)ptr + atSize, (used - at) * sz);
      if (ptr)
        mem->free(ptr);
      ptr = nt;
      total = asz / sz;
    }
    used += n;
  }
  else
  {
    if (used - at)
      memmove((char *)ptr + (at + n) * sz, (char *)ptr + at * sz, (used - at) * sz);
    used += n;
  }
  out_idx = at;
  return ptr;
}

#define EXPORT_PULL dll_pull_baseutil_tabMem
#include <supp/exportPull.h>
