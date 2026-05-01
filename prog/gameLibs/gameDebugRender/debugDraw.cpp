// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameDebugRender/debugDraw.h>

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
#include <gui/dag_stdGuiRender.h>
#include <gameDebugRender/primitives2dCache.h>

const float crosshairSize = 0.01f;

void game_dbg::draw_crosshair(Primitives2dCache &prim_cache)
{
  float cx = StdGuiRender::screen_width() * 0.5f;
  float cy = StdGuiRender::screen_height() * 0.5f;
  float pixels = StdGuiRender::screen_width() * crosshairSize;
  prim_cache.addLine({cx - pixels, cy}, {cx + pixels, cy}, 2.f, E3DCOLOR_MAKE(255, 255, 255, 255));
  prim_cache.addLine({cx, cy - pixels}, {cx, cy + pixels}, 2.f, E3DCOLOR_MAKE(255, 255, 255, 255));
}
#endif
