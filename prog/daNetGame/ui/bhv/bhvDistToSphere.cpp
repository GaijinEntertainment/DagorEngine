// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvDistToSphere.h"

#include <ecs/scripts/sqEntity.h>

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_renderObject.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>

#include <3d/dag_render.h>

#include "ui/uiShared.h"


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvDistToSphere, bhv_dist_to_sphere, cstr);


BhvDistToSphere::BhvDistToSphere() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


int BhvDistToSphere::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  TIME_PROFILE(bhv_dist_to_sphere_update);

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  bool wasHidden = elem->isHidden();

  Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  ecs::EntityId targetEid = scriptDesc.RawGetSlotValue<ecs::EntityId>(strings->targetEid, ecs::INVALID_ENTITY_ID);
  if (targetEid == ecs::INVALID_ENTITY_ID || !uishared::has_valid_world_3d_view())
    elem->setHidden(true);
  else if (auto transform = g_entity_mgr->getNullable<TMatrix>(targetEid, ECS_HASH("transform")))
  {
    float sphereRadius = scriptDesc.RawGetSlotValue(strings->radius, 0);
    float dist = length(uishared::view_itm.getcol(3) - transform->getcol(3)) - sphereRadius;
    float minDistance = scriptDesc.RawGetSlotValue(strings->minDistance, 0);
    if (dist > minDistance)
    {
      if (fabsf(float(atof(elem->props.text.c_str())) - floorf(dist + 0.5f)) > 1e-3f)
        discard_text_cache(elem->robjParams);
      elem->props.text.sprintf("%.f", dist);
      elem->setHidden(false);
    }
    else
      elem->setHidden(true);
  }
  else
    elem->setHidden(true);

  return (elem->isHidden() != wasHidden) ? darg::R_REBUILD_RENDER_AND_INPUT_LISTS : 0;
}
