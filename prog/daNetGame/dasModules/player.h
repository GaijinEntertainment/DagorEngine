// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>

#include "game/player.h"

namespace bind_dascript
{
struct LookingAt
{
  const Point3 *pos;
  const Point3 *dir;
};

inline LookingAt player_get_looking_at(ecs::EntityId player_eid)
{
  LookingAt lookingAt{};
  lookingAt.pos = game::player_get_looking_at(player_eid, &lookingAt.dir);
  return lookingAt;
}
} // namespace bind_dascript