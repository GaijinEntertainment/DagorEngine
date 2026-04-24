// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
namespace bvh::splinegen
{

void teardown(ContextId) {}
void on_unload_scene(ContextId) {}
void add_meshes(ContextId, Sbuffer *, eastl::vector<eastl::pair<uint32_t, MeshInfo>> &, uint32_t, uint32_t &) {}
void update_instances(ContextId, const Point3 &) {}

} // namespace bvh::splinegen