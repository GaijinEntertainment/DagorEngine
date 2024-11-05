//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityId.h>

MAKE_EXTERNAL_TYPE_FACTORY(EntityId, ecs::EntityId)

namespace das
{
template <>
struct cast<ecs::EntityId>
{
  static __forceinline ecs::EntityId to(vec4f x) { return ecs::EntityId(ecs::entity_id_t(v_extract_xi(v_cast_vec4i(x)))); }
  static __forceinline vec4f from(ecs::EntityId x) { return v_cast_vec4f(v_splatsi(ecs::entity_id_t(x))); }
};
}; // namespace das
