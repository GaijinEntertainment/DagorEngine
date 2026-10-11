// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <math/dag_Point3.h>

class BezierSpline3d;

namespace levelsplines
{
class SplineIntersections;

// Index in the span is the spline id the intersection graph refers to.
struct DebugSpline
{
  const char *name;
  const BezierSpline3d *spl;
};

// Console: app.debug_level_splines [off] [lines] [nodes] [names] [spline name]
// No args turns the whole draw on when it is off, and off when it is on. Keys pick the parts, in any order;
// any other word is a spline name.
bool is_debug_draw_on();

// Call between begin_draw_cached_debug_lines() and end_draw_cached_debug_lines(); paths may be null.
// Names go through add_debug_text_mark(), so they show only in a frame that prepares and renders debug text marks.
// When the spline name given to the command matches no spline, or a spline with no curve, it prints a message
// and turns the draw off.
// The console command registers when splineDebugDraw.cpp is linked, so a build that must not have it references none of it.
void draw_debug_splines(dag::ConstSpan<DebugSpline> splines, const SplineIntersections *paths, const Point3 &cam_pos);
} // namespace levelsplines
