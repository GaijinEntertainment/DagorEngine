// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "kernelExact/exactPoint.h"

namespace dacsg::exact
{
// Every sign is exact: an interval filter decides first, then BigInt on int64grid or expansions on shewchuk (fp.cpp).

// the exact fallbacks of the calling thread since its last take; a job takes them at its end
uint64_t take_exact_fallbacks();
// a worker's take joins the calling thread's count
void add_exact_fallbacks(uint64_t n);

int sign_point_plane(const ExactPoint &p, const Plane &pl);
// sign of ((b - a) x (c - a))[axis]: the orientation in the projection that drops axis; group names the plane group
// all three points lie on when the caller knows one, so a tie on the line of that plane and another shows by the keys;
// memo is the caller's worker memo of the shewchuk backend's signs
int orient2d(const ExactPoint &a, const ExactPoint &b, const ExactPoint &c, int axis, uint32_t group = NO_INDEX,
  fp::Memo *memo = nullptr);
// sign of a[axis] - b[axis]
int compare_coord(const ExactPoint &a, const ExactPoint &b, int axis);
bool point_equal(const ExactPoint &a, const ExactPoint &b);
// sign of det(b - a, c - a, d - a) of four input vertices
int orient3d(const GridPoint &a, const GridPoint &b, const GridPoint &c, const GridPoint &d);
// the same of either backend; all four must be leaf vertices (PointKind::Grid), as a.fp picks the union member
int orient3d(const ExactPoint &a, const ExactPoint &b, const ExactPoint &c, const ExactPoint &d);
// sign of det(a.n, b.n, d), the order of two wedges around a line along d (spec 7.2 step 8)
int wedge_order(const Plane &a, const Plane &b, const ExactDir &d);
int sign_dot(const Plane &a, const Plane &b);
} // namespace dacsg::exact
