// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvDistToPriority.h"
#include "ui/uiShared.h"
#include <ecs/scripts/sqEntity.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_element.h>
#include <3d/dag_render.h>
#include <ecs/core/entityManager.h>


using namespace darg;


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvDistToPriority, bhv_dist_to_priority, cstr);


BhvDistToPriority::BhvDistToPriority() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


int BhvDistToPriority::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  Sqrat::Object data = elem->props.scriptDesc.RawGetSlot(elem->csk->data);
  if (data.IsNull())
    return 0;

  ecs::EntityId eid = data.RawGetSlotValue(strings->eid, ecs::INVALID_ENTITY_ID);
  if (eid == ecs::INVALID_ENTITY_ID)
    return 0;
  const TMatrix *transform = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform_lastFrame"));
  if (!transform)
    transform = g_entity_mgr->getNullable<TMatrix>(eid, ECS_HASH("transform"));
  if (transform)
  {
    Point3 pos = transform->getcol(3);
    float dist = length(uishared::view_itm.getcol(3) - pos);
    elem->props.storage.SetValue(elem->csk->priority, -dist);
    return 0;
  }

  return 0;
}
