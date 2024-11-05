// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_resourcePool.h>

// incremental unique ids should be build-wide unique,
// otherwise some compilers can generate multiple instances of them
static unsigned poolUIDs[(int)ResourceType::Last + 1] = {};

unsigned resource_pool_detail::getNextUniqueResourceId(ResourceType res_type) { return ++poolUIDs[(int)res_type]; }
