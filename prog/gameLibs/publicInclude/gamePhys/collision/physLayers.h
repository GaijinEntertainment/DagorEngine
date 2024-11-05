//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace dacoll
{

enum PhysLayer : int
{
  EPL_DEFAULT = 1 << 0,
  EPL_STATIC = 1 << 1,
  EPL_KINEMATIC = 1 << 2,
  EPL_DEBRIS = 1 << 3,
  EPL_SENSOR = 1 << 4,
  EPL_CHARACTER = 1 << 5,

  EPL_ALL = -1
};

static constexpr int DEFAULT_DYN_COLLISION_MASK = (EPL_ALL & ~(EPL_KINEMATIC | EPL_STATIC));

}; // namespace dacoll
