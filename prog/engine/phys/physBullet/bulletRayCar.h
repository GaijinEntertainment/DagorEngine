// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <phys/dag_vehicle.h>
#include <phys/dag_physics.h>
#include <generic/dag_tab.h>
#include "../physCommon/physRayCarBase_impl.h"

class BulletRbRayCar : public PhysRbRayCarBase
{
public:
  BulletRbRayCar(PhysBody *o, int iter);
  virtual ~BulletRbRayCar();

  void debugRender() override;

  btRigidBody *object;
};
