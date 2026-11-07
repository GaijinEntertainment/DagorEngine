// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>

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
