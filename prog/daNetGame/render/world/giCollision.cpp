// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include <gameRes/dag_collisionResource.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include "private_worldRenderer.h"


const CollisionResource *lru_collision_get_collres(uint32_t i)
{
  if (i == 0)
    return static_cast<WorldRenderer *>(get_world_renderer())->getStaticSceneCollisionResource();
  return rendinst::getRIGenExtraCollRes(i - 1);
}

mat43f lru_collision_get_transform(rendinst::riex_handle_t h)
{
  if (DAGOR_UNLIKELY(h == rendinst::RIEX_HANDLE_NULL))
  {
    mat43f f;
    f.row0 = V_C_UNIT_1000;
    f.row1 = V_C_UNIT_0100;
    f.row2 = V_C_UNIT_0010;
    return f;
  }
  return rendinst::getRIGenExtra43(h);
}
uint32_t lru_collision_get_type(rendinst::riex_handle_t h) { return rendinst::handle_to_ri_type(h) + 1u; }
