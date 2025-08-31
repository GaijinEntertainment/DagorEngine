// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <phys/dag_vehicle.h>
#include <phys/dag_physics.h>
#include <generic/dag_tab.h>
#include "../physCommon/physRayCarBase_impl.h"

class JoltRbRayCar : public PhysRbRayCarBase
{
public:
  JoltRbRayCar(PhysBody *o, int iter);
  virtual ~JoltRbRayCar();

  void debugRender() override;

protected:
  JPH::BodyID object;
};