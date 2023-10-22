#include <render/shipWakeFx.h>
#include <render/wakePs.h>
#include <math/dag_mathUtils.h>
#include <util/dag_convar.h>

using namespace wfx;

#define VERIFY_SHIP_INDEX G_ASSERT(index < ships.size())
#define DEFAULT_NUM_SHIPS 32
static Point3 FOAM_ALBEDO(0.75f, 0.75f, 0.75f);

CONSOLE_BOOL_VAL("render", water_foam_distorted_on, false);


ShipWakeFx::ShipWakeFx(const Settings &in_settings) : settings(in_settings)
{
  effectManager = new EffectManager();

  ParticleSystem::MainParams psWakeParams;
  psWakeParams.diffuseTexId = settings.wakeTexId;
  psWakeParams.renderType = ParticleSystem::RENDER_HEIGHT;
  psWake = effectManager->addPSystem(psWakeParams);
  psWake->reserveEmitters(DEFAULT_NUM_SHIPS * 2);

  ParticleSystem::MainParams psWakeTrailParams;
  psWakeTrailParams.diffuseTexId = settings.wakeTrailTexId;
  psWakeTrailParams.renderType = ParticleSystem::RENDER_HEIGHT;
  psWakeTrail = effectManager->addPSystem(psWakeTrailParams);
  psWakeTrail->reserveEmitters(DEFAULT_NUM_SHIPS * 3);

  ParticleSystem::MainParams psFoamParams;
  psFoamParams.diffuseTexId = settings.foamTexId;
  psFoamParams.renderType = ParticleSystem::RENDER_FOAM;
  psFoam = effectManager->addPSystem(psFoamParams);
  psFoam->reserveEmitters(DEFAULT_NUM_SHIPS * 6);

  if (!settings.reduceFoam)
  {
    ParticleSystem::MainParams psFoamHeadParams;
    psFoamHeadParams.diffuseTexId = settings.foamHeadTexId;
    psFoamHeadParams.renderType = ParticleSystem::RENDER_FOAM;
    psFoamHead = effectManager->addPSystem(psFoamHeadParams);
    psFoamHead->reserveEmitters(DEFAULT_NUM_SHIPS * 2);

    ParticleSystem::MainParams psFoamTurboParams;
    psFoamTurboParams.diffuseTexId = settings.foamTurboTexId;
    psFoamTurboParams.renderType = ParticleSystem::RENDER_FOAM;
    psFoamTurbo = effectManager->addPSystem(psFoamTurboParams);
    psFoamTurbo->reserveEmitters(DEFAULT_NUM_SHIPS * 2);
  }
  else
  {
    psFoamHead = nullptr;
    psFoamTurbo = nullptr;
  }

  ParticleSystem::MainParams psFoamTrailParams;
  psFoamTrailParams.diffuseTexId = settings.foamTrailTexId;
  psFoamTrailParams.renderType = ParticleSystem::RENDER_FOAM;
  psFoamTrail = effectManager->addPSystem(psFoamTrailParams);
  psFoamTrail->reserveEmitters(DEFAULT_NUM_SHIPS);

  if (settings.foamDistortedTileTexId != BAD_TEXTUREID && settings.foamDistortedParticleTexId != BAD_TEXTUREID &&
      settings.foamDistortedGradientTexId != BAD_TEXTUREID)
  {
    ParticleSystem::MainParams psFoamDistortedParams;
    psFoamDistortedParams.diffuseTexId = settings.foamDistortedTileTexId;
    psFoamDistortedParams.distortionTexId = settings.foamDistortedParticleTexId;
    psFoamDistortedParams.gradientTexId = settings.foamDistortedGradientTexId;
    psFoamDistortedParams.renderType = ParticleSystem::RENDER_FOAM_DISTORTED;
    psFoamDistorted = effectManager->addPSystem(psFoamDistortedParams);
    psFoamDistorted->reserveEmitters(DEFAULT_NUM_SHIPS);

    ParticleSystem::MainParams psFoamMaskParams;
    psFoamMaskParams.diffuseTexId = settings.foamDistortedTileTexId;
    psFoamMaskParams.distortionTexId = settings.foamDistortedParticleTexId;
    psFoamMaskParams.gradientTexId = settings.foamDistortedGradientTexId;
    psFoamMaskParams.renderType = ParticleSystem::RENDER_FOAM_MASK;
    psFoamMask = effectManager->addPSystem(psFoamMaskParams);
    psFoamMask->reserveEmitters(DEFAULT_NUM_SHIPS);
  }
  else
  {
    psFoamDistorted = nullptr;
    psFoamMask = nullptr;
  }
}

ShipWakeFx::~ShipWakeFx() { del_it(effectManager); }

