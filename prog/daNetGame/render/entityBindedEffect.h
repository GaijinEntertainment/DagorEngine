// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ecs/core/entitySystem.h>
#include <math/dag_color.h>

namespace AnimV20
{
class AnimcharBaseComponent;
}

class CollisionResource;

bool get_animchar_collision_transform(const ecs::EntityId eid,
  const CollisionResource *&collision,
  const AnimV20::AnimcharBaseComponent *&eid_animchar,
  TMatrix &eid_transform);

ecs::EntityId spawn_fx(const TMatrix &fx_tm, const int fx_id);
ecs::EntityId spawn_human_binded_fx(const TMatrix &fx_tm,
  const TMatrix &itm,
  const int fx_id,
  const ecs::EntityId eid,
  const int node_coll_id,
  const Color4 &color_mult = Color4(1.f, 1.f, 1.f, 1.f),
  const char *tmpl = "entity_binded_effect");
