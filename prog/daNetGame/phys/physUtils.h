// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/utility.h>
#include <EASTL/vector.h>
#include <EASTL/tuple.h>
#include <math/dag_mathUtils.h>
#include <gamePhys/phys/utils.h>
#include <gamePhys/phys/physActor.h>
#include "physConsts.h"

class IPhysActor;
namespace danet
{
class BitStream;
}

int phys_get_tickrate();
int phys_get_bot_tickrate();
bool phys_set_tickrate(int tickrat = -1, int bot_tickrate = -1); // negative means default
float phys_get_timestep();

inline int phys_time_to_seed(float at_time)
{
  // Use tick instead of time as it's more in sync between client & server
  int timeSeed = gamephys::nearestPhysicsTickNumber(at_time, phys_get_timestep()) >> 1; // tolerate time error in one tick
  return (timeSeed << 5) - timeSeed; // mult to prime (31) to reduce avalanche effect
}

// returns (interpTime, elapsedTime)
eastl::pair<float, float> phys_get_xpolation_times(int prev_tick, int last_tick, float timestep, float at_time);

float phys_get_present_time_delay_sec(PhysTickRateType tr_type, float time_step, bool client_side = false);
float get_timespeed_accumulated_delay_sec();

void phys_send_auth_state(IPhysActor *nu, danet::BitStream &&state_data, float);
void phys_send_part_auth_state(IPhysActor *nu, danet::BitStream &&data, int tick);
void phys_send_broadcast_auth_state(IPhysActor *nu, danet::BitStream &&state_data, float);
void phys_send_broadcast_part_auth_state(IPhysActor *nu, danet::BitStream &&data, int tick);

int get_interp_delay_ticks(PhysTickRateType tr_type);

template <typename PhysActor>
float get_interp_delay_time(const PhysActor &net_phys)
{
  const float physDt = net_phys.phys.timeStep;
  const int delayTicks = get_interp_delay_ticks(net_phys.tickrateType);
  return float(delayTicks) * physDt;
}

template <typename PhysActor>
inline float calc_interpk(const PhysActor &net_phys, float at_time)
{
  const float physDt = net_phys.phys.timeStep;

  float interpK = 0.f;
  if (net_phys.getRole() == IPhysActor::NetRole::ROLE_REMOTELY_CONTROLLED_SHADOW)
  {
    const int delayTicks = get_interp_delay_ticks(net_phys.tickrateType);
    const float interpDelay = float(delayTicks) * physDt + get_timespeed_accumulated_delay_sec();
    const float atTime = at_time - interpDelay;

    if (const typename PhysActor::SnapshotPair *spair = net_phys.findPhysSnapshotPair(atTime))
    {
      float interpTime = 1.0f, elapsedTime = 0.0f;
      eastl::tie(interpTime, elapsedTime) = phys_get_xpolation_times(spair->prevSnap.atTick, spair->lastSnap.atTick, physDt, atTime);
      interpK = saturate(safediv(elapsedTime, interpTime));
    }
  }
  else
    interpK = get_phys_interpk_clamped(net_phys.phys, at_time);
  return interpK;
}
