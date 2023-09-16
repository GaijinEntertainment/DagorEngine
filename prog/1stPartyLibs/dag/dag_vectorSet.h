/*
 * Dagor Engine
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 *
 * (for conditions of distribution and use, see EULA in "prog/eula.txt")
*/

#ifndef _DAGOR_DAG_VECTOR_SET_H_
#define _DAGOR_DAG_VECTOR_SET_H_
#pragma once

#include <dag/dag_vector.h>
#include <EASTL/vector_set.h>

namespace dag
{
  template <typename Key, typename Compare = eastl::less<Key>, typename Allocator = EASTLAllocatorType, //Allocator is not actually used if RandomAccessContainer has allocator
            typename RandomAccessContainer = dag::Vector<Key, Allocator> >
  using VectorSet = eastl::vector_set<Key, Compare, Allocator, RandomAccessContainer>;
}
#endif
