//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/resourceSlot/nodeHandleWithSlotsAccessVector.h>
#include <daECS/core/entityManager.h>

template <>
struct ecs::TypeCopyConstructible<resource_slot::NodeHandleWithSlotsAccessVector>
{
  static constexpr bool value = false;
};

template <>
struct ecs::TypeCopyAssignable<resource_slot::NodeHandleWithSlotsAccessVector>
{
  static constexpr bool value = false;
};

template <>
struct ecs::TypeComparable<resource_slot::NodeHandleWithSlotsAccessVector>
{
  static constexpr bool value = false;
};

ECS_DECLARE_RELOCATABLE_TYPE(resource_slot::NodeHandleWithSlotsAccessVector);