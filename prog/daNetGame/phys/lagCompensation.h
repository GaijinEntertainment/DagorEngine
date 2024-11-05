// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>

#define NO_LAG_COMPENSATION_TIME 0.f

class BasePhysActor;

enum class LCError
{
  Ok,
  UnknownEntity,  // Not found history for requested entity
  NoPhysActor,    // Can't get phys actor component for passed entity
  EntityIsDead,   // Entity is dead, lag compensation is not applicable
  TimeNotFound,   // Not found suitable interpolation pair for given time
  TimeGapIsTooBig // Time gap of found interpolation pair is too big. Perhaps server lagged.
};

class ILagCompensationMgr
{
public:
  virtual void startLagCompensation(float to_time, ecs::EntityId except_eid) = 0;
  virtual LCError backtrackEntity(ecs::EntityId eid, float to_time) = 0;
  virtual void finishLagCompensation() = 0;
};
ILagCompensationMgr &get_lag_compensation();

bool is_need_to_lag_compensate();
float lag_compensation_time(ecs::EntityId avatar_eid,
  ecs::EntityId lc_eid,
  float at_time,
  int interp_delay_ticks_packed,
  float additional_interp_delay = 0.f,
  BasePhysActor **out_lc_eid_phys_actor = nullptr);
