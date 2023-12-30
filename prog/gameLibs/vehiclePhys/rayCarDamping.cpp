#include "rayCarDamping.h"
#include "rayCar.h"
#include <debug/dag_debug.h>

#define MAX_TIME_SINCE_CRASH 10.0f

RayCarDamping::RayCarDamping(RayCar * /*dummy*/) :
  timeSinceCrash(MAX_TIME_SINCE_CRASH),
  disableDampingTimer(0),
  disableDamping(false),
  crashDampingFactor(0.1),
  crashDampingModif(1),
  crashDampingModifStatic(0.6),
  crashDampingMinKmh(4),
  maxTimeSinceCrash(0.04),
  minCrashNormImpSq(40 * 40)
{}

void RayCarDamping::loadParams(const DataBlock *db, const DataBlock *ddb, RayCar * /*dummy*/)
{
  crashDampingFactor = db->getReal("crashDampingFactor", ddb->getReal("crashDampingFactor", 0.1));
  crashDampingModifStatic = db->getReal("crashDampingModifStatic", ddb->getReal("crashDampingModifStatic", 0.6));
  crashDampingMinKmh = db->getReal("crashDampingMinKmh", ddb->getReal("crashDampingMinKmh", 4));
  maxTimeSinceCrash = db->getReal("maxTimeSinceCrash", ddb->getReal("maxTimeSinceCrash", 0.04));

  minCrashNormImpSq = sqr(db->getReal("minCrashNormImp", ddb->getReal("minCrashNormImp", 40)));
}

void RayCarDamping::resetTimer(RayCar * /*dummy*/) { timeSinceCrash = MAX_TIME_SINCE_CRASH; }

void RayCarDamping::update(RayCar *car, float dt)
{
  if (float_nonzero(crashDampingFactor))
  {
    timeSinceCrash += dt;
    if (disableDamping)
      return;
    if (disableDampingTimer > dt)
    {
      disableDampingTimer -= dt;
      return;
    }
    if (length(car->getVel()) * 3.6f > crashDampingMinKmh && timeSinceCrash < maxTimeSinceCrash)
    {
      float f = (1.0f - timeSinceCrash) * (1.0f - timeSinceCrash) * crashDampingFactor;
      f *= crashDampingModif;
      if (f > 0)
      {
        if (f > 0.90)
          f = 0.90;
        // debug("%p.rcar->curSpeedKmh=%.3f timeSinceCrash=%.2f f=%.3f",
        //   this, rcar->curSpeedKmh, timeSinceCrash, f );

        PhysBody *phBody = car->getPhysBody();
        phBody->setAngularVelocity(phBody->getAngularVelocity() * (1 - f / 2));
        Point3 linVel = phBody->getVelocity();
        if (linVel.y > 0)
          linVel.y *= (1 - f);
        phBody->setVelocity(linVel);
      }
    }
  }
}

void RayCarDamping::onCollision(RayCar *car, PhysBody *other, const Point3 &norm_imp)
{
  if (!float_nonzero(crashDampingFactor))
    return;

  //  float norm_imp_aligned =
  //    other ? length(norm_imp): -norm_imp*normalize(car->getPhysBody()->getVelocity());

  // if ( sqr(norm_imp_aligned) > minCrashNormImpSq )
  if (lengthSq(norm_imp) > minCrashNormImpSq)
  {
    // debug("crash! imp=" FMT_P3 " (aligned=%.3f)", P3D(norm_imp), norm_imp_aligned);
    timeSinceCrash = 0;
    crashDampingModif = other ? 1.0 : crashDampingModifStatic;
  }
}
