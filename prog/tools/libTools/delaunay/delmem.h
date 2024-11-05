// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_mem.h>

namespace delaunay
{
extern IMemAlloc *delmem;
template <class T>
void delete_obj(T *p)
{
  p->~T();
  delmem->free(p);
}
}; // namespace delaunay
