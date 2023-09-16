#include <gamePhys/phys/floatingVolume.h>

#include <gamePhys/props/atmosphere.h>
#include <gamePhys/common/loc.h>
#include <generic/dag_smallTab.h>
#include <ioSys/dag_dataBlock.h>

using namespace gamephys;

bool floating_volumes::init(FloatingVolume &volume, const DataBlock *floatsBlk)
{
  if (!floatsBlk)
    return false;

  const int volNid = floatsBlk->getNameId("volume");
  for (int i = 0; i < floatsBlk->paramCount(); ++i)
  {
    if (floatsBlk->getParamNameId(i) != volNid || floatsBlk->getParamType(i) != DataBlock::TYPE_POINT4)
      continue;
    Point4 sphere = floatsBlk->getPoint4(i);
    volume.floatingVolumes.push_back(BSphere3(Point3::xyz(sphere), sphere.w));
  }
  volume.floatVolumesCd = floatsBlk->getReal("floatVolumesCd", 0.47f);
  return true;
}

static void apply_impulse(const DPoint3 &impulse, const DPoint3 &arm, DPoint3 &outVel, DPoint3 &outOmega, float invMass,
  const DPoint3 &invMomentOfInertia, const Point3 &gravity_center)
{
  G_ASSERT(!check_nan(impulse));
  G_ASSERT(!check_nan(arm));
  G_ASSERT(!check_nan(invMomentOfInertia));
  G_ASSERT(!check_nan(invMass));
  outVel += impulse * invMass;
  DPoint3 angularImpulseMomentum = (arm - dpoint3(gravity_center)) % impulse;
  angularImpulseMomentum.x *= invMomentOfInertia.x;
  angularImpulseMomentum.y *= invMomentOfInertia.y;
  angularImpulseMomentum.z *= invMomentOfInertia.z;
  outOmega += angularImpulseMomentum;
}

bool floating_volumes::check_sailing(const FloatingVolume &vol, const gamephys::Loc &location, const Plane3 &water_plane)
{
  for (const BSphere3 &volume : vol.floatingVolumes)
  {
    Point3 volumePos = volume.c;
    location.transform(volumePos);
    float waterDist = water_plane.distance(volumePos);
    if (waterDist <= volume.r)
      return true;
  }
  return false;
}

bool floating_volumes::update(const FloatingVolume &float_volume, float dt, const VolumeParams &params, DPoint3 &add_vel,
  DPoint3 &add_omega)
{
  if (!params.canTraceWorld || float_volume.floatingVolumes.empty())
    return false;

  G_ASSERT(float_volume.floatingVolumes.size() < MAX_VOLUMES);

  float waterDists[MAX_VOLUMES];
  for (int i = 0; i < float_volume.floatingVolumes.size(); ++i)
  {
    Point3 volumePos = float_volume.floatingVolumes[i].c;
    params.location.transform(volumePos);
    waterDists[i] = params.waterPlane.distance(volumePos);
  }

  return update(float_volume.floatingVolumes.data(), float_volume.floatingVolumes.size(), float_volume.floatVolumesCd, dt, params,
    waterDists, add_vel, add_omega);
}

