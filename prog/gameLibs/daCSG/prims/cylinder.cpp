// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "prims/prims.h"
#include <math.h>

namespace dacsg
{
// A zero radius is an apex
static void frustum(PrimBuilder &b, float r_bottom, float r_top, float h, uint32_t seg)
{
  dag::Vector<ProfilePoint> prof;
  if (r_bottom > 0)
    prof.push_back(ProfilePoint{Point2(0, -0.5f * h), 0, 0, true});
  prof.push_back(ProfilePoint{Point2(r_bottom, -0.5f * h), 0, SMOOTH, false});
  prof.push_back(ProfilePoint{Point2(r_top, 0.5f * h), 1, 0, true});
  if (r_top > 0)
    prof.push_back(ProfilePoint{Point2(0, 0.5f * h), 1, 0, false});
  lathe(b, prof, false, seg, 360);
}

bool make_cylinder(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float r = a.d.p[PrimDesc::RADIUS], h = a.d.p[PrimDesc::HEIGHT];
  if (!positive(r) || !positive(h))
    return fail(err, "radius and height must be positive");
  frustum(b, r, r, h, a.seg);
  return true;
}

bool make_cone(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float rb = a.d.p[PrimDesc::RADIUS], rt = a.d.p[PrimDesc::CONE_RADIUS_TOP], h = a.d.p[PrimDesc::CONE_HEIGHT];
  if (!check_finite(rb) || !check_finite(rt) || rb < 0 || rt < 0 || (rb == 0 && rt == 0))
    return fail(err, "radii must not be negative and one must be positive");
  if (!positive(h))
    return fail(err, "height must be positive");
  frustum(b, rb, rt, h, a.seg);
  return true;
}

bool make_ccyl(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float r = a.d.p[PrimDesc::RADIUS], h = a.d.p[PrimDesc::HEIGHT];
  if (!positive(r) || !positive(h) || !positive(a.d.p[PrimDesc::CCYL_CHAMFER]))
    return fail(err, "radius, height and chamfer must be positive");
  if (a.d.p[PrimDesc::CCYL_CHAMFER] >= r || 2 * a.d.p[PrimDesc::CCYL_CHAMFER] > h * (1 + 1e-6f))
    return fail(err, "chamfer must be below r and at most h / 2");
  // a chamfer within the tolerance of h / 2, from either side, keeps the radius-r ring one ring at the centre
  const float c = eastl::min(a.d.p[PrimDesc::CCYL_CHAMFER], 0.5f * h);
  const double side = 2 * c < h * (1 - 1e-6f) ? h - 2 * c : 0, slant = c * sqrt(2.0), len = 2 * slant + side;
  const float y0 = 0.5f * h, y1 = side > 0 ? y0 - c : 0;
  dag::Vector<ProfilePoint> prof;
  prof.push_back(ProfilePoint{Point2(0, -y0), 0, 0, true});
  prof.push_back(ProfilePoint{Point2(r - c, -y0), 0, SMOOTH, false});
  prof.push_back(ProfilePoint{Point2(r, -y1), float(slant / len), SMOOTH, false});
  if (side > 0)
    prof.push_back(ProfilePoint{Point2(r, y1), float((slant + side) / len), SMOOTH, false});
  prof.push_back(ProfilePoint{Point2(r - c, y0), 1, 0, true});
  prof.push_back(ProfilePoint{Point2(0, y0), 1, 0, false});
  lathe(b, prof, false, a.seg, 360);
  return true;
}
} // namespace dacsg
