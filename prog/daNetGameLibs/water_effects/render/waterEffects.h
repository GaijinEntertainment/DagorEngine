// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <3d/dag_resPtr.h>
#include <daECS/core/entityId.h>
#include <daECS/core/componentType.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <shaders/dag_postFxRenderer.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector_map.h>
#include <ioSys/dag_dataBlock.h>
#include <math/integer/dag_IPoint2.h>
#include <render/renderEvent.h>

struct FoamFxParams;
class FoamFx;
class ShipWakeFx;
class WaterProjectedFx;
class FFTWater;

class WaterEffects
{
public:
  struct Settings;

  WaterEffects(const Settings &in_settings);
  ~WaterEffects();

  void update(
    const TMatrix &view_tm, const TMatrix &view_itm, const TMatrix4 &proj_tm, const TMatrix4 &glob_tm, float dt, FFTWater &water);
  void render(FFTWater &water,
    const TMatrix &view_tm,
    const TMatrix4 &proj_tm,
    const Driver3dPerspective &perps,
    const ManagedTex &heightMaskTex,
    const ManagedTex &normalsTex,
    bool is_under_water);
  void renderFoam();
  void renderInternalWaterProjFx();
  void reset();

  void updateWaterProjectedFx();
  void useWaterTextures();

  void setResolution(uint32_t rtWidth, uint32_t rtHeight, float lodBias, int water_quality);
  void setFoamFxParams(const FoamFxParams &params);
  void effectsResolutionChanged();

  bool shouldUseWfxTextures() const;
  bool isUsingFoamFx() const;

  struct Settings
  {
    uint32_t rtWidth = 0;
    uint32_t rtHeight = 0;
    IPoint2 maxRenderingResolution = {0, 0};
    bool unitWakeOn = true;
    bool foamFxOn = false;
    int waterQuality = 0;
    float lodBias = 0.f;
  };

private:
  TEXTUREID loadTexture(const char *name);

  void createWaterProjectedFx();
  void clearWaterProjectedFx();
  void setProjFxResolution();
  void createFoamFx(int target_width, int target_height, int water_quality);
  void clearFoamFx();
  float getWaterProjectedFxLodBias() const;
  IPoint2 calcWaterProjectedFxResolution(IPoint2 target_res) const;

  struct UnitIndex
  {
    int index;
    float lastFront;
  };

  eastl::unique_ptr<WaterProjectedFx> waterProjectedFx;
  IPoint2 projFxResolution = IPoint2(0, 0);

  Settings settings;
  eastl::unique_ptr<FoamFx> foamFx;
  eastl::unique_ptr<ShipWakeFx> shipWakeFx;

  PostFxRenderer normalsRender;
  int effects_depth_texVarId = -1;
  int wfx_normal_pixel_sizeVarId = -1;
  int wfx_hmapVarId = -1;

  shaders::UniqueOverrideStateId flipCullStateId;

  struct ShipEffectContext
  {
    int wakeId;
  };
  eastl::vector_map<ecs::EntityId, ShipEffectContext> knownShips;

public:
  struct GatherEmittersEventCtx
  {
    FFTWater &water;
    eastl::vector_map<ecs::EntityId, ShipEffectContext> &processedShips;
    eastl::vector_map<ecs::EntityId, ShipEffectContext> &knownShips;
    ShipWakeFx &shipWakeFx;
    float minSpeed, maxSpeed, waveScale;

    inline void addEmitter(ecs::EntityId eid, const BBox3 &box, const TMatrix &tm, const Point3 &vel) const;
  };
};

dafg::NodeHandle makeWaterEffectsPrepareNode();
dafg::NodeHandle makeWaterEffectsNode(WaterEffects &water_effects);
FFTWater *get_water();

ECS_DECLARE_BOXED_TYPE(WaterEffects);
