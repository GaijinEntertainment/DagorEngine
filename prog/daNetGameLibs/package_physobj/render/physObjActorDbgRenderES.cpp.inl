// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../physObj.h"
#include <ecs/core/entitySystem.h>
#include <ecs/render/updateStageRender.h>

ECS_NO_ORDER
ECS_TAG(render, dev)
static inline void debug_draw_phys_phys_obj_es(const UpdateStageInfoRenderDebug &info, PhysObjActor &phys_obj_net_phys)
{
  G_UNUSED(info);
  phys_obj_net_phys.phys.drawDebug();
}
