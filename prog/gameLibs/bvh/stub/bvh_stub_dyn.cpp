// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>

namespace bvh::dyn
{
void init(int, float, bool) {}
void teardown(bool, bool) {}
void init(ContextId) {}
void teardown(ContextId) {}
void enable_dynamic_planar_decals(bool) {}
void on_unload_scene(ContextId) {}
void update_dynrend_instances(ContextId, dynrend::ContextId, dynrend::ContextId, const Point3 &) {}
void update_animchar_instances(ContextId, dynrend::ContextId, dynrend::ContextId, const Point3 &, dynrend::BVHIterateCallback) {}
void set_up_dynrend_context_for_processing(dynrend::ContextId) {}
void wait_dynrend_instances() {}
void tidy_up_skins(ContextId) {}
void wait_tidy_up_skins() {}

void debug_update() {}
} // namespace bvh::dyn