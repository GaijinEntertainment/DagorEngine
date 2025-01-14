// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <dag/dag_vectorSet.h>
#include <dag/dag_vectorMap.h>
#include <util/dag_oaHashNameMap.h>
#include <daECS/core/entitySystem.h>
#include <util/dag_multicastEvent.h>
#include <render/dynamicShadowRenderExtensions.h>
#include "common.h"

namespace dagdp
{

#if DAGDP_DEBUG
// In debug mode, we want to be more conservative to catch issues earlier.
inline constexpr float DYNAMIC_THRESHOLD_MULTIPLIER = 0.75f;
#else
inline constexpr float DYNAMIC_THRESHOLD_MULTIPLIER = 1.0f;
#endif

struct View
{
  ViewInfo info;
  dag::Vector<dabfg::NodeHandle> nodes;
#if DAGDP_DEBUG
  uint32_t dynamicInstanceCounter;
#endif
};

struct GlobalConfig
{
  bool enabled = false;
  bool enableDynamicShadows = false;
  dynamic_shadow_render::QualityParams dynamicShadowQualityParams = {};
};

#if DAGDP_DEBUG
struct GlobalDebug
{
  dag::Vector<ViewBuilder> builders;
};
#endif

class GlobalManager
{
  GlobalConfig config;

  RulesBuilder rulesBuilder;
  bool rulesAreValid = false;

  dag::Vector<View> views;
  dag::Vector<dabfg::NodeHandle> viewIndependentNodes; // Built together with views, but not belonging to any particular one.
  bool viewsAreCreated = false;
  bool viewsAreBuilt = false;

#if DAGDP_DEBUG
  GlobalDebug debug;
#endif

  GlobalManager(const GlobalManager &) = delete;            // Non-copyable.
  GlobalManager &operator=(const GlobalManager &) = delete; // Non-copyable.
  GlobalManager(GlobalManager &&) = delete;                 // Non-movable.
  GlobalManager &operator=(GlobalManager &&) = delete;      // Non-movable.

public:
  GlobalManager() = default;
  void reconfigure(const GlobalConfig &new_config);
  void destroyViews();
  void invalidateViews();
  void invalidateRules();
  void update();
  const ViewInfo &getViewInfo(int view_index) const { return views[view_index].info; }

#if DAGDP_DEBUG
  void imgui();
#endif

private:
  static void queryLevelSettings(RulesBuilder &rules_builder);
  static void accumulateObjectGroups(RulesBuilder &rules_builder);
  static void accumulatePlacers(RulesBuilder &rules_builder);

  void recreateViews();
  void rebuildRules();
  void rebuildViews();
};

} // namespace dagdp

ECS_DECLARE_BOXED_TYPE(dagdp::GlobalManager);