// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>

namespace bvh
{

namespace smoke_tracers
{

void init() {}
void teardown() {}
void init(ContextId) {}
void teardown(ContextId) {}
void connect(smoke_tracers_connect_callback) {}
void update_instances() {}
void get_instances(ContextId, Sbuffer *&, Sbuffer *&) {}
void set_source_buffers(Sbuffer *, Sbuffer *, Sbuffer *) {}
void get_memory_statistics(int &count, int64_t &vb, int64_t &blas)
{
  count = 0;
  vb = 0;
  blas = 0;
}

} // namespace smoke_tracers

} // namespace bvh
