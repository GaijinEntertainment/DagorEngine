// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <dag/dag_vector.h>

namespace bvh
{

namespace fx
{

void init(void) {}
void teardown(void) {}
void collect_blas_addresses(dag::Vector<uint64_t> &) {}
void init(struct bvh::Context *) {}
void teardown(struct bvh::Context *) {}
void get_instances(struct bvh::Context *, class Sbuffer *&, class Sbuffer *&) {}
void on_unload_scene(struct bvh::Context *) {}
void connect(void(__cdecl *)(struct BVHConnection *)) {}

} // namespace fx

} // namespace bvh