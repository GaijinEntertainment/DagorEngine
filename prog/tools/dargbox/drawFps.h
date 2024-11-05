// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <stdio.h>
#include <gui/dag_stdGuiRender.h>
#include <workCycle/dag_genGuiMgr.h>
#include <math/dag_bounds2.h>
#include <generic/dag_initOnDemand.h>

class DrawFpsGuiMgr : public IGeneralGuiManager
{
public:
  virtual void beforeRender(int /*ticks_usec*/) {}

  virtual bool canCloseNow()
  {
    DEBUG_CTX("closing window");
    debug_flush(false);
    return true;
  }

  virtual void drawFps(float minfps, float maxfps, float lastfps)
  {
    StdGuiRender::start_render();
    StdGuiRender::set_font(0);

    static char s[16];
    BBox2 b;
    real x;

    sprintf(s, "%.1f", minfps);
    b = StdGuiRender::get_str_bbox(s);
    const real OFS = StdGuiRender::get_str_bbox("9999.99").width().x;
    const real Y_POS = 40;
#if _TARGET_PC
    const real X_POS = StdGuiRender::screen_width();
#else
    const real X_POS = StdGuiRender::screen_width() - 80;
#endif
    x = X_POS - OFS * 2 - 25 - (b[1].x - b[0].x);
    StdGuiRender::goto_xy(x + 2, Y_POS + 2);
    StdGuiRender::set_color(E3DCOLOR_MAKE(0, 0, 0, 255));
    StdGuiRender::draw_str(s);
    StdGuiRender::goto_xy(x, Y_POS);
    StdGuiRender::set_color(E3DCOLOR_MAKE(255, 255, 255, 255));
    StdGuiRender::draw_str(s);

    sprintf(s, "%.1f", maxfps);
    b = StdGuiRender::get_str_bbox(s);
    x = X_POS - OFS - 25 - (b[1].x - b[0].x);
    StdGuiRender::goto_xy(x + 2, Y_POS + 2);
    StdGuiRender::set_color(E3DCOLOR_MAKE(0, 0, 0, 255));
    StdGuiRender::draw_str(s);
    StdGuiRender::goto_xy(x, Y_POS);
    StdGuiRender::set_color(E3DCOLOR_MAKE(255, 255, 255, 255));
    StdGuiRender::draw_str(s);

    sprintf(s, "%.1ffps", lastfps);

    b = StdGuiRender::get_str_bbox(s);
    x = X_POS - 5 - (b[1].x - b[0].x);
    StdGuiRender::goto_xy(x + 2, Y_POS + 2);
    StdGuiRender::set_color(E3DCOLOR_MAKE(0, 0, 0, 255));
    StdGuiRender::draw_str(s);
    StdGuiRender::goto_xy(x, Y_POS);
    StdGuiRender::set_color(E3DCOLOR_MAKE(255, 255, 255, 255));
    StdGuiRender::draw_str(s);

    StdGuiRender::end_render();
  }
};
