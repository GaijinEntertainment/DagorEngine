// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "BhvRotateRelativeToDir.h"

#include <ecs/scripts/sqEntity.h>
#include <math/dag_mathAng.h>

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_transform.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>

#include "ui/userUi.h"


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvRotateRelativeToDir, bhv_rotate_relative_to_dir, cstr);

using namespace darg;

BhvRotateRelativeToDir::BhvRotateRelativeToDir() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}


int BhvRotateRelativeToDir::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  const auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  if (elem->isHidden())
    return 0;

  const Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  const ecs::EntityId targetEid = scriptDesc.RawGetSlotValue<ecs::EntityId>(strings->targetEid, ecs::INVALID_ENTITY_ID);
  const float additiveAngle = scriptDesc.GetSlotValue<float>(strings->additiveAngle, 0.0);
  const Point3 elementDirection = g_entity_mgr->getOr(targetEid, ECS_HASH("transform"), TMatrix::IDENT).getcol(0);
  const float lookAng = dir_to_angles(elementDirection).x;
  elem->transform->rotate = lookAng + additiveAngle;
  return 0;
}