// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/walker/humanPhysUtils.h>
#include <math/dag_mathUtils.h>

Point3 gamephys::accelerate_directional(const Point3 &cur_vel, const Point3 &wish_dir, float wish_spd, float accel, float dt)
{
  float curSpd = cur_vel * wish_dir;
  if (wish_spd <= curSpd) // we don't need to accelerate in that direction further, we already have proper velocity in direction
    return cur_vel;
  return cur_vel + wish_dir * min(accel * dt, wish_spd - curSpd);
}

Point3 gamephys::accelerate_directional_clamp(const Point3 &cur_vel, const Point3 &wish_dir, float wish_spd, float accel, float dt)
{
  Point3 resVel = accelerate_directional(cur_vel, wish_dir, wish_spd, accel, dt);
  float lenSq = lengthSq(resVel);
  if (lenSq > sqr(wish_spd))
    resVel *= safediv(wish_spd, sqrtf(lenSq));
  return resVel;
}

Point3 gamephys::accelerate_geometrical(const Point3 &cur_vel, const Point3 &wish_dir, float wish_spd, float accel, float dt)
{
  float curSpd = cur_vel * wish_dir;
  if (wish_spd <= curSpd) // we don't need to accelerate in that direction further, we already have proper velocity in direction
    return cur_vel;
  Point3 wishVel = wish_dir * wish_spd;
  Point3 delta = wishVel - cur_vel;
  float deltaLen = length(delta);
  return cur_vel + delta * safeinv(deltaLen) * min(accel * dt, deltaLen);
}

float gamephys::calc_friction_mult(float cur_spd, float thres_spd, float friction_k, float dt)
{
  float fricSpd = max(cur_spd, thres_spd);
  float deltaSpd = min(fricSpd * friction_k * dt, cur_spd);
  return safediv(cur_spd - deltaSpd, cur_spd);
}

Point3 gamephys::apply_friction(const Point3 &cur_vel, float thres_spd, float friction_k, float dt)
{
  return cur_vel * calc_friction_mult(length(Point2::xz(cur_vel)), thres_spd, friction_k, dt);
}

Point3 gamephys::apply_omni_friction(const Point3 &cur_vel, float thres_spd, float friction_k, float dt)
{
  return cur_vel * calc_friction_mult(length(cur_vel), thres_spd, friction_k, dt);
}
