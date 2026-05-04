//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <animChar/dag_animCharacter2.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>

// Holds reference to original animchar resource, while entity is alive. Avoid cleaning it up right after being loaded,
// and then reloading it again, when unused resources are being freed, while creating several similar entities.
// It is also used to fix leak of AnimData in same scenario, until the core issue is fixed.
struct AnimcharResourceReferenceHolder
{
  EA_NON_COPYABLE(AnimcharResourceReferenceHolder);
  AnimcharResourceReferenceHolder(ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    if (const char *animCharResName = mgr.getOr(eid, ECS_HASH("animchar__res"), ecs::nullstr))
      animcharRes = (AnimV20::IAnimCharacter2 *)::get_game_resource_ex(animCharResName, CharacterGameResClassId);
  }
  AnimcharResourceReferenceHolder(AnimcharResourceReferenceHolder &&rhs) : animcharRes(eastl::exchange(rhs.animcharRes, nullptr)) {}
  ~AnimcharResourceReferenceHolder()
  {
    if (animcharRes)
      ::release_game_resource_ex(animcharRes, CharacterGameResClassId);
  }

  AnimV20::IAnimCharacter2 *animcharRes = nullptr;
};

ECS_DECLARE_RELOCATABLE_TYPE(AnimcharResourceReferenceHolder);
