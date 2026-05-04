// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/component.h>
#include <daECS/core/componentsMap.h>
#include <daECS/core/entityComponent.h>
#include <ecs/render/decalsES.h>

ECS_REGISTER_RELOCATABLE_TYPE(ResizableDecalManager, nullptr)
ECS_REGISTER_RELOCATABLE_TYPE(RingBufferDecalManager, nullptr)
ECS_REGISTER_BOXED_TYPE(DecalsMatrices, nullptr)

ECS_TRACK(transform)
ECS_REQUIRE(eastl::true_type decals__useDecalMatrices)
static __forceinline void decals_es_event_handler(const ecs::Event &, ecs::EntityId eid, const TMatrix &transform)
{
  DecalsMatrices::entity_update(eid, transform);
}

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(eastl::true_type decals__useDecalMatrices)
static __forceinline void decals_delete_es_event_handler(const ecs::Event &, ecs::EntityId eid) { DecalsMatrices::entity_delete(eid); }
