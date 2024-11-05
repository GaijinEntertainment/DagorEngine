// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/bhvLatencyBar.h>

#include "stdRendObj.h"

#include <3d/dag_lowLatency.h>
#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_transform.h>
#include <util/dag_localization.h>

#include <perfMon/dag_cpuFreq.h>
#include <startup/dag_globalSettings.h>
#include <workCycle/dag_workCycle.h>
#include <frameTimeMetrics/aggregator.h>

namespace darg
{

BhvLatencyBar bhv_latency_bar;

BhvLatencyBar::BhvLatencyBar() :
  Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0),
  frameTimeMetrics(eastl::make_unique<FrameTimeMetricsAggregator>()),
  displayMode(PerfDisplayMode::OFF)
{}

void BhvLatencyBar::onAttach(Element *elem)
{
  discard_text_cache(elem->robjParams);
  textVersion = -1;
}

int BhvLatencyBar::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  frameTimeMetrics->update(::get_time_msec(), ::dagor_frame_no(), ::dagor_game_act_time, displayMode);
  if (textVersion != frameTimeMetrics->getTextVersion())
  {
    textVersion = frameTimeMetrics->getTextVersion();
    elem->props.text.setStr(frameTimeMetrics->getLatencyInfoString().c_str());
    discard_text_cache(elem->robjParams);
  }
  return 0;
}

void BhvLatencyBar::setDisplayMode(const PerfDisplayMode &display_mode) { displayMode = display_mode; }
} // namespace darg