bool floating_volumes::update(const BSphere3 *volumes, int volume_count, float viscosity_cf, float dt, const VolumeParams &params,
  const float *water_dists, DPoint3 &add_vel, DPoint3 &add_omega)
{
  if (!params.canTraceWorld || volume_count <= 0)
    return false;

  const double waterDensity = 1000.0;
  double totalVolume = 0.f;
  DPoint3 impVel(0, 0, 0), impOmega(0, 0, 0);
  DPoint3 viscVel(0, 0, 0), viscOmega(0, 0, 0);
  DPoint3 centralVelocity = dpoint3(params.invOrient * params.velocity);
  for (int i = 0; i < volume_count; ++i)
  {
    const BSphere3 &volume = volumes[i];
    float waterDist = water_dists[i];
    if (waterDist > volume.r)
      continue;

    double capHt = clamp(-waterDist + volume.r, 0.f, volume.r * 2.f);
    double sphereVolume = PI * sqr(capHt) * (3.f * volume.r - capHt) / 3.f;
    totalVolume += sphereVolume;
    apply_impulse(dpoint3(waterDensity * gamephys::atmosphere::g() * sphereVolume * dt * params.invOrient.getUp()), dpoint3(volume.c),
      impVel, impOmega, params.invMass, params.invMomOfInertia, params.gravityCenter);
    DPoint3 localVelDir = centralVelocity + params.omega % dpoint3(volume.c - params.gravityCenter);
    double spd = length(localVelDir);
    if (spd > 1e-2)
    {
      localVelDir /= spd;
      double refArea = PI * volume.r * capHt * 0.5f;
      // Additional vertical visc
      DPoint3 vertViscForce =
        -sign(localVelDir.y) * DPoint3(0.0, sqr(localVelDir.y), 0.0) * 0.5f * refArea * waterDensity * dt * viscosity_cf;
      DPoint3 viscosityForce = -localVelDir * sqr(spd) * 0.5f * refArea * waterDensity * dt * viscosity_cf;
      apply_impulse(viscosityForce + vertViscForce, dpoint3(volume.c), viscVel, viscOmega, params.invMass, params.invMomOfInertia,
        params.gravityCenter);
    }
  }

  // Physically visosity can't slow down speed more than by the amount of this speed, but due to
  // discrete simulation over time the values can be more than linear and angular speed.
  // And if the speed will be much enugh, it will infinetly increase in absolute value due to viscosity force applying.
  // So here we restrict viscosity to slow down speed no more than it's absolute value.
  double velAmount = saturate(dot(viscVel, viscVel) / (dot(centralVelocity, centralVelocity) + 1e-6));
  double omegaAmount = saturate(dot(viscOmega, viscOmega) / (dot(params.omega, params.omega) + 1e-6));
  add_vel += lerp(viscVel, -centralVelocity, velAmount) + impVel;
  add_omega += lerp(viscOmega, -params.omega, omegaAmount) + impOmega;

  return totalVolume * waterDensity * params.invMass > 0.5;
}

bool floating_volumes::update_visual(const FloatingVolume &float_volume, float at_time, const gamephys::Loc &visualLocation,
  float full_mass, bool can_trace_world)
{
  if (!can_trace_world || float_volume.floatingVolumes.empty())
    return false;

  const float traceRad = 2.f;
  bool underwater = false;
  Point3 pos = Point3::xyz(visualLocation.P);
  Point3 p0 = pos + Point3(1.f, 0.f, 0.f) * traceRad;
  Point3 p1 = pos + Point3(-0.5f, 0.f, -0.86f) * traceRad;
  Point3 p2 = pos + Point3(-0.5f, 0.f, 0.86f) * traceRad;
  p0.y = dacoll::traceht_water_at_time(p0, 10.f, at_time, underwater);
  p1.y = dacoll::traceht_water_at_time(p1, 10.f, at_time, underwater);
  p2.y = dacoll::traceht_water_at_time(p2, 10.f, at_time, underwater);
  if (!(dacoll::is_valid_water_height(p0.y) && dacoll::is_valid_water_height(p1.y) && dacoll::is_valid_water_height(p2.y)))
    return false;

  Plane3 waterPlane = Plane3(p0, p1, p2);

  const double waterDensity = 1000.0;
  double totalVolume = 0.f;
  ;
  for (int i = 0; i < float_volume.floatingVolumes.size(); ++i)
  {
    const BSphere3 &volume = float_volume.floatingVolumes[i];
    Point3 volumePos = volume.c;
    visualLocation.transform(volumePos);
    float waterDist = waterPlane.distance(volumePos);
    if (waterDist > volume.r)
      continue;

    double capHt = clamp(-waterDist + volume.r, 0.f, volume.r * 2.f);
    double sphereVolume = PI * sqr(capHt) * (3.f * volume.r - capHt) / 3.f;
    totalVolume += sphereVolume;
  }
  return totalVolume * waterDensity > 0.5 * full_mass;
}
