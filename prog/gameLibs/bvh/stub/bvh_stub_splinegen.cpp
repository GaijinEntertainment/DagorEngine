// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
namespace bvh::splinegen
{

void teardown(ContextId) {}
void on_unload_scene(ContextId) {}
void add_meshes(ContextId, Sbuffer *, const MeshInfo &, dag::ConstSpan<uint32_t>, dag::ConstSpan<uint32_t>, uint32_t &) {}
void update_instances(ContextId, const Point3 &) {}
void tidy_up_splinegen(ContextId) {}
void remove_pending_object_add(ContextId, uint32_t) {}

} // namespace bvh::splinegen