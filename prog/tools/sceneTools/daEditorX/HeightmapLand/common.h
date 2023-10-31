#ifndef __SPLINES_COMMON__
#define __SPLINES_COMMON__
#pragma once


#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <generic/dag_span.h>
#include <util/dag_string.h>


extern bool objectWasMoved, objectWasRotated, objectWasScaled;

bool triang_inside_triangle(float Ax, float Ay, float Bx, float By, float Cx, float Cy, float Px, float Py);
float triang_area(dag::ConstSpan<Point2> pts);
bool triang_snip(dag::ConstSpan<Point2> pts, int u, int v, int w, int n, int *V);
bool make_clockwise_coords(dag::Span<Point2> pts);
bool make_clockwise_coords(dag::Span<Point3> pts3);

bool lines_inters_ignore_Y(Point3 &pf1, Point3 &pf2, Point3 &ps1, Point3 &ps2);

#endif // __SPLINES_COMMON__
