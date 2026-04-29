// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>

namespace bvh::gpugrass
{
void init() {}
void teardown() {}
void init(ContextId) {}
void teardown(ContextId) {}
void on_unload_scene(ContextId) {}
void generate_instances(ContextId, bool) {}
void make_meta(ContextId, const GPUGrassBase &) {}
void get_instances(ContextId, Sbuffer *&, Sbuffer *&) {}
void get_memory_statistics(ContextId, int &gpuGrassCount, int64_t &gpuGrassMemory, int64_t &gpuGrassTexturesMemory)
{
  gpuGrassCount = gpuGrassMemory = gpuGrassTexturesMemory = 0;
}
} // namespace bvh::gpugrass