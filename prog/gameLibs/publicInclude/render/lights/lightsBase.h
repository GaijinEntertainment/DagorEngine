//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <generic/dag_enumBitMask.h>

enum class LightType
{
  Spot,
  Omni,
  Invalid
};

enum class LightBufferSlot : uint8_t
{
  Main = 0,
  Secondary = 1
};

// Uses optimized shader variants for frames where few lights are
// visible. See dynamic_lights_count shader variable.
// "Few" means no more than the threshold, which is the size of a GPU
// word in bits.
// should match render\shaders\dynamic_lights_count.dshl
enum class DynLightsOptimizationMode : int
{
  NO_LIGHTS,
  NO_SPOTS_FEW_OMNI,
  FEW_SPOTS_NO_OMNI,
  FEW_SPOTS_FEW_OMNI,
  FULL_CLUSTERED
};

static constexpr int LIGHTS_OPTIMIZATION_THRESHOLD = 32;

static constexpr uint32_t LIGHTS_BUFFER_SLOTS = 2;
static_assert(uint32_t(LightBufferSlot::Secondary) + 1 == LIGHTS_BUFFER_SLOTS);

enum class DynLightsFeatures : uint8_t
{
  CpuOnly = 0,
  GpuLightsPartition = 1,
  GpuLightsSort = 2,
  GpuLightsManagerPersistentStorage = 4,
  GpuLightShadowsPersistentStorage = 8,
  GpuLightsCulling = (GpuLightsPartition | GpuLightsSort | GpuLightsManagerPersistentStorage | GpuLightShadowsPersistentStorage),
  GpuClusteredLightsGrid = 16,
  GpuTiledLightsZBinning = 32,
  GpuFull = (GpuLightsCulling | GpuClusteredLightsGrid | GpuTiledLightsZBinning)
};
DAGOR_ENABLE_ENUM_BITMASK(DynLightsFeatures);
class IDynLightsPipelineSection
{
public:
  virtual void init(DynLightsFeatures requested_features) = 0;
  virtual DynLightsFeatures getRuntimeFeatures() const = 0;
  bool isGpu() const;

protected:
  ~IDynLightsPipelineSection();
};
