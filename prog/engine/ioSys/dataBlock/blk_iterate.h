#pragma once

#include <ioSys/dag_dataBlock.h>
#include <EASTL/deque.h>

template <typename Cb, typename Alloc = eastl::allocator>
static inline void iterate_blocks_bfs(const DataBlock &db, Cb cb)
{
  eastl::deque<const DataBlock *, Alloc> queue;
  queue.push_back(&db);
  while (!queue.empty())
  {
    auto node = queue.front();
    queue.pop_front();
    cb(*node);
    for (uint32_t i = 0, e = node->blockCount(); i < e; ++i)
      if (auto b = node->getBlock(i))
        queue.push_back(b);
  }
}
