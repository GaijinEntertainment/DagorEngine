//
// Dagor Engine 6.5 - 1st party libs
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <EASTL/vector_set.h>

namespace dag
{
  template <typename Key, typename Compare = eastl::less<Key>, typename Allocator = EASTLAllocatorType, //Allocator is not actually used if RandomAccessContainer has allocator
            typename RandomAccessContainer = dag::Vector<Key, Allocator> >
  using VectorSet = eastl::vector_set<Key, Compare, Allocator, RandomAccessContainer>;
}
