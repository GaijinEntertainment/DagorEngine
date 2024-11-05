//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/integer/dag_IPoint2.h>
#include <daRg/dag_behavior.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>

class FrameTimeMetricsAggregator;

enum class PerfDisplayMode;

namespace darg
{

class BhvFpsBar : public darg::Behavior
{
  eastl::unique_ptr<FrameTimeMetricsAggregator> frameTimeMetrics;
  PerfDisplayMode displayMode;

public:
  BhvFpsBar();
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override;
  virtual void onAttach(Element *elem) override;

  void setDisplayMode(const PerfDisplayMode &display_mode);
  void setRenderingResolution(const eastl::optional<IPoint2> &resolution);
};

extern BhvFpsBar bhv_fps_bar;

} // namespace darg
