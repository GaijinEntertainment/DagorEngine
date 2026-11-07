// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <debug/dag_debug.h>

namespace bvh
{
bool connect_lru_collision(ContextId, LRURendinstCollision *, lru_collision_gather_fn, const LruCollisionSettings &)
{
  logerr("[BVH] connect_lru_collision: LRU collision is compiled out (BVHSupport = no or BVHLruCollision = no)");
  return false;
}
void invalidate_lru_collision(ContextId) {}
void set_lru_collision_range(ContextId, float, float) {}
LruCollisionStats get_lru_collision_stats(ContextId) { return {}; }
} // namespace bvh
