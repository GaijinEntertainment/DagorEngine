// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <debug/dag_debug.h>

#include "../bvh_context.h"

namespace bvh::lru_collision
{
void teardown(ContextId) {}
void on_unload_scene(ContextId) {}
void update(ContextId, const Point3 &) {}
const dag::Vector<NativeInstance> &get_instances(ContextId, const Point3 &)
{
  static const dag::Vector<NativeInstance> empty;
  return empty;
}
void bind_resources(ContextId) {}
} // namespace bvh::lru_collision

namespace bvh
{
bool connect_lru_collision(ContextId, LRURendinstCollision *, lru_collision_gather_fn, const LruCollisionSettings &)
{
  logerr("[BVH] connect_lru_collision: the LruCollision module is compiled out (BVHLruCollision = no)");
  return false;
}
void remove_lru_collision(ContextId) {}
void invalidate_lru_collision(ContextId) {}
void set_lru_collision_range(ContextId, float, float) {}
LruCollisionStats get_lru_collision_stats(ContextId) { return {}; }
} // namespace bvh
