// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/bhvFpsBar.h>

#include "stdRendObj.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_transform.h>

#include <frameTimeMetrics/aggregator.h>
#include <workCycle/dag_workCycle.h>
#include <osApiWrappers/dag_atomic.h>
#include <perfMon/dag_cpuFreq.h>
#include <startup/dag_globalSettings.h>
#include <workCycle/dag_wcHooks.h>


namespace darg
{

BhvFpsBar bhv_fps_bar;


static int fps_text_version = 0;
static uint32_t last_color = 0;

static SQInteger cast_color(E3DCOLOR val)
{
  int i = (int &)val.u;
  return SQInteger(i);
}

// Hack - unfreeze to modify
class UnfreezeRAII
{
public:
  UnfreezeRAII(Sqrat::Object &o) : obj(o), wasImmutable(o.GetObject()._flags & SQOBJ_FLAG_IMMUTABLE)
  {
    if (wasImmutable)
      obj.UnfreezeSelf();
  }

  ~UnfreezeRAII()
  {
    if (wasImmutable)
      obj.FreezeSelf();
  }

  Sqrat::Object &obj;
  bool wasImmutable;
};

BhvFpsBar::BhvFpsBar() :
  Behavior(darg::Behavior::STAGE_BEFORE_RENDER, 0),
  frameTimeMetrics(eastl::make_unique<FrameTimeMetricsAggregator>()),
  displayMode(PerfDisplayMode::OFF)
{}

void BhvFpsBar::onAttach(Element *elem)
{
  UnfreezeRAII unfreeze(elem->props.scriptDesc);
  elem->props.scriptDesc.SetValue(elem->csk->color, cast_color(frameTimeMetrics->getColorForFpsInfo()));

  if (elem->robjParams)
    elem->robjParams->load(elem);
  elem->props.text = frameTimeMetrics->getFpsInfoString().c_str();
  discard_text_cache(elem->robjParams);
}

void BhvFpsBar::setRenderingResolution(const eastl::optional<IPoint2> &resolution)
{
  frameTimeMetrics->setRenderingResolution(resolution);
}

int BhvFpsBar::update(UpdateStage /*stage*/, darg::Element *elem, float /*dt*/)
{
  const auto getFramesPresented = interlocked_acquire_load_ptr(dwc_get_frames_presented);
  frameTimeMetrics->update(::get_time_msec(), ::dagor_frames_presented(), ::dagor_game_act_time, displayMode,
    getFramesPresented ? getFramesPresented() : 1);

  // Hacks
  uint32_t color = frameTimeMetrics->getColorForFpsInfo();
  if (color != last_color)
  {
    last_color = color;

    UnfreezeRAII unfreeze(elem->props.scriptDesc);
    elem->props.scriptDesc.SetValue(elem->csk->color, cast_color(frameTimeMetrics->getColorForFpsInfo()));
    if (elem->robjParams)
      elem->robjParams->load(elem);
  }

  if (fps_text_version != frameTimeMetrics->getTextVersion())
  {
    fps_text_version = frameTimeMetrics->getTextVersion();
    elem->props.text = frameTimeMetrics->getFpsInfoString().c_str();
    discard_text_cache(elem->robjParams);
  }
  return 0;
}

void BhvFpsBar::setDisplayMode(const PerfDisplayMode &display_mode) { displayMode = display_mode; }
} // namespace darg
