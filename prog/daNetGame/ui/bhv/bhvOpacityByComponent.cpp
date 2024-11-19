// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvOpacityByComponent.h"

#include <daRg/dag_element.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>


SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvOpacityByComponent, bhv_opacity_by_component, cstr);

using namespace darg;

BhvOpacityByComponent::BhvOpacityByComponent() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}

void BhvOpacityByComponent::onAttach(Element *elem)
{
  const auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  if (elem->isHidden())
    return;

  eastl::string opacityComponentName =
    elem->props.scriptDesc.RawGetSlotValue<eastl::string>(strings->opacityComponentName, "ui__opacityComponent");
  ecs::hash_str_t opacityComponentHash = ecs_str_hash(opacityComponentName.c_str());
  elem->props.storage.SetValue(strings->opacityComponentHash, opacityComponentHash);
}

int BhvOpacityByComponent::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  const auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  if (elem->isHidden())
    return 0;

  const ecs::EntityId entity =
    elem->props.scriptDesc.RawGetSlotValue<ecs::EntityId>(strings->opacityComponentEntity, ecs::INVALID_ENTITY_ID);
  if (entity == ecs::INVALID_ENTITY_ID)
    return 0;

  eastl::string opacityComponentName =
    elem->props.scriptDesc.RawGetSlotValue<eastl::string>(strings->opacityComponentName, "ui__opacityComponent");
  ecs::hash_str_t opacityComponentHash = elem->props.storage.RawGetSlotValue<ecs::hash_str_t>(strings->opacityComponentHash, 0);
  ecs::HashedConstString opacityComponentNameHashed{opacityComponentName.c_str(), opacityComponentHash};
  float opacity = g_entity_mgr->getOr(entity, opacityComponentNameHashed, -1.0f);
  if (opacity < 0.0)
    return 0;

  elem->props.setCurrentOpacity(opacity);
  return 0;
}
