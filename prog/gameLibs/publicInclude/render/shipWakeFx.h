//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_bounds3.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>
#include <3d/dag_texMgr.h>

namespace wfx
{
class EffectManager;
class ParticleSystem;
} // namespace wfx

class ShipWakeFx
{
public:
  struct Settings;
  struct ShipDesc;
  struct ShipState;

  ShipWakeFx(const Settings &in_settings);
  ~ShipWakeFx();

  bool hasWork() const;

  uint32_t addShip(const ShipDesc &desc);
  void removeShip(uint32_t index);
  void clearShips();
  int getShipsCount() const;
  bool isShipRemoved(uint32_t index) const;

  void setShipState(uint32_t index, const ShipState &state);
  void resetShipSpawnCounters(uint32_t index, bool jump = false);

  void update(float dt);
  void render();
  void renderFoam();
  void renderFoamMask();
  void reset();

  const Settings &getSettings() { return settings; }

  struct Settings
  {
    TEXTUREID wakeTexId = BAD_TEXTUREID;
    TEXTUREID wakeTrailTexId = BAD_TEXTUREID;
    TEXTUREID foamTexId = BAD_TEXTUREID;
    TEXTUREID foamHeadTexId = BAD_TEXTUREID;
    TEXTUREID foamTurboTexId = BAD_TEXTUREID;
    TEXTUREID foamTrailTexId = BAD_TEXTUREID;
    TEXTUREID foamDistortedTileTexId = BAD_TEXTUREID;
    TEXTUREID foamDistortedParticleTexId = BAD_TEXTUREID;
    TEXTUREID foamDistortedGradientTexId = BAD_TEXTUREID;
    bool reduceFoam = false;
  };

  struct ShipDesc
  {
    BBox3 box;
  };

  struct ShipState
  {
    TMatrix tm;
    Point3 velocity;
    float frontPos = 0.f;
    float wakeHeadOffset = 0.f;
    float wakeHeadShift = 0.f;
    bool hasFront = true;
    eastl::optional<eastl::pair<float, float>> foamVelocityThresholds;
  };

private:
  struct Ship
  {
    ShipDesc desc;
    // wave ressistance power (25)
    float wrp;

    uint32_t wakeTrail;
    uint32_t foamTrail;
    uint32_t foamDistorted;
    uint32_t foamMask;
    struct
    {
      uint32_t wakeFront;
      uint32_t wakeBack;
      uint32_t foamFront;
      uint32_t foamBack;
      uint32_t foamTrailWave;
      uint32_t foamHead;
      uint32_t foamTurbo;
    } s[2];

    bool removed;
    float lastLongVelSign;
    float frontOffset;
    float scaleZ;
  };

  Settings settings;
  Tab<Ship> ships;

  wfx::EffectManager *effectManager;
  wfx::ParticleSystem *psWake;
  wfx::ParticleSystem *psWakeTrail;
  wfx::ParticleSystem *psFoam;
  wfx::ParticleSystem *psFoamHead;
  wfx::ParticleSystem *psFoamTurbo;
  wfx::ParticleSystem *psFoamTrail;
  wfx::ParticleSystem *psFoamDistorted;
  wfx::ParticleSystem *psFoamMask;
};
