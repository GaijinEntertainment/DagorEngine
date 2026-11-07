// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "prims/prims.h"

namespace dacsg
{
bool make_torus(const PrimArgs &a, PrimBuilder &b, eastl::string &err)
{
  const float rMajor = a.d.p[PrimDesc::RADIUS], rMinor = a.d.p[PrimDesc::TORUS_RADIUS_MINOR];
  if (!positive(rMajor) || !positive(rMinor))
    return fail(err, "radii must be positive");
  if (rMinor >= rMajor)
    return fail(err, "minor radius must be below the major radius");
  // v seam at the inner equator
  dag::Vector<ProfilePoint> prof(a.segMinor);
  for (uint32_t k = 0; k < a.segMinor; ++k)
  {
    const Point2 d = unit_dir(360.0 * k / a.segMinor - 180);
    prof[k] = ProfilePoint{Point2(rMajor + rMinor * d.x, rMinor * d.y), float(k) / a.segMinor, SMOOTH, false, d};
  }
  lathe(b, prof, true, a.seg, 360);
  return true;
}
} // namespace dacsg
