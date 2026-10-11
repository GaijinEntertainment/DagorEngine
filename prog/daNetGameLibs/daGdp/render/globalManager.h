// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_mathBase.h>
#include <dag/dag_vectorSet.h>
#include <dag/dag_vectorMap.h>
#include <util/dag_oaHashNameMap.h>
#include <daECS/core/entitySystem.h>
#include <util/dag_multicastEvent.h>
#include <render/lights/dynamicShadowRenderExtensions.h>
#include "common.h"

namespace dagdp
{

// Grow the dynamic instance region when it is this full, before it actually overflows and instances get dropped.
inline constexpr float DYNAMIC_GROW_THRESHOLD = 0.85f;
inline constexpr float MIN_GLOBAL_DENSITY_MUL = 0.5f;

struct View
{
  ViewInfo info;
  dag::Vector<dafg::NodeHandle> nodes;
#if DAGDP_DEBUG
  uint32_t dynamicInstanceCounter;
#endif
};

struct GlobalConfig
{
  bool enabled = false;
  bool enableDynamicShadows = false;
  dynamic_shadow_render::QualityParams dynamicShadowQualityParams = {};

  bool operator==(const GlobalConfig &other) const
  {
    const auto &a = dynamicShadowQualityParams;
    const auto &b = other.dynamicShadowQualityParams;
    return enabled == other.enabled && enableDynamicShadows == other.enableDynamicShadows &&
           a.maxShadowsToUpdateOnFrame == b.maxShadowsToUpdateOnFrame &&
           a.maxShadowViewsToUpdateOnFrame == b.maxShadowViewsToUpdateOnFrame && a.maxShadowDist == b.maxShadowDist;
  }
};

#if DAGDP_DEBUG
struct GlobalDebug
{
  dag::Vector<ViewBuilder> builders;
};
#endif

struct RequiredLimits
{
  uint32_t maxMeshes = 0;
  uint32_t maxTiles = 0;
  uint32_t max3dTiles = 0;
  uint32_t maxVolumes = 0;
  uint32_t maxObjects = 0;
  uint32_t maxTriangles = 0;
};

class GlobalManager
{
  GlobalConfig config;

  RulesBuilder rulesBuilder;
  bool rulesAreValid = false;

  dag::Vector<View> views;
  dag::Vector<dafg::NodeHandle> viewIndependentNodes; // Built together with views, but not belonging to any particular one.
  bool viewsAreCreated = false;
  bool viewsAreBuilt = false;
  bool triangleSizeDebugEnabled = false;

  RequiredLimits grownLimits;

  // Static because gather nodes have no manager instance (they are wired up via
  // broadcast events). Gather nodes and update() run on the same thread (FG
  // execution is single-threaded), so a plain accumulator is enough.
  static RequiredLimits requiredLimits;

#if DAGDP_DEBUG
  GlobalDebug debug;
#endif

  GlobalManager(const GlobalManager &) = delete;            // Non-copyable.
  GlobalManager &operator=(const GlobalManager &) = delete; // Non-copyable.
  GlobalManager(GlobalManager &&) = delete;                 // Non-movable.
  GlobalManager &operator=(GlobalManager &&) = delete;      // Non-movable.

public:
  static float globalDensityMul;

  // Scales the emitted place count of every placer at view build time; change requires invalidateViews().
  static float clampedGlobalDensityMul() { return clamp(globalDensityMul, MIN_GLOBAL_DENSITY_MUL, 1.0f); }

  GlobalManager()
  {
    globalDensityMul = 1;
    requiredLimits = {}; // a pending report from a dead manager must not grow this one
  }
  ~GlobalManager();
  void reconfigure(const GlobalConfig &new_config);
  const GlobalConfig &getConfig() const { return config; }
  void destroyViews();
  void invalidateViews();
  void invalidateRules();
  void update(ecs::EntityManager &manager);
  static void updateRequiredLimits(const RequiredLimits &required);
  const ViewInfo &getViewInfo(int view_index) const { return views[view_index].info; }
  void setTriangleDebugView(bool is_triangle_debug) { triangleSizeDebugEnabled = is_triangle_debug; }

#if DAGDP_DEBUG
  void imgui();
#endif

private:
  static void queryLevelSettings(RulesBuilder &rules_builder);
  static void accumulateObjectGroups(RulesBuilder &rules_builder);
  static void accumulatePlacers(RulesBuilder &rules_builder);

  void recreateViews();
  void rebuildRules();
  void rebuildViews(ecs::EntityManager &manager);
  void applyRequiredLimits();
};

} // namespace dagdp

ECS_DECLARE_BOXED_TYPE(dagdp::GlobalManager);