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
void add_impulse(const Point3 &pos, float strength, float distortion, float volume_radius = 0.0f);
void add_bow_wave(const TMatrix &transform, BBox3 box, float mass, float distortion, float radius, bool reverse);
} // namespace water_ripples

struct GatherEmittersEventCtx
{
  FFTWater &water;
  float dt;

  enum class EmitterType
  {
    Spherical,
    Box
  };

  void emitPeriodicRipple(
    const Point3 &position, Point4 &emittance_history, float mass, float minEmitStrength, float emitPeriod, float radius);

  void addEmitter(EmitterType type,
    const gamephys::Loc &location,
    Point4 &emittance_history,
    float mass,
    const Point3 &velocity,
    float maxSpeed,
    float minEmitStrength,
    float maxEmitStrength,
    float emitDistance,
    float emitPeriod,
    const BBox3 &box,
    float radius,
    bool emitPeriodicRipples);

  void addSphericalEmitter(const gamephys::Loc &location,
    Point4 &emittance_history,
    float mass,
    const Point3 &velocity,
    float maxSpeed,
    float minEmitStrength,
    float maxEmitStrength,
    float emitDistance,
    float emitPeriod,
    const BBox3 &box,
    bool emitPeriodicRipples)
  {
    float radius = (box.lim[1].x - box.lim[0].x + box.lim[1].z - box.lim[0].z) * 0.25;
    addEmitter(EmitterType::Spherical, location, emittance_history, mass, velocity, maxSpeed, minEmitStrength, maxEmitStrength,
      emitDistance, emitPeriod, box, radius, emitPeriodicRipples);
  }

  void addBoxEmitter(const gamephys::Loc &location,
    Point4 &emittance_history,
    float mass,
    const Point3 &velocity,
    float maxSpeed,
    float minEmitStrength,
    float maxEmitStrength,
    float emitDistance,
    float emitPeriod,
    const BBox3 &box,
    float radius,
    bool emitPeriodicRipples)
  {
    addEmitter(EmitterType::Box, location, emittance_history, mass, velocity, maxSpeed, minEmitStrength, maxEmitStrength, emitDistance,
      emitPeriod, box, radius, emitPeriodicRipples);
  }
};

ECS_DECLARE_BOXED_TYPE(WaterRipples);
