// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <EASTL/vector.h>

namespace bvh::grass
{
void init() {}
void teardown() {}
void init(ContextId) {}
void teardown(ContextId) {}
void on_unload_scene(ContextId) {}
void reload_grass(ContextId, RandomGrass *) {}
void get_instances(ContextId, Sbuffer *&, Sbuffer *&) {}
void get_memory_statistics(int64_t &vb, int64_t &ib, int64_t &blas, int64_t &meta, int64_t &queries)
{
  vb = ib = blas = meta = queries = 0;
}
UniqueBLAS *get_blas(int, int) { return nullptr; }
} // namespace bvh::grass