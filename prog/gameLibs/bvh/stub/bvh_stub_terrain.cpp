// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>

int bvh_terrain_lod_count = 1;

namespace bvh::terrain
{
UniqueBLAS *get_blas(int, int) {}
bool is_dirty(ContextId) { return false; }
dag::Vector<eastl::tuple<uint64_t, MeshMetaAllocator::AllocId, Point2>, framemem_allocator> get_blases(ContextId) { return {}; };
dag::Vector<Sbuffer *> get_buffers(ContextId) { return {}; };

void init() {}
void init(ContextId) {}
void teardown() {}
void teardown(ContextId) {}
} // namespace bvh::terrain
