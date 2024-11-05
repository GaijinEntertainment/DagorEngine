// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vectorMap.h>
#include <daECS/core/entityId.h>
#include <daECS/core/componentType.h>

class Point3;
class WaterRipples;
class BBox3;
class FFTWater;
namespace gamephys
{
struct Loc;
}

namespace water_ripples
{
void add_impulse(const Point3 &pos, float mass, float volume_radius = 0.0f);
}

struct GatherEmittersEventCtx
{
  FFTWater &water;
  float dt;

  void addEmitter(const gamephys::Loc &location,
    Point4 &emittance_history,
    float mass,
    float speed,
    float maxSpeed,
    float minEmitStrength,
    float maxEmitStrength,
    float emitDistance,
    float emitPeriod,
    const BBox3 &box,
    bool emitPeriodicRipples);
};

ECS_DECLARE_BOXED_TYPE(WaterRipples);
