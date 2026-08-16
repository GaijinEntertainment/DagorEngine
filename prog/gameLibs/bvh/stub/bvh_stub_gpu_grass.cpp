// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <dag/dag_vector.h>

namespace bvh::gpugrass
{
void init() {}
void teardown() {}
void init(ContextId) {}
void teardown(ContextId) {}
void on_unload_scene(ContextId) {}
void generate_instances(ContextId, bool) {}
void make_meta(ContextId, const GPUGrassBase &) {}
void process_omm(ContextId) {}
void get_instances(ContextId, Sbuffer *&, Sbuffer *&) {}
void collect_blas_addresses(ContextId, dag::Vector<uint64_t> &) {}
void get_memory_statistics(ContextId, int &gpuGrassCount, int64_t &gpuGrassMemory, int64_t &gpuGrassTexturesMemory)
{
  gpuGrassCount = gpuGrassMemory = gpuGrassTexturesMemory = 0;
}
} // namespace bvh::gpugrass