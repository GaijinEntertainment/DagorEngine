//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daRg/dag_behavior.h>
#include <EASTL/unique_ptr.h>

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
};

extern BhvFpsBar bhv_fps_bar;

} // namespace darg
