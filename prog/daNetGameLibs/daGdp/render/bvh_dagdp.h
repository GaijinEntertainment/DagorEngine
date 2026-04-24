// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entitySystem.h>
#include <math/integer/dag_IPoint2.h>
#include <shaders/dag_computeShaders.h>
#include <math/dag_hlsl_floatx.h>

class RenderableInstanceLodsResource;

namespace dagdp
{
struct DagdpBvhPreMapping
{
  const RenderableInstanceLodsResource *riRes;
  int lodIdx;
  int elemIdx;
  Color4 wind_channel_strength;
};
#include "../shaders/dagdp_bvh_mapping.hlsli"

class BvhManager
{
public:
  Ptr<ComputeShaderElement> makeRTHWInstances;
  Ptr<ComputeShaderElement> makeIndirectArgs;
  DagdpBvhMapping getMappingData(const DagdpBvhPreMapping &preMapping) const;
  void requestStaticBLAS(const RenderableInstanceLodsResource *ri) const;
  static size_t hw_instance_size();
};

BvhManager *get_bvh_manager();
} // namespace dagdp

ECS_DECLARE_BOXED_TYPE(dagdp::BvhManager);