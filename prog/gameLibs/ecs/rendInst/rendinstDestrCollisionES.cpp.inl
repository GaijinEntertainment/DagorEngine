// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/rendinstDestr.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/delayedAct/actInThread.h>

ECS_BEFORE(start_async_phys_sim_es)
ECS_TAG(gameClient)
static void process_ri_destr_collision_queue_es(const ParallelUpdateFrameDelayed &) { rendinstdestr::process_ri_collision_queue(); }
