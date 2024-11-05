// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>

namespace bvh::dyn
{
void init() {}
void teardown() {}
void init(ContextId) {}
void teardown(ContextId) {}
void on_unload_scene(ContextId) {}
void update_dynrend_instances(ContextId, dynrend::ContextId, const Point3 &) {}
void set_up_dynrend_context_for_processing(dynrend::ContextId) {}
} // namespace bvh::dyn