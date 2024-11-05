// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "BhvRotateByComponent.h"

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

SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvRotateByComponent, bhv_rotate_by_component, cstr);

using namespace darg;

BhvRotateByComponent::BhvRotateByComponent() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0), componentNameHash() {}

void BhvRotateByComponent::onAttach(Element *elem)
{
  const auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  if (elem->isHidden())
    return;

  const Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  eastl::string rotationComponentName =
    scriptDesc.RawGetSlotValue<eastl::string>(strings->rotationComponentName, "ui__rotateElementByAngle");
  componentNameHash = ECS_HASH_SLOW(rotationComponentName.c_str());
}

int BhvRotateByComponent::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  const auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  if (elem->isHidden())
    return 0;

  const Sqrat::Table &scriptDesc = elem->props.scriptDesc;
  const ecs::EntityId entity = scriptDesc.RawGetSlotValue<ecs::EntityId>(strings->rotationComponentEntity, ecs::INVALID_ENTITY_ID);
  const float angle = g_entity_mgr->getOr(entity, componentNameHash, 0.0f);
  elem->transform->rotate = angle;
  return 0;
}