uint32_t ShipWakeFx::addShip(const ShipDesc &desc)
{
  uint32_t index = ships.size();
  for (uint32_t shipNo = 0; shipNo < ships.size(); ++shipNo)
    if (ships[shipNo].removed)
    {
      index = shipNo;
      break;
    }

  Ship &ship = index < ships.size() ? ships[index] : ships.push_back();

  ship.removed = false;
  ship.lastLongVelSign = 1.0f;
  ship.desc = desc;
  ship.frontOffset = 0;

  float boxLen = desc.box.width().length();
  float wrp = cvt(boxLen, 20.0f, 30.0f, 15.0f, 25.0f);
  wrp = cvt(boxLen, 55.0f, 85.0f, wrp, 40.0f);
  wrp = cvt(boxLen, 130.0f, 180.0f, wrp, 60.0f);
  ship.wrp = wrp;
  ship.scaleZ = min(0.75f + wrp * 0.01f, desc.box.width().y * 0.05f);

  // psWake
  {
    const float radius = wrp * 0.08f;
    const float emitPerMeter = 1.2f / radius;
    const float posSpread = radius * 0.125f;
    const float lifeTime = radius * 1.0f;

    ship.frontOffset = radius * 0.5f;

    ParticleSystem::EmitterParams emitterParams;
    emitterParams.spawn.emitPerSecond = 0.0f;
    emitterParams.spawn.emitPerMeter = emitPerMeter;
    emitterParams.spawn.lifeTime = lifeTime;
    emitterParams.spawn.lifeSpread = 0.0f;
    emitterParams.pose.pos = Point2(0, 0);
    emitterParams.pose.posSpread = posSpread;
    emitterParams.pose.rot = 0;
    emitterParams.pose.rotSpread = PI / 6;
    emitterParams.pose.radius = radius;
    emitterParams.pose.radiusSpread = 0.0f;
    emitterParams.pose.radiusScale = 4.0f;
    emitterParams.velocity.vel = 0;
    emitterParams.velocity.velRes = 0.1f;
    emitterParams.velocity.velSpread = 0.2f;
    emitterParams.velocity.dir = Point2(0, 0);
    emitterParams.velocity.dirSpread = PI / 6;
    emitterParams.material.startColor = Point4(1, 1, 1, 1);
    emitterParams.material.endColor = Point4(1, 1, 1, 1);
    emitterParams.activeFlags = ParticleSystem::MOD_VELOCITY;
    emitterParams.pose.scale = Point3(1.0f, 1.0f, ship.scaleZ);
    ship.s[0].wakeFront = psWake->addEmitter(emitterParams);
    emitterParams.pose.scale = Point3(-1.0f, 1.0f, ship.scaleZ);
    ship.s[1].wakeFront = psWake->addEmitter(emitterParams);
  }

  // psWakeTrail
  {
    const float radius1 = wrp * 0.14f;
    const float emitPerMeter1 = 0.7f / radius1;
    const float posSpread1 = radius1 * 0.143f;
    const float lifeTime1 = radius1 * 4.0f;

    ParticleSystem::EmitterParams emitterParams;
    emitterParams.spawn.emitPerSecond = 0.0f;
    emitterParams.spawn.emitPerMeter = emitPerMeter1;
    emitterParams.spawn.lifeTime = lifeTime1;
    emitterParams.spawn.lifeSpread = 0.0f;
    emitterParams.pose.pos = Point2(0, 0);
    emitterParams.pose.posSpread = posSpread1;
    emitterParams.pose.rot = 0;
    emitterParams.pose.rotSpread = PI;
    emitterParams.pose.radius = radius1;
    emitterParams.pose.radiusSpread = 0.0f;
    emitterParams.pose.radiusScale = 6.0f;
    emitterParams.velocity.vel = 0;
    emitterParams.velocity.velSpread = 0;
    emitterParams.velocity.dir = Point2(0, 0);
    emitterParams.velocity.dirSpread = 0;
    emitterParams.material.startColor = Point4(1, 1, 1, 1);
    emitterParams.material.endColor = Point4(2, 2, 2, 1);
    emitterParams.pose.scale = Point3(1.0f, 1.0f, ship.scaleZ);
    ship.wakeTrail = psWakeTrail->addEmitter(emitterParams);

    const float radius2 = wrp * 0.04f;
    const float emitPerMeter2 = 0.4f / radius2;
    const float posSpread2 = radius2 * 0.25f;
    const float lifeTime2 = radius2 * 7.3f;

    emitterParams.spawn.emitPerMeter = emitPerMeter2;
    emitterParams.spawn.lifeTime = lifeTime2;
    emitterParams.spawn.lifeSpread = 0.0f;
    emitterParams.pose.pos = Point2(0, 0);
    emitterParams.pose.posSpread = posSpread2;
    emitterParams.pose.rot = 0;
    emitterParams.pose.rotSpread = PI;
    emitterParams.pose.radius = radius2;
    emitterParams.pose.radiusSpread = 0.0f;
    emitterParams.pose.radiusScale = 8.0f;
    emitterParams.velocity.vel = 0.0f;
    emitterParams.velocity.velRes = 0.1f;
    emitterParams.velocity.velSpread = 0.2f;
    emitterParams.velocity.dir = Point2(0, -1);
    emitterParams.velocity.dirSpread = PI / 12;
    emitterParams.material.startColor = Point4(1, 1, 1, 1);
    emitterParams.material.endColor = Point4(2, 2, 2, 1);
    emitterParams.activeFlags = ParticleSystem::MOD_VELOCITY;
    emitterParams.pose.scale = Point3(1.0f, 1.0f, 1.0f);
    ship.s[0].wakeBack = psWakeTrail->addEmitter(emitterParams);

    emitterParams.pose.scale = Point3(-1.0f, 1.0f, 1.0f);
    ship.s[1].wakeBack = psWakeTrail->addEmitter(emitterParams);
  }

  // psFoam
  {
    const float radius1 = wrp * 0.06f;
    const float emitPerMeter1 = 0.75f / radius1;
    const float posSpread1 = radius1 * 0.167f;
    const float lifeTime1 = radius1 * 1.0f;

    ParticleSystem::EmitterParams emitterParams;
    emitterParams.spawn.emitPerSecond = 0.0f;
    emitterParams.spawn.emitPerMeter = emitPerMeter1;
    emitterParams.spawn.lifeTime = lifeTime1;
    emitterParams.spawn.lifeSpread = 0.0f;
    emitterParams.pose.pos = Point2(0, 0);
    emitterParams.pose.posSpread = posSpread1;
    emitterParams.pose.rot = 0;
    emitterParams.pose.rotSpread = PI / 6;
    emitterParams.pose.radius = radius1;
    emitterParams.pose.radiusSpread = 0.0f;
    emitterParams.pose.radiusScale = 2.0f;
    emitterParams.velocity.vel = 0.0f;
    emitterParams.velocity.velRes = 0.1f;
    emitterParams.velocity.velSpread = 0.1f;
    emitterParams.velocity.dir = Point2(-1, 0);
    emitterParams.velocity.dirSpread = PI / 12;
    emitterParams.material.uvNumFrames = IPoint2(8, 8);
    emitterParams.material.startColor = Point4::xyzV(FOAM_ALBEDO, 1);
    emitterParams.material.endColor = Point4::xyzV(FOAM_ALBEDO, 0);
    emitterParams.activeFlags = ParticleSystem::MOD_VELOCITY;
    emitterParams.pose.scale = Point3(1.0f, 1.0f, 1.0f);
    ship.s[0].foamFront = psFoam->addEmitter(emitterParams);
    emitterParams.pose.scale = Point3(-1.0f, 1.0f, 1.0f);
    ship.s[1].foamFront = psFoam->addEmitter(emitterParams);

    const float emitPerMeter2 = 25.0f / wrp;
    const float lifeTime2 = wrp * 0.12f;
    const float lifeDelay2 = lifeTime1 * 0.25f;

    emitterParams.spawn.emitPerMeter = emitPerMeter2;
    emitterParams.spawn.lifeTime = lifeTime2;
    emitterParams.spawn.lifeDelay = lifeDelay2;
    emitterParams.material.startColor = Point4::xyzV(FOAM_ALBEDO, 0.75f);
    emitterParams.material.endColor = Point4::xyzV(FOAM_ALBEDO, 0);
    emitterParams.pose.scale = Point3(1.0f, 1.0f, 1.0f);
    ship.s[0].foamBack = psFoam->addEmitter(emitterParams);
    emitterParams.pose.scale = Point3(-1.0f, 1.0f, 1.0f);
    ship.s[1].foamBack = psFoam->addEmitter(emitterParams);

    const float radius3 = wrp * 0.14f;
    const float radiusSpread3 = radius3 * 0.143f;
    const float emitPerMeter3 = 1.4f / radius3;
    const float posSpread3 = radius3 * 0.143f;
    const float lifeTime3 = radius3 * 2.0f;

    emitterParams.spawn.emitPerSecond = 0.0f;
    emitterParams.spawn.emitPerMeter = emitPerMeter3;
    emitterParams.spawn.lifeTime = lifeTime3;
    emitterParams.spawn.lifeSpread = 0.0f;
    emitterParams.pose.pos = Point2(0, 0);
    emitterParams.pose.posSpread = posSpread3;
    emitterParams.pose.rot = 0;
    emitterParams.pose.rotSpread = PI / 2;
    emitterParams.pose.radius = radius3;
    emitterParams.pose.radiusSpread = radiusSpread3;
    emitterParams.pose.radiusScale = 1.5f;
    emitterParams.velocity.vel = 0.0f;
    emitterParams.velocity.velRes = 0.1f;
    emitterParams.velocity.velSpread = 1.0f;
    emitterParams.velocity.dir = Point2(-1, 0);
    emitterParams.velocity.dirSpread = PI / 12;
    emitterParams.material.uvNumFrames = IPoint2(8, 8);
    emitterParams.material.startColor = Point4::xyzV(FOAM_ALBEDO, 1);
    emitterParams.material.endColor = Point4::xyzV(FOAM_ALBEDO, 0);
    emitterParams.activeFlags = ParticleSystem::MOD_VELOCITY;
    emitterParams.pose.scale = Point3(1.0f, 1.0f, 1.0f);
    ship.s[0].foamTrailWave = psFoam->addEmitter(emitterParams);
    emitterParams.pose.scale = Point3(-1.0f, 1.0f, 1.0f);
    ship.s[1].foamTrailWave = psFoam->addEmitter(emitterParams);
  }

  if (!settings.reduceFoam)
  {
    // psFoamHead
    {
      const float radius = wrp * 0.06f;
      const float emitPerMeter = 0.525f / radius;
      const float posSpread = radius * 0.167f;
      const float lifeTime = radius * 1.0f;

      ParticleSystem::EmitterParams emitterParams;
      emitterParams.spawn.emitPerSecond = 0.0f;
      emitterParams.spawn.emitPerMeter = emitPerMeter;
      emitterParams.spawn.lifeTime = lifeTime;
      emitterParams.spawn.lifeSpread = 0;
      emitterParams.pose.pos = Point2(0, 0);
      emitterParams.pose.posSpread = posSpread;
      emitterParams.pose.rot = 0;
      emitterParams.pose.rotSpread = PI / 24;
      emitterParams.pose.radius = radius;
      emitterParams.pose.radiusSpread = 0;
      emitterParams.pose.radiusScale = 3.5f;
      emitterParams.velocity.vel = 0.0f;
      emitterParams.velocity.velRes = 0.1f;
      emitterParams.velocity.velSpread = 0.1f;
      emitterParams.velocity.dir = Point2(-1, 0);
      emitterParams.velocity.dirSpread = PI / 24;
      emitterParams.material.uvNumFrames = IPoint2(8, 8);
      emitterParams.material.startColor = Point4::xyzV(FOAM_ALBEDO, 1);
      emitterParams.material.endColor = Point4::xyzV(FOAM_ALBEDO, 0);
      emitterParams.activeFlags = ParticleSystem::MOD_VELOCITY;
      emitterParams.pose.scale = Point3(1.0f, 1.0f, 1.0f);
      ship.s[0].foamHead = psFoamHead->addEmitter(emitterParams);
      emitterParams.pose.scale = Point3(-1.0f, 1.0f, 1.0f);
      ship.s[1].foamHead = psFoamHead->addEmitter(emitterParams);
    }

    // psFoamTurbo
    {
      const float radius = wrp * 0.07f;
      const float emitPerMeter = 0.525f / radius;
      const float posSpread = radius * 0.143f;
      const float lifeTime = radius * 0.857f;

      ParticleSystem::EmitterParams emitterParams;
      emitterParams.spawn.emitPerSecond = 0.0f;
      emitterParams.spawn.emitPerMeter = emitPerMeter;
      emitterParams.spawn.lifeTime = lifeTime;
      emitterParams.spawn.lifeSpread = 0.0f;
      emitterParams.pose.pos = Point2(0, 0);
      emitterParams.pose.posSpread = posSpread;
      emitterParams.pose.rot = 0;
      emitterParams.pose.rotSpread = PI / 24;
      emitterParams.pose.radius = radius;
      emitterParams.pose.radiusSpread = 0.0f;
      emitterParams.pose.radiusScale = 3.0f;
      emitterParams.velocity.vel = 0.0f;
      emitterParams.velocity.velRes = 0.1f;
      emitterParams.velocity.velSpread = 0.1f;
      emitterParams.velocity.dir = Point2(-1, 0);
      emitterParams.velocity.dirSpread = PI / 24;
      emitterParams.material.uvNumFrames = IPoint2(8, 8);
      emitterParams.material.startColor = Point4::xyzV(FOAM_ALBEDO, 1);
      emitterParams.material.endColor = Point4::xyzV(FOAM_ALBEDO, 0);
      emitterParams.activeFlags = ParticleSystem::MOD_VELOCITY;
      emitterParams.pose.scale = Point3(1.0f, 1.0f, 1.0f);
      ship.s[0].foamTurbo = psFoamTurbo->addEmitter(emitterParams);
      emitterParams.pose.scale = Point3(-1.0f, 1.0f, 1.0f);
      ship.s[1].foamTurbo = psFoamTurbo->addEmitter(emitterParams);
    }
  }

  // psFoamTrail
  {
    const float radius = wrp * 0.08f;
    const float radiusSpread = radius * 0.25f;
    const float emitPerMeter = 0.8f / radius;
    const float posSpread = radius * 0.25f;
    const float lifeTime = radius * 3.5f;

    ParticleSystem::EmitterParams emitterParams;
    emitterParams.spawn.emitPerSecond = 0.0f;
    emitterParams.spawn.emitPerMeter = emitPerMeter;
    emitterParams.spawn.lifeTime = lifeTime;
    emitterParams.spawn.lifeSpread = 0.0f;
    emitterParams.pose.pos = Point2(0, 0);
    emitterParams.pose.posSpread = posSpread;
    emitterParams.pose.rot = 0;
    emitterParams.pose.rotSpread = PI / 6;
    emitterParams.pose.radius = radius;
    emitterParams.pose.radiusSpread = radiusSpread;
    emitterParams.pose.radiusScale = 2.0f;
    emitterParams.velocity.vel = 0.0f;
    emitterParams.velocity.velSpread = 0.0f;
    emitterParams.velocity.dir = Point2(-1, 0);
    emitterParams.velocity.dirSpread = 0.0f;
    emitterParams.material.uvNumFrames = IPoint2(8, 8);
    emitterParams.material.startColor = Point4::xyzV(FOAM_ALBEDO, 1);
    emitterParams.material.endColor = Point4::xyzV(FOAM_ALBEDO, 0);
    ship.foamTrail = psFoamTrail->addEmitter(emitterParams);
  }

  if (psFoamDistorted)
  {
    const float radius = wrp * 0.05f;
    const float radiusSpread = radius * 0.25f;
    const float emitPerMeter = 1.5f / radius;
    const float posSpread = radius * 0.25f;
    const float lifeTime = radius * 3.5f;

    ParticleSystem::EmitterParams emitterParams;
    emitterParams.spawn.emitPerSecond = 0.0f;
    emitterParams.spawn.emitPerMeter = emitPerMeter;
    emitterParams.spawn.lifeTime = lifeTime;
    emitterParams.spawn.lifeSpread = 0.0f;
    emitterParams.pose.pos = Point2(0, 0);
    emitterParams.pose.posSpread = posSpread;
    emitterParams.pose.rot = 0;
    emitterParams.pose.rotSpread = PI / 6;
    emitterParams.pose.radius = radius;
    emitterParams.pose.radiusSpread = radiusSpread;
    emitterParams.pose.radiusScale = 5.0f;
    emitterParams.velocity.vel = 0.0f;
    emitterParams.velocity.velSpread = 0.0f;
    emitterParams.velocity.dir = Point2(-1, 0);
    emitterParams.velocity.dirSpread = 0.0f;
    emitterParams.material.uvNumFrames = IPoint2(1, 1);
    emitterParams.material.startColor = Point4::xyzV(FOAM_ALBEDO, 1);
    emitterParams.material.endColor = Point4::xyzV(FOAM_ALBEDO, 0);
    ship.foamDistorted = psFoamDistorted->addEmitter(emitterParams);
  }

  if (psFoamMask)
  {
    const float radius = wrp * 0.08f;
    const float radiusSpread = radius * 0.25f;
    const float emitPerMeter = 1.5f / radius;
    const float posSpread = radius * 0.25f;
    const float lifeTime = radius * 3.5f;

    ParticleSystem::EmitterParams emitterParams;
    emitterParams.spawn.emitPerSecond = 0.0f;
    emitterParams.spawn.emitPerMeter = emitPerMeter;
    emitterParams.spawn.lifeTime = lifeTime;
    emitterParams.spawn.lifeSpread = 0.0f;
    emitterParams.pose.pos = Point2(0, 0);
    emitterParams.pose.posSpread = posSpread;
    emitterParams.pose.rot = 0;
    emitterParams.pose.rotSpread = PI / 6;
    emitterParams.pose.radius = radius;
    emitterParams.pose.radiusSpread = radiusSpread;
    emitterParams.pose.radiusScale = 5.0f;
    emitterParams.velocity.vel = 0.0f;
    emitterParams.velocity.velSpread = 0.0f;
    emitterParams.velocity.dir = Point2(-1, 0);
    emitterParams.velocity.dirSpread = 0.0f;
    emitterParams.material.uvNumFrames = IPoint2(1, 1);
    emitterParams.material.startColor = Point4::xyzV(Point3(1.0f, 1.0f, 1.0f), 1);
    emitterParams.material.endColor = Point4::xyzV(Point3(1.0f, 1.0f, 1.0f), 0);
    ship.foamMask = psFoamMask->addEmitter(emitterParams);
  }

  return index;
}

