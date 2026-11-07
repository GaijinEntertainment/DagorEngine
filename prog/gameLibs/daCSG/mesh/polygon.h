// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <math/dag_Point2.h>
#include <util/dag_stdint.h>
#include <EASTL/string.h>

namespace dacsg
{
// Twice the area of a, b, c: positive for a left turn
double orient2(const DPoint2 &a, const DPoint2 &b, const DPoint2 &c);
// By the crossings of a ray toward +x
bool inside_loop(dag::ConstSpan<DPoint2> loop, const DPoint2 &p);
// Positive for CCW
double signed_area(dag::ConstSpan<Point2> pts);
// Ear clipper shared by the caps and the kernels: outer loop plus holes, any winding
// tris: indices into the outer loop's points, then into each hole's, in the order given; counterclockwise whatever the winding
bool triangulate_polygon(dag::ConstSpan<Point2> outer, dag::ConstSpan<dag::ConstSpan<Point2>> holes, dag::Vector<uint32_t> &tris,
  eastl::string &err);
// force: a stalled loop still clips, so every loop edge lands in exactly one triangle, and the outer loop keeps its order, so
// the triangles turn its way; else counterclockwise
bool triangulate_polygon(dag::ConstSpan<DPoint2> outer, dag::ConstSpan<dag::ConstSpan<DPoint2>> holes, dag::Vector<uint32_t> &tris,
  eastl::string &err, bool force = false);
} // namespace dacsg
