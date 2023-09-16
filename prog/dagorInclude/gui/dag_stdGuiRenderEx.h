//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gui/dag_stdGuiRender.h>

const E3DCOLOR COLOR_BLACK = E3DCOLOR_MAKE(0, 0, 0, 255);
const E3DCOLOR COLOR_WHITE = E3DCOLOR_MAKE(255, 255, 255, 255);
const E3DCOLOR COLOR_LTRED = E3DCOLOR_MAKE(255, 0, 0, 255);
const E3DCOLOR COLOR_LTGREEN = E3DCOLOR_MAKE(0, 255, 0, 255);
const E3DCOLOR COLOR_LTBLUE = E3DCOLOR_MAKE(0, 0, 224, 255);
const E3DCOLOR COLOR_YELLOW = E3DCOLOR_MAKE(255, 255, 0, 255);
const E3DCOLOR COLOR_RED = E3DCOLOR_MAKE(128, 0, 0, 255);
const E3DCOLOR COLOR_GREEN = E3DCOLOR_MAKE(0, 128, 0, 255);
const E3DCOLOR COLOR_BLUE = E3DCOLOR_MAKE(0, 0, 128, 255);


namespace StdGuiRender
{
#define DSA_OVERLOADS_PARAM_DECL real x, real y,
#define DSA_OVERLOADS_PARAM_PASS x, y,
DECLARE_DSA_OVERLOADS_FAMILY(static inline void draw_strf_to, inline void draw_strf_to, draw_strf_to);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

// render formated text string to specified position using current font (ASCII or UTF-8)
inline void draw_strf_to(real x, real y, const char *format, const DagorSafeArg *arg, int anum)
{
  goto_xy(x, y);
  draw_strv(1.0f, format, arg, anum);
}

// render filled polygon
inline void draw_fill_tri(Point2 &x0, Point2 &x1, Point2 &x2, const E3DCOLOR &col)
{
  GuiVertex v[3];
  GuiVertexTransform xf;
  xf.resetViewTm();

#define MAKEDATA(index)            \
  {                                \
    v[index].zeroTc0();            \
    v[index].zeroTc1();            \
    v[index].setPos(xf, x##index); \
    v[index].color = col;          \
  }

  MAKEDATA(0);
  MAKEDATA(1);
  MAKEDATA(2);
#undef MAKEDATA

  const StdGuiRender::IndexType indices[3] = {0, 1, 2};
  StdGuiRender::draw_faces(v, 3, indices, 1);
}

// render filled polygon fan (the last vertex is shared)
inline void draw_fill_poly_fan(Point2 *m, int num, const E3DCOLOR &col)
{
  if (num < 3)
    return;

  for (int i = 0; i < (num - 2); ++i)
    draw_fill_tri(m[i], m[i + 1], m[num - 1], col);
}

} // namespace StdGuiRender
