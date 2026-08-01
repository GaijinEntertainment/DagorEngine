//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/array.h>
#include <vecmath/dag_vecMathDecl.h>
#include <generic/dag_relocatableFixedVector.h>

// TODO: should also be moved to the namespace.
enum class DynamicShadowRenderGPUObjects
{
  NO,
  YES
};

namespace dynamic_shadow_render
{

struct FrameUpdate
{
  struct View
  {
    mat44f invView;
    mat44f view;
  };

  eastl::array<View, 6> views;
  mat44f proj;
  int numViews;
  float maxDrawDistance;
  DynamicShadowRenderGPUObjects renderGPUObjects;
};

struct QualityParams
{
  int maxShadowsToUpdateOnFrame;
  float maxShadowDist;
};

static constexpr int ESTIMATED_MAX_SHADOWS_TO_UPDATE_PER_FRAME = 5; // See dynamicShadowsMaxUpdatePerFrame.

template <typename T>
using FrameVector = dag::RelocatableFixedVector<T, ESTIMATED_MAX_SHADOWS_TO_UPDATE_PER_FRAME, true>;

using FrameUpdates = FrameVector<FrameUpdate>;

} // namespace dynamic_shadow_render