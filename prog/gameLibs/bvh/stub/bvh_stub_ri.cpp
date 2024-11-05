// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>

namespace bvh::ri
{
void init() {}
void teardown() {}
void init(ContextId) {}
void teardown(ContextId) {}
void on_unload_scene(ContextId) {}
void prepare_ri_extra_instances() {}
void update_ri_gen_instances(ContextId, RiGenVisibility *, const Point3 &) {}
void update_ri_extra_instances(ContextId, const Point3 &, const Frustum &) {}
void wait_ri_gen_instances_update(ContextId) {}
void wait_ri_extra_instances_update(ContextId) {}
void cut_down_trees(ContextId) {}
void wait_cut_down_trees() {}
} // namespace bvh::ri