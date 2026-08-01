// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvOpacityByComponent.h"

#include <daRg/dag_element.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <ecs/scripts/sqEntity.h>

SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvOpacityByComponent, bhv_opacity_by_component, cstr);

using namespace darg;

// Keeps the component name alive so update() can build HashedConstString without refetching it
struct BhvOpacityByComponentData
{
  eastl::string componentName;
  ecs::hash_str_t componentHash = 0;
};

BhvOpacityByComponent::BhvOpacityByComponent() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}

void BhvOpacityByComponent::onAttach(Element *elem)
{
  const auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  if (elem->isHidden())
    return;

  BhvOpacityByComponentData *bhvData = new BhvOpacityByComponentData();
  bhvData->componentName =
    elem->props.scriptDesc.RawGetSlotValue<eastl::string>(strings->opacityComponentName, "ui__opacityComponent");
  bhvData->componentHash = ecs_str_hash(bhvData->componentName.c_str());
  elem->props.storage.SetValue(strings->opacityComponentData, bhvData);
}

void BhvOpacityByComponent::onDetach(Element *elem, DetachMode)
{
  const auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  BhvOpacityByComponentData *bhvData =
    elem->props.storage.RawGetSlotValue<BhvOpacityByComponentData *>(strings->opacityComponentData, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(strings->opacityComponentData);
    delete bhvData;
  }
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

  BhvOpacityByComponentData *bhvData =
    elem->props.storage.RawGetSlotValue<BhvOpacityByComponentData *>(strings->opacityComponentData, nullptr);
  if (!bhvData)
    return 0;

  ecs::HashedConstString opacityComponentNameHashed{bhvData->componentName.c_str(), bhvData->componentHash};
  float opacity = g_entity_mgr->getOr(entity, opacityComponentNameHashed, -1.0f);
  if (opacity < 0.0)
    return 0;

  elem->props.setCurrentOpacity(opacity);
  return 0;
}
