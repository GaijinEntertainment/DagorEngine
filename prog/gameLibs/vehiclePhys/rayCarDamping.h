// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>
#include <phys/dag_physics.h>

class RayCar;
class DataBlock;

//== FIXME: dummy RayCar arguments are here to enable linking with multiple physics engines at the same time, but they are not needed

class RayCarDamping
{
public:
  RayCarDamping(RayCar * /*dummy*/ = NULL);

  void loadParams(const DataBlock *blk, const DataBlock *def_blk, RayCar * /*dummy*/ = NULL);

  void update(RayCar *car, float dt);
  void resetTimer(RayCar * /*dummy*/ = NULL);

  void disable(real for_time) { disableDampingTimer = for_time; }
  void disableFull(bool set) { disableDamping = set; }

  void onCollision(RayCar *car, PhysBody *other, const Point3 &norm_imp);

private:
  real timeSinceCrash, disableDampingTimer;
  bool disableDamping;

  real crashDampingFactor, crashDampingModifStatic, crashDampingModif;
  real crashDampingMinKmh, maxTimeSinceCrash, minCrashNormImpSq;
};
