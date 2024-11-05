//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <math/dag_Quat.h>
#include <math/dag_mathUtils.h>

namespace danet
{
class BitStream;
};

namespace gamephys
{
// Histories
bool readHistory32(const danet::BitStream &bs, uint32_t &data);
void writeHistory32(danet::BitStream &bs, uint32_t data);

// Time
double alignTime(double time, float fixed_dt);
inline int floorPhysicsTickNumber(float time, float fixed_dt)
{
  return int(time / fixed_dt); // conversion to int always truncates (round-to-zero)
}
inline int nearestPhysicsTickNumber(float time, float fixed_dt)
{
#if _TARGET_SIMD_SSE > 0
  return _mm_cvtss_si32(_mm_set_ss(time / fixed_dt));
#else
  return int(time / fixed_dt + 0.5f);
#endif
}
inline int ceilPhysicsTickNumber(float time, float fixed_dt)
{
  // Note: difference from std ceil() - for exactly divisible by fixed_dt time N this gives N+1 tick (i.e. 123.0 -> 124)
  // This is intentional, since this is how our controls/physics works
  return floorPhysicsTickNumber(time, fixed_dt) + 1;
}

// Interpolation / extrapolation
template <class T>
inline void calc_pos_vel_at_time(double at_time, const T &pos1, const T &vel1, double time1, const T &pos2, const T &vel2,
  double time2, T &out_pos, T &out_vel)
{
  const double step = time2 - time1;
  const double dt = at_time - time1;
  const double stepPart = dt / step;
  if (time1 <= at_time && at_time <= time2)
  {
    // interpolation by acceleration and its derrivative
    const T deltaPos = pos2 - pos1;
    const T deltaVel = vel2 - vel1;
    const T accelDeriv = (deltaVel - step * (deltaPos - vel1 * step) / (sqr(step) * 0.5f)) /
                         (sqr(step) * 0.5f - step * (sqr(step) * step * 0.333f) / (sqr(step) * 0.5f));
    const T accelPrev = (deltaVel - accelDeriv * sqr(step) * 0.5f) / step;

    out_pos = pos1 + vel1 * dt + accelPrev * sqr(dt) * 0.5f + accelDeriv * sqr(dt) * dt * 0.333f;
    out_vel = vel1 + accelPrev * dt + accelDeriv * sqr(dt) * 0.5f;
  }
  else
  {
    // current acceleration is not used in extrapolation as it may produce wrong estimations
    out_pos = pos1 * (1.0 - stepPart) + pos2 * stepPart;
    out_vel = vel1 * (1.0 - stepPart) + vel2 * stepPart;
  }
}

template <class T>
inline void calc_quat_omega_at_time(double at_time, const Quat &quat1, const T &omega1, double time1, const Quat &quat2,
  const T &omega2, double time2, Quat &out_quat, T &out_omega)
{
  const double step = time2 - time1;
  const double dt = at_time - time1;
  const double stepPart = dt / step;
  out_quat = normalize(qinterp(quat1, quat2, stepPart));
  out_omega = omega1 * (1.0 - stepPart) + omega2 * stepPart;
}
}; // namespace gamephys

// Moved from dng/phys/physUtils.h
// TODO: Move to gamephys namespace
template <typename Phys>
inline float get_phys_interp_time(const Phys &phys, float at_time)
{
  if (int tdelta = phys.currentState.atTick - phys.currentState.lastAppliedControlsForTick - 1) // correction for delayed controls mode
    return at_time + tdelta * phys.timeStep;
  else
    return at_time;
}

template <typename Phys>
inline float get_phys_interpk_clamped(const Phys &phys, float at_time)
{
  return cvt(get_phys_interp_time(phys, at_time), phys.previousState.atTick * phys.timeStep, phys.currentState.atTick * phys.timeStep,
    0.f, 1.f);
}
