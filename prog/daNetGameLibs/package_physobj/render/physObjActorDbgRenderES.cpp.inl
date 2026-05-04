// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../physObj.h"
#include <daECS/core/entitySystem.h>
#include <daECS/core/ecsQuery.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/entityManager.h>
#include <ecs/render/updateStageRender.h>

ECS_NO_ORDER
ECS_TAG(render, dev)
static inline void debug_draw_phys_phys_obj_es(const UpdateStageInfoRenderDebug &info, PhysObjActor &phys_obj_net_phys)
{
  G_UNUSED(info);
  phys_obj_net_phys.phys.drawDebug();
}
