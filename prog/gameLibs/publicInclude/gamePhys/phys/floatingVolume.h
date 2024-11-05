//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_plane3.h>
#include <generic/dag_tab.h>
#include <gamePhys/common/loc.h>

class DataBlock;

namespace gamephys
{
namespace floating_volumes
{
static constexpr int MAX_VOLUMES = 16;

struct FloatingVolume
{
  Tab<BSphere3> floatingVolumes;
  float floatVolumesCd = 0.47f;
};
struct VolumeParams
{
  bool canTraceWorld;
  double invMass;
  Quat invOrient;
  DPoint3 invMomOfInertia;
  DPoint3 velocity;
  DPoint3 omega;
  Point3 gravityCenter;
  Plane3 waterPlane;
  gamephys::Loc location;
};
bool init(FloatingVolume &volume, const DataBlock *floats_blk);
bool update(const FloatingVolume &float_volume, float dt, const VolumeParams &params, DPoint3 &add_vel, DPoint3 &add_omega);
// VolumeParams::waterPlane is ignored.
bool update(const BSphere3 *volumes, int volume_count, float viscosity_cf, float dt, const VolumeParams &params,
  const float *water_dists, DPoint3 &add_vel, DPoint3 &add_omega);
bool update_visual(const FloatingVolume &volume, float at_time, const gamephys::Loc &visualLocation, float full_mass,
  bool can_trace_world);
bool check_sailing(const FloatingVolume &volume, const gamephys::Loc &location, const Plane3 &water_plane);
}; // namespace floating_volumes
}; // namespace gamephys
