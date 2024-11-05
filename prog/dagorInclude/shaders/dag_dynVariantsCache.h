//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <generic/dag_span.h>

template <typename A>
class DynVariantsCache
{
  dag::Vector<int, A> cache;

public:
  DynVariantsCache();
  dag::ConstSpan<int> getCache() const { return cache; }
};

extern template class DynVariantsCache<framemem_allocator>;
extern template class DynVariantsCache<EASTLAllocatorType>;
