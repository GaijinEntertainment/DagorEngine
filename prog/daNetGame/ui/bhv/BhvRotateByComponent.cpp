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

// Keeps the component name alive so update() can build HashedConstString without refetching it
struct BhvRotateByComponentData
{
  eastl::string componentName;
  ecs::hash_str_t componentHash = 0;
};

BhvRotateByComponent::BhvRotateByComponent() : Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0) {}

void BhvRotateByComponent::onAttach(Element *elem)
{
  const auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  if (elem->isHidden())
    return;

  BhvRotateByComponentData *bhvData = new BhvRotateByComponentData();
  bhvData->componentName =
    elem->props.scriptDesc.RawGetSlotValue<eastl::string>(strings->rotationComponentName, "ui__rotateElementByAngle");
  bhvData->componentHash = ecs_str_hash(bhvData->componentName.c_str());
  elem->props.storage.SetValue(strings->rotationComponentData, bhvData);
}

void BhvRotateByComponent::onDetach(Element *elem, DetachMode)
{
  const auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  BhvRotateByComponentData *bhvData =
    elem->props.storage.RawGetSlotValue<BhvRotateByComponentData *>(strings->rotationComponentData, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(strings->rotationComponentData);
    delete bhvData;
  }
}

int BhvRotateByComponent::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  const auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  if (elem->isHidden())
    return 0;

  const ecs::EntityId entity =
    elem->props.scriptDesc.RawGetSlotValue<ecs::EntityId>(strings->rotationComponentEntity, ecs::INVALID_ENTITY_ID);
  if (entity == ecs::INVALID_ENTITY_ID)
    return 0;

  BhvRotateByComponentData *bhvData =
    elem->props.storage.RawGetSlotValue<BhvRotateByComponentData *>(strings->rotationComponentData, nullptr);
  if (!bhvData)
    return 0;

  ecs::HashedConstString rotationComponentNameHashed{bhvData->componentName.c_str(), bhvData->componentHash};
  const float angle = g_entity_mgr->getOr(entity, rotationComponentNameHashed, 0.0f);

  if (!elem->transform) // transform is an optional element prop
    return 0;
  elem->transform->rotate = angle;
  return 0;
}