void ShipWakeFx::removeShip(uint32_t index)
{
  VERIFY_SHIP_INDEX;
  const Ship &ship = ships[index];
  G_ASSERT(!ship.removed);
  if (ship.removed)
    return;

  ships[index].removed = true;
  for (int sNo = 0; sNo < 2; ++sNo)
  {
    psWake->removeEmitter(ship.s[sNo].wakeFront);
    psWakeTrail->removeEmitter(ship.s[sNo].wakeBack);
    psFoam->removeEmitter(ship.s[sNo].foamFront);
    psFoam->removeEmitter(ship.s[sNo].foamBack);
    psFoam->removeEmitter(ship.s[sNo].foamTrailWave);
    if (!settings.reduceFoam)
    {
      psFoamHead->removeEmitter(ship.s[sNo].foamHead);
      psFoamTurbo->removeEmitter(ship.s[sNo].foamTurbo);
    }
  }

  psWakeTrail->removeEmitter(ship.wakeTrail);
  psFoamTrail->removeEmitter(ship.foamTrail);

  if (psFoamDistorted)
    psFoamDistorted->removeEmitter(ship.foamDistorted);
  if (psFoamMask)
    psFoamMask->removeEmitter(ship.foamMask);
}

void ShipWakeFx::clearShips()
{
  psWake->clearEmitters();
  psWakeTrail->clearEmitters();
  psFoam->clearEmitters();
  psFoamTrail->clearEmitters();
  if (!settings.reduceFoam)
  {
    psFoamHead->clearEmitters();
    psFoamTurbo->clearEmitters();
  }

  if (psFoamDistorted)
    psFoamDistorted->clearEmitters();
  if (psFoamMask)
    psFoamMask->clearEmitters();

  clear_and_shrink(ships);
}

