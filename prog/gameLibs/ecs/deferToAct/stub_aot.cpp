// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/deferToAct/deferToAct.h>
#include <debug/dag_assert.h>

void defer_to_act(das::Context * /*context*/, const eastl::string & /*func_name*/) { G_ASSERT(0); }
void clear_deferred_to_act(das::Context * /*context*/) {}
