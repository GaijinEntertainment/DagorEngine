// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <vecmath/dag_vecMath.h>

namespace bvh::ri
{
vec4f ri_tree_anim_max_distance_sq_v = v_zero();

void init(const AdditionalSettings &) {}
void teardown(bool) {}
void init(ContextId) {}
void teardown(ContextId) {}
void on_unload_scene(ContextId) {}
void prepare_ri_extra_instances() {}
void update_ri_gen_instances(ContextId, const dag::Vector<RiGenVisibility *> &, const Point3 &, const Point3 &, const Frustum &,
  threadpool::JobPriority)
{}
void update_ri_extra_instances(ContextId, const Point3 &, const Frustum &, const Frustum &, const Point3 &, threadpool::JobPriority) {}
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