/*
 * Dagor Engine
 * Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
 *
 * (for conditions of use see prog/license.txt)
*/

#ifndef _DAGOR_DAG_VECTOR_MAP_H_
#define _DAGOR_DAG_VECTOR_MAP_H_
#pragma once

#include <dag/dag_vector.h>
#include <EASTL/vector_map.h>

namespace dag
{
  template <typename Key, typename T, typename Compare = eastl::less<Key>,
            typename Allocator = EASTLAllocatorType, //Allocator is not actually used if RandomAccessContainer has allocator
            typename RandomAccessContainer = dag::Vector<eastl::pair<Key, T>, Allocator> >
  using VectorMap = eastl::vector_map<Key, T, Compare, Allocator, RandomAccessContainer>;
}

#endif
