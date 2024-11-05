// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <EASTL/vector.h>

#include "../bvh_context.h"

namespace bvh::gobj
{
void init() {}
void teardown() {}
void init(ContextId) {}
void teardown(ContextId) {}
void on_unload_scene(ContextId) {}
void get_memory_statistics(int &meta, int &queries) { meta = queries = 0; }
void get_instances(ContextId, Sbuffer *&, Sbuffer *&) {}
} // namespace bvh::gobj