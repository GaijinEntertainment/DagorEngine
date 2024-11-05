// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstExtraAccess.h>
#include <ecs/core/entityManager.h>
#include <ecs/rendInst/riExtra.h>

static CollisionResource *get_collres_from_riextra(rendinst::riex_handle_t ri_extra__handle)
{
  CollisionResource *collRes = nullptr;
  rendinst::getRIExtraCollInfo(ri_extra__handle, &collRes, nullptr);
  return collRes;
}

CollisionResource *get_collres_from_riextra(ecs::EntityManager &mgr, ecs::EntityId eid)
{
  const rendinst::riex_handle_t *riHandle = mgr.getNullable<rendinst::riex_handle_t>(eid, ECS_HASH("ri_extra__handle"));
  if (riHandle == nullptr)
  {
    if (const RiExtraComponent *ri_extra = g_entity_mgr->getNullable<RiExtraComponent>(eid, ECS_HASH("ri_extra")))
      riHandle = &ri_extra->handle;
  }

  return riHandle ? get_collres_from_riextra(*riHandle) : nullptr;
}
