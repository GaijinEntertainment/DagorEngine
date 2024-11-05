//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/shadowSystem.h>
#include <render/spotLightsManager.h>
#include <EASTL/fixed_function.h>
#include <EASTL/unique_ptr.h>

#include <shaders/dag_computeShaders.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <dag/dag_vectorMap.h>

class DistanceReadbackLights
{
  ShadowSystem *lightShadows;
  SpotLightsManager *spotLights;
  eastl::unique_ptr<ComputeShaderElement> findMaxDepth2D;
  eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>> maxValueBuffer;
  RingCPUBufferLock resultRingBuffer;
  dag::VectorMap<int, int> lightLogCount;

  int lastNonOptId;
  bool processing;

  using RenderStaticCallback = void(mat44f_cref globTm, const TMatrix &viewItm, int updateIndex, int frustumIndex,
    DynamicShadowRenderGPUObjects render_gpu_objects);
  void dispatchQuery(eastl::fixed_function<sizeof(void *) * 2, RenderStaticCallback> render_static);
  void completeQuery();

public:
  DistanceReadbackLights(ShadowSystem *shadowSystem, SpotLightsManager *spotLights, const char *name_suffix);
  void update(eastl::fixed_function<sizeof(void *) * 2, RenderStaticCallback> render_static);
};