bool ShipWakeFx::hasWork() const
{
  return (psWake && psWake->isActive()) || (psWakeTrail && psWakeTrail->isActive()) || (psFoam && psFoam->isActive()) ||
         (psFoamHead && psFoamHead->isActive()) || (psFoamTurbo && psFoamTurbo->isActive()) ||
         (psFoamTrail && psFoamTrail->isActive()) || (psFoamDistorted && psFoamDistorted->isActive()) ||
         (psFoamMask && psFoamMask->isActive());
}

int ShipWakeFx::getShipsCount() const { return ships.size(); }

bool ShipWakeFx::isShipRemoved(uint32_t index) const
{
  VERIFY_SHIP_INDEX;
  return ships[index].removed;
}

void ShipWakeFx::setShipState(uint32_t index, const ShipState &state)
{
  const float WAKE_LATERAL = 0.45f;
  const float FOAM_LATERAL = 0.60f;
  const float WAKE_TRAIL_LATERAL = 0.15f;
  const float FOAM_TRAIL_LATERIAL = WAKE_TRAIL_LATERAL;
  const float WAKE_TRAIL_SCALEZ_MIN_VEL = 3.0f;

  VERIFY_SHIP_INDEX;
  const Ship &ship = ships[index];
  G_ASSERT(!ship.removed);
  if (ship.removed)
    return;

  const float SPAWN_OFFSET = ship.wrp * 0.06f;
  const float WAKE_OFFSET = -ship.wrp * 0.06f;
  const float WAKE_TRAIL_OFFSET = ship.wrp * 0.04f;
  const float FOAM_TRAIL_OFFSET = WAKE_TRAIL_OFFSET;

  const float longVelSign = sign(dot(Point2::xz(state.velocity), Point2::xz(state.tm.getcol(0))));
  if (ship.lastLongVelSign != longVelSign)
    resetShipSpawnCounters(index);
  ships[index].lastLongVelSign = longVelSign;
  Point3 dir3, up, right3;
  ortho_normalize(state.tm.getcol(0) * longVelSign, Point3(0, 1, 0), dir3, up, right3);
  const Point2 shipPos = Point2::xz(state.tm.getcol(3));
  const Point2 shipDir = normalize(Point2::xz(dir3));
  const Point2 shipRight = normalize(Point2::xz(right3));
  const Point2 velocity = Point2::xz(state.velocity);
  const float longVel = fabsf(dot(velocity, shipDir));
  const float latVel = fabsf(dot(velocity, shipRight));
  const float shipRot = -atan2f(shipDir.x, shipDir.y);
  const Point2 shipSz = Point2::xz(ship.desc.box.width());
  const Point2 shipFront = Point2(longVelSign > 0 ? state.frontPos + ship.frontOffset : -ship.desc.box.lim[0].x,
    (ship.desc.box.lim[0].z + ship.desc.box.lim[1].z) / 2 + state.wakeHeadShift);
  const Point2 shipBack =
    Point2(longVelSign > 0 ? ship.desc.box.lim[0].x : -ship.desc.box.lim[1].x, (ship.desc.box.lim[0].z + ship.desc.box.lim[1].z) / 2);

  const float WAKE_TRAIL_POS = shipSz.x * 0.5f;
  const float WAKE_SEC_TRAIL_POS = shipSz.x * 0.75f;

  const Point2 right[2] = {shipRight, -shipRight};
  const float wakeAngleVel = atanf(WAKE_LATERAL);
  const float wakeAngle[2] = {-wakeAngleVel - PI / 4 + shipRot, wakeAngleVel + PI / 4 + shipRot};
  const float foamAngle[2] = {-wakeAngleVel - PI / 2 + shipRot, wakeAngleVel + PI / 2 + shipRot};
  const float foamAngleHeadVel = atanf(FOAM_LATERAL);
  const float foamAngleHead[2] = {-foamAngleHeadVel - PI / 2 + shipRot, foamAngleHeadVel + PI / 2 + shipRot};

  float foamAlphaValue = 1;
  if (state.foamVelocityThresholds.has_value())
  {
    float range = state.foamVelocityThresholds.value().second - state.foamVelocityThresholds.value().first;
    if (range > 0)
      foamAlphaValue = saturate((state.velocity.length() - state.foamVelocityThresholds.value().first) / range);
  }

  for (int sNo = 0; sNo < 2; ++sNo)
  {
    const float latVelSign = sign(dot(velocity, right[sNo]));
    const Point2 wakePos =
      shipPos + (shipDir * shipFront.x + shipRight * shipFront.y) + right[sNo] * (SPAWN_OFFSET + state.wakeHeadOffset);

    const Point2 flowVel = right[sNo] * (longVel > 0.1f ? max(longVel * WAKE_LATERAL, 1.0f) : 0.0f) +
                           right[sNo] * (latVel > 0.1f ? max(latVel, 0.5f) * latVelSign : 0.0f);
    const Point2 flowDir = normalizeDef(flowVel, shipDir);
    const float flowVelLen = flowVel.length();

    // psWake
    if (state.hasFront)
    {
      ParticleSystem::Pose emitterPose = psWake->getEmitterPose(ship.s[sNo].wakeFront);
      emitterPose.pos = wakePos + right[sNo] * WAKE_OFFSET;
      emitterPose.rot = wakeAngle[sNo];
      psWake->setEmitterPose(ship.s[sNo].wakeFront, emitterPose);

      ParticleSystem::Velocity emitterVelocity = psWake->getEmitterVelocity(ship.s[sNo].wakeFront);
      emitterVelocity.dir = flowDir;
      emitterVelocity.vel = flowVelLen;
      psWake->setEmitterVelocity(ship.s[sNo].wakeFront, emitterVelocity);
    }

    // psFoam
    {
      wfx::ParticleSystem::Pose emitterPose;
      ParticleSystem::Velocity emitterVelocity;

      if (state.hasFront)
      {
        emitterPose = psFoam->getEmitterPose(ship.s[sNo].foamFront);
        emitterPose.pos = wakePos;
        emitterPose.rot = foamAngle[sNo];
        psFoam->setEmitterPose(ship.s[sNo].foamFront, emitterPose);
        emitterVelocity = psFoam->getEmitterVelocity(ship.s[sNo].foamFront);
        emitterVelocity.dir = flowDir;
        emitterVelocity.vel = flowVelLen;
        psFoam->setEmitterVelocity(ship.s[sNo].foamFront, emitterVelocity);
        psFoam->setEmitterAlpha(ship.s[sNo].foamFront, foamAlphaValue);

        emitterPose = psFoam->getEmitterPose(ship.s[sNo].foamBack);
        emitterPose.pos = wakePos;
        emitterPose.rot = foamAngle[sNo];
        psFoam->setEmitterPose(ship.s[sNo].foamBack, emitterPose);
        emitterVelocity = psFoam->getEmitterVelocity(ship.s[sNo].foamBack);
        emitterVelocity.dir = flowDir;
        emitterVelocity.vel = flowVelLen;
        psFoam->setEmitterVelocity(ship.s[sNo].foamBack, emitterVelocity);
        psFoam->setEmitterAlpha(ship.s[sNo].foamBack, foamAlphaValue);
      }


      emitterPose = psFoam->getEmitterPose(ship.s[sNo].foamTrailWave);
      emitterPose.pos = shipPos + shipDir * (shipBack.x + WAKE_TRAIL_POS) + right[sNo] * (shipBack.y + FOAM_TRAIL_OFFSET);
      emitterPose.rot = foamAngle[sNo];
      psFoam->setEmitterPose(ship.s[sNo].foamTrailWave, emitterPose);
      const Point2 foamVel = right[sNo] * fabsf(longVel) * FOAM_TRAIL_LATERIAL;
      emitterVelocity = psFoam->getEmitterVelocity(ship.s[sNo].foamTrailWave);
      emitterVelocity.dir = normalizeDef(foamVel, shipDir);
      emitterVelocity.vel = foamVel.length();
      psFoam->setEmitterVelocity(ship.s[sNo].foamTrailWave, emitterVelocity);
      psFoam->setEmitterAlpha(ship.s[sNo].foamTrailWave, foamAlphaValue);
    }

    if (!settings.reduceFoam && state.hasFront)
    {
      const Point2 foamHeadVel = right[sNo] * (longVel > 0.1f ? max(longVel * FOAM_LATERAL, 1.0f) : 0.0f) +
                                 right[sNo] * (latVel > 0.1f ? max(latVel, 0.5f) * latVelSign : 0.0f);
      const Point2 foamHeadVelDir = normalizeDef(foamHeadVel, shipDir);
      const float foamVelHeadVelLen = foamHeadVel.length();

      // psFoamHead
      {
        wfx::ParticleSystem::Pose emitterPose = psFoamHead->getEmitterPose(ship.s[sNo].foamHead);
        emitterPose.pos = wakePos;
        emitterPose.rot = foamAngleHead[sNo];
        psFoamHead->setEmitterPose(ship.s[sNo].foamHead, emitterPose);

        ParticleSystem::Velocity emitterVelocity = psFoam->getEmitterVelocity(ship.s[sNo].foamHead);
        emitterVelocity.dir = foamHeadVelDir;
        emitterVelocity.vel = foamVelHeadVelLen;
        psFoamHead->setEmitterVelocity(ship.s[sNo].foamHead, emitterVelocity);
        psFoamHead->setEmitterAlpha(ship.s[sNo].foamHead, foamAlphaValue);
      }

      // psFoamTurbo
      {
        wfx::ParticleSystem::Pose emitterPose = psFoamTurbo->getEmitterPose(ship.s[sNo].foamTurbo);
        emitterPose.pos = wakePos;
        emitterPose.rot = foamAngleHead[sNo];
        psFoamTurbo->setEmitterPose(ship.s[sNo].foamTurbo, emitterPose);

        ParticleSystem::Velocity emitterVelocity = psFoam->getEmitterVelocity(ship.s[sNo].foamTurbo);
        emitterVelocity.dir = foamHeadVelDir;
        emitterVelocity.vel = foamVelHeadVelLen;
        psFoamTurbo->setEmitterVelocity(ship.s[sNo].foamTurbo, emitterVelocity);
        psFoamTurbo->setEmitterAlpha(ship.s[sNo].foamTurbo, foamAlphaValue);
      }
    }
  }

  // psWakeTrail
  {
    const float speedScaleZ = saturate((longVel - WAKE_TRAIL_SCALEZ_MIN_VEL) / WAKE_TRAIL_SCALEZ_MIN_VEL);
    ParticleSystem::Pose emitterPose = psWakeTrail->getEmitterPose(ship.wakeTrail);
    emitterPose.pos = shipPos + shipDir * (shipBack.x + WAKE_TRAIL_POS) + shipRight * shipBack.y;
    emitterPose.scale.z = ship.scaleZ * speedScaleZ;
    psWakeTrail->setEmitterPose(ship.wakeTrail, emitterPose);

    for (int sNo = 0; sNo < 2; ++sNo)
    {
      emitterPose = psWakeTrail->getEmitterPose(ship.s[sNo].wakeBack);
      emitterPose.pos = shipPos + shipDir * (shipBack.x + WAKE_SEC_TRAIL_POS) + right[sNo] * (shipBack.y + WAKE_TRAIL_OFFSET);
      emitterPose.scale.z = ship.scaleZ * speedScaleZ;
      psWakeTrail->setEmitterPose(ship.s[sNo].wakeBack, emitterPose);

      const Point2 trailVel = right[sNo] * fabsf(longVel) * WAKE_TRAIL_LATERAL;
      ParticleSystem::Velocity emitterVelocity = psWakeTrail->getEmitterVelocity(ship.s[sNo].wakeBack);
      emitterVelocity.dir = normalizeDef(trailVel, shipDir);
      emitterVelocity.vel = trailVel.length();
      psWakeTrail->setEmitterVelocity(ship.s[sNo].wakeBack, emitterVelocity);
    }
  }

  // psFoamTrail
  {
    ParticleSystem::Pose emitterPose = psFoamTrail->getEmitterPose(ship.foamTrail);
    emitterPose.pos = shipPos + shipDir * (shipBack.x + WAKE_TRAIL_POS) + shipRight * shipBack.y;
    emitterPose.rot = PI + shipRot;
    psFoamTrail->setEmitterPose(ship.foamTrail, emitterPose);
    psFoamTrail->setEmitterAlpha(ship.foamTrail, foamAlphaValue);
  }

  if (psFoamDistorted)
  {
    ParticleSystem::Pose emitterPose = psFoamDistorted->getEmitterPose(ship.foamDistorted);
    emitterPose.pos = shipPos + shipDir * (shipBack.x + WAKE_TRAIL_POS) + shipRight * shipBack.y;
    emitterPose.rot = PI + shipRot;
    psFoamDistorted->setEmitterPose(ship.foamDistorted, emitterPose);
    psFoamDistorted->setEmitterAlpha(ship.foamDistorted, foamAlphaValue);
  }

  if (psFoamMask)
  {
    ParticleSystem::Pose emitterPose = psFoamMask->getEmitterPose(ship.foamMask);
    emitterPose.pos = shipPos + shipDir * (shipBack.x + WAKE_TRAIL_POS) + shipRight * shipBack.y;
    emitterPose.rot = PI + shipRot;
    psFoamMask->setEmitterPose(ship.foamMask, emitterPose);
    psFoamMask->setEmitterAlpha(ship.foamMask, foamAlphaValue);
  }
}

void ShipWakeFx::resetShipSpawnCounters(uint32_t index, bool jump)
{
  VERIFY_SHIP_INDEX;
  const Ship &ship = ships[index];

  for (int sNo = 0; sNo < 2; ++sNo)
  {
    psWake->resetEmitterSpawnCounters(ship.s[sNo].wakeFront, jump);
    psWakeTrail->resetEmitterSpawnCounters(ship.s[sNo].wakeBack, jump);
    psFoam->resetEmitterSpawnCounters(ship.s[sNo].foamFront, jump);
    psFoam->resetEmitterSpawnCounters(ship.s[sNo].foamBack, jump);
    psFoam->resetEmitterSpawnCounters(ship.s[sNo].foamTrailWave, jump);
    if (!settings.reduceFoam)
    {
      psFoamHead->resetEmitterSpawnCounters(ship.s[sNo].foamHead, jump);
      psFoamTurbo->resetEmitterSpawnCounters(ship.s[sNo].foamTurbo, jump);
    }
  }
  psWakeTrail->resetEmitterSpawnCounters(ship.wakeTrail, jump);
  psFoamTrail->resetEmitterSpawnCounters(ship.foamTrail, jump);

  if (psFoamDistorted)
    psFoamDistorted->resetEmitterSpawnCounters(ship.foamDistorted, jump);
  if (psFoamMask)
    psFoamMask->resetEmitterSpawnCounters(ship.foamMask, jump);
}

void ShipWakeFx::update(float dt) { effectManager->update(dt); }

void ShipWakeFx::render()
{
  effectManager->render(ParticleSystem::RENDER_HEIGHT);

  if (water_foam_distorted_on.get())
  {
    if (psFoamDistorted)
      psFoamDistorted->setRenderType(ParticleSystem::RENDER_HEIGHT_DISTORTED);

    effectManager->render(ParticleSystem::RENDER_HEIGHT_DISTORTED);
  }
}

bool ShipWakeFx::renderFoam()
{
  bool renderedAnything = false;
  renderedAnything |= effectManager->render(ParticleSystem::RENDER_FOAM);

  if (water_foam_distorted_on.get())
  {
    if (psFoamDistorted)
      psFoamDistorted->setRenderType(ParticleSystem::RENDER_FOAM_DISTORTED);

    renderedAnything |= effectManager->render(ParticleSystem::RENDER_FOAM_DISTORTED);
  }
  return renderedAnything;
}

void ShipWakeFx::renderFoamMask() { effectManager->render(ParticleSystem::RENDER_FOAM_MASK); }

void ShipWakeFx::reset() { effectManager->reset(); }