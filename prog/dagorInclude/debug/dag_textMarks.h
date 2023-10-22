//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_e3dColor.h>


class Point3;

bool cvt_debug_text_pos(const TMatrix4 &glob_tm, const Point3 &wp, float &out_cx, float &out_cy);
void add_debug_text_mark(float scr_cx, float scr_cy, const char *str, int length = -1, float line_ofs = 0,
  E3DCOLOR frame_color = E3DCOLOR_MAKE(0, 0, 0, 160));

void add_debug_text_mark(const Point3 &wp, const char *str, int length = -1, float line_ofs = 0,
  E3DCOLOR frame_color = E3DCOLOR_MAKE(0, 0, 0, 160));

void render_debug_text_marks();
void reset_debug_text_marks();
void shutdown_debug_text_marks();
