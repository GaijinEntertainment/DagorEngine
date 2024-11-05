// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdio.h>
#include <gui/dag_stdGuiRender.h>
#include <workCycle/dag_genGuiMgr.h>
#include <math/dag_bounds2.h>
#include <generic/dag_initOnDemand.h>
#include <frameTimeMetrics/aggregator.h>
#include <startup/dag_globalSettings.h>

class De3GuiMgrDrawFps : public IGeneralGuiManager
{
  FrameTimeMetricsAggregator frameTimeMetrics;

public:
  virtual void beforeRender(int ticks_usec)
  {
    frameTimeMetrics.update(::get_time_msec(), ::dagor_frame_no(), ticks_usec / 1000.f / 1000.f, PerfDisplayMode::FPS);
  }

  virtual bool canCloseNow()
  {
    DEBUG_CTX("closing window");
    debug_flush(false);
    return true;
  }

  virtual void drawFps(float minfps, float maxfps, float lastfps)
  {
    StdGuiRender::start_render();

    const real OFS = StdGuiRender::get_str_bbox("FPS:9999.9 (999.9<999.9 999.9)").width().x;
    const real Y_POS = 40;
#if _TARGET_PC
    const real X_POS = StdGuiRender::screen_width();
#else
    const real X_POS = StdGuiRender::screen_width() - 80;
#endif
    StdGuiRender::goto_xy(X_POS - OFS, Y_POS);
    StdGuiRender::set_color(frameTimeMetrics.getColorForFpsInfo());
    StdGuiRender::draw_str(frameTimeMetrics.getFpsInfoString().c_str());

    StdGuiRender::end_render();
  }
};
