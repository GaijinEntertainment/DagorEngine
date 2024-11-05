// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>

namespace bullet
{
struct StaticCollision
{
  Tab<btRigidBody *> bodies;
  Tab<char *> dataBlocks;
  PhysWorld *pw;

  StaticCollision(PhysWorld *_pw) : pw(_pw), bodies(inimem), dataBlocks(inimem) {}
};

extern Tab<PhysBody *> staticBodies;
extern Tab<StaticCollision *> swcRegistry;
} // namespace bullet
