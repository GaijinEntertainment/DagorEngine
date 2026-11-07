// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "mesh/solidUtil.h"

namespace dacsg
{
// a and b point to three points each; contact within the tolerance (spec.md 5.1) only touches; from-to is the crossing
// segment, or the face centers of a coplanar overlap, and both are undefined on a false return
bool triangles_intersect(const DPoint3 *a, const DPoint3 *b, double tolerance, DPoint3 &from, DPoint3 &to);

// Half the signed solid angle of triangle a, b, c seen from p: a term of the generalized winding number
double half_solid_angle(const DPoint3 &p, const DPoint3 &a, const DPoint3 &b, const DPoint3 &c);
// The half solid angles of a closed surface sum above pi around a point inside it
bool winds_around(double half_angles);
} // namespace dacsg
