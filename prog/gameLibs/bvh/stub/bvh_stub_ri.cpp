// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>

namespace bvh::ri
{
void init(int, float, float) {}
void teardown(bool) {}
void init(ContextId) {}
void teardown(ContextId) {}
void on_unload_scene(ContextId) {}
void prepare_ri_extra_instances() {}
void update_ri_gen_instances(ContextId, const dag::Vector<RiGenVisibility *> &, const Point3 &, const Point3 &, const Frustum &,
  threadpool::JobPriority)
{}
void update_ri_extra_instances(ContextId, const Point3 &, const Frustum &, const Frustum &, threadpool::JobPriority) {}
void wait_ri_gen_instances_update(ContextId) {}
void wait_ri_extra_instances_update(ContextId) {}
void tidy_up_trees(ContextId) {}
void wait_tidy_up_trees() {}

void set_dist_mul(float) {}
void override_out_of_camera_ri_dist_mul(float) {}
void on_scene_loaded(ContextId) {}
void readdRendinst(ContextId, const RenderableInstanceLodsResource *) {}

float get_ri_lod_dist_bias() { return 0; }

void debug_update() {}

} // namespace bvh::ri