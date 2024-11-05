// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "waterEffects.h"
#include <fftWater/fftWater.h>
#include <render/shipWakeFx.h>
#include <daECS/core/event.h>
#include <daECS/core/entitySystem.h>
#include <ecs/core/entityManager.h>

extern bool test_box_is_in_water(FFTWater &water, const TMatrix &transform, const BBox3 &box);

inline void WaterEffects::GatherEmittersEventCtx::addEmitter(
  ecs::EntityId eid, const BBox3 &box, const TMatrix &transform, const Point3 &velocity) const
{
  if (test_box_is_in_water(water, transform, box))
  {
    ShipEffectContext effectContext;
    auto knownIter = knownShips.find(eid);
    if (knownIter == knownShips.end())
    {
      ShipWakeFx::ShipDesc desc = {box, 1.0f, 1.0f};
      desc.box.lim[1].y += desc.box.width().y * waveScale;
      effectContext.wakeId = shipWakeFx.addShip(desc);
    }
    else
      effectContext = knownIter->second;

    processedShips[eid] = effectContext;

    ShipWakeFx::ShipState state;
    state.tm = transform;
    state.velocity = velocity;
    state.frontPos = 0;
    state.wakeHeadOffset = 0;
    state.wakeHeadShift = 0;
    state.foamVelocityThresholds = eastl::make_pair(minSpeed, maxSpeed);
    shipWakeFx.setShipState(effectContext.wakeId, state);
  }
};

ECS_BROADCAST_EVENT_TYPE(GatherEmittersForWaterEffects, WaterEffects::GatherEmittersEventCtx *)
