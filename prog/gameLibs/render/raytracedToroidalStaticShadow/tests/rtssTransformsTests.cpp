// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <core/rtssTransforms.h>

#include <math/dag_check_nan.h>
#include <math/random/dag_random.h>
#include <EASTL/algorithm.h>
#include <EASTL/vector.h>
#include <math.h>

using Catch::Matchers::WithinAbs;

static constexpr double DEG = M_PI / 180;
static constexpr double TEXEL0 = 0.05;
static constexpr int PAGE_TEXELS = 62;
static constexpr int WINDOW_PAGES = 16;
static constexpr double FAR_DIST = 1e5;

static double rnd(int &seed, double a, double b) { return a + (b - a) * _frnd(seed); }

static DPoint3 rnd_unit(int &seed)
{
  const double y = rnd(seed, -1, 1), phi = rnd(seed, 0, 2 * M_PI), r = sqrt(1 - y * y);
  return DPoint3(r * cos(phi), y, r * sin(phi));
}

static DPoint3 rnd_far_point(int &seed)
{
  const double phi = rnd(seed, 0, 2 * M_PI);
  return DPoint3(FAR_DIST * cos(phi), rnd(seed, 0, 500), FAR_DIST * sin(phi));
}

static Point3 sun_after_step(int &seed, const Point3 &light_dir, double angle)
{
  const DPoint3 d = dpoint3(light_dir);
  return point3(-rtss_rotate(d, normalize(d % rnd_unit(seed)), angle));
}

static RtssLightXform rnd_xform(int &seed, const DPoint3 &camera)
{
  const RtssLightXform first = rtss_first_transform(point3(rnd_unit(seed)), camera);
  const DPoint3 pivot = camera - dpoint3(first.d) * rnd(seed, 30, 400);
  return rtss_next_transform(first, sun_after_step(seed, first.d, rnd(seed, 0, 30) * DEG), pivot, camera);
}

static double ortho_error(const RtssLightXform &x)
{
  const DPoint3 ex = dpoint3(x.ex), ey = dpoint3(x.ey), d = dpoint3(x.d);
  return eastl::max({fabs(length(ex) - 1), fabs(length(ey) - 1), fabs(length(d) - 1), fabs(ex * ey), fabs(ex * d), fabs(ey * d)});
}

static bool right_handed(const RtssLightXform &x) { return (x.ex % x.ey) * x.d > 0.5f; }

static bool is_finite(const Point3 &p) { return check_finite(p.x) && check_finite(p.y) && check_finite(p.z); }

static bool is_finite(const RtssLightXform &x)
{
  return is_finite(x.ex) && is_finite(x.ey) && is_finite(x.d) && check_finite(x.c.x) && check_finite(x.c.y) && check_finite(x.c.z) &&
         check_finite(x.z0);
}

static IPoint2 camera_window_origin(const RtssLightXform &x, const DPoint3 &camera, double page_world)
{
  return rtss_page(x, camera, page_world) - IPoint2(WINDOW_PAGES / 2, WINDOW_PAGES / 2);
}

struct ShiftReference
{
  DPoint3 pivotLs;
  DPoint2 u;
  double sinTheta, oneMinusCosTheta;
};

static ShiftReference shift_reference(const RtssLightXform &old, const RtssLightXform &nw, const DPoint3 &pivot)
{
  const DPoint3 oldRayMove = nw.toLs(pivot + dpoint3(old.d) * 1000.0) - nw.toLs(pivot);
  const double moveLen = sqrt(oldRayMove.x * oldRayMove.x + oldRayMove.y * oldRayMove.y);
  const DPoint3 dOld = dpoint3(old.d), dNew = dpoint3(nw.d);
  const double theta = atan2(length(dOld % dNew), dOld * dNew);
  return {old.toLs(pivot), DPoint2(oldRayMove.x / moveLen, oldRayMove.y / moveLen), sin(theta), 2 * sqr(sin(theta / 2))};
}

static DPoint2 shift_by_formula(const ShiftReference &ref, const RtssLightXform &old, const DPoint3 &receiver, double &depth)
{
  const DPoint3 rel = old.toLs(receiver) - ref.pivotLs;
  const double alongU = rel.x * ref.u.x + rel.y * ref.u.y;
  depth = rel.z;
  return ref.u * (rel.z * ref.sinTheta - alongU * ref.oneMinusCosTheta);
}

TEST_CASE("first transform is the construction of spec 6.1, orthonormal and fixed to the world origin")
{
  int seed = 1;
  eastl::vector<Point3> dirs = {Point3(0, 1, 0), Point3(0, -1, 0), Point3(1, 0, 0), Point3(0.001f, 1, 0.001f),
    Point3(0.05f, 0.999f, 0), Point3(0.3f, 0.2f, -0.7f)};
  for (int i = 0; i < 10000; ++i)
    dirs.push_back(point3(rnd_unit(seed)));

  double worstOrtho = 0, worstDir = 0, worstRows = 0;
  bool originFixed = true, rightHanded = true, cameraDepth = true;
  for (const Point3 &dir : dirs)
  {
    const DPoint3 camera = rnd_far_point(seed);
    const RtssLightXform x = rtss_first_transform(dir, camera);
    const DPoint3 d = -normalize(dpoint3(dir)), ex = normalize((fabs(d.y) > 0.999 ? DPoint3(1, 0, 0) : DPoint3(0, 1, 0)) % d);
    worstOrtho = eastl::max(worstOrtho, ortho_error(x));
    worstDir = eastl::max(worstDir, length(dpoint3(x.d) - d));
    worstRows = eastl::max({worstRows, length(dpoint3(x.ex) - ex), length(dpoint3(x.ey) - d % ex)});
    originFixed &= x.c == DPoint3(0, 0, 0);
    cameraDepth &= fabs(x.z0 - d * camera) <= 1e-6 * length(camera); // constraint: the first transform anchors at c = 0
    rightHanded &= right_handed(x);
  }
  INFO("orthonormality error " << worstOrtho << ", direction error " << worstDir << ", row error " << worstRows);
  CHECK(worstOrtho < 1e-6);
  CHECK(worstDir < 1e-6);
  CHECK(worstRows < 1e-6);
  CHECK(originFixed);
  CHECK(cameraDepth);
  CHECK(rightHanded);
}

TEST_CASE("next transform keeps the pivot 100 km from the origin")
{
  int seed = 2;
  const double pageWorld = PAGE_TEXELS * TEXEL0;
  double worstPivot = 0, worstGpuPivot = 0, worstRoundTrip = 0, largestC = 0;
  bool finite = true;
  for (int i = 0; i < 10000; ++i)
  {
    const DPoint3 camera = rnd_far_point(seed);
    const RtssLightXform old = rnd_xform(seed, camera);
    const DPoint3 pivot = camera - dpoint3(old.d) * rnd(seed, 30, 400);
    const RtssLightXform nw = rtss_next_transform(old, sun_after_step(seed, old.d, rnd(seed, 0.01, 30) * DEG), pivot, camera);
    finite &= is_finite(nw);
    worstPivot = eastl::max(worstPivot, length(nw.toLs(pivot) - old.toLs(pivot)));
    worstRoundTrip = eastl::max(worstRoundTrip, length(nw.fromLs(nw.toLs(pivot)) - pivot) / length(pivot));
    largestC = eastl::max(largestC, length(nw.c));

    const IPoint2 origin = camera_window_origin(old, camera, pageWorld);
    Point4 oldRows[3], newRows[3];
    rtss_pack_xform(old, camera, origin, pageWorld, oldRows);
    rtss_pack_xform(nw, camera, origin, pageWorld, newRows);
    const Point3 pivotRel = point3(pivot - camera);
    const Point3 gpuMove = rtss_rows_to_ls(newRows, pivotRel) - rtss_rows_to_ls(oldRows, pivotRel);
    worstGpuPivot = eastl::max(worstGpuPivot, (double)sqrtf(gpuMove.x * gpuMove.x + gpuMove.y * gpuMove.y));
  }
  INFO("pivot move " << worstPivot << " m, float rows " << worstGpuPivot << " m, largest |c| " << largestC << " m");
  CHECK(finite);
  CHECK(worstPivot < 1e-6);
  CHECK(worstGpuPivot < 1e-3);
  CHECK(worstRoundTrip < 1e-6);
  CHECK(largestC > 1e4);
}

TEST_CASE("packed rows are small and the float lookup finds the page and texel of the double reference")
{
  int seed = 3;
  const DPoint3 camera0(70000.3, 150.7, -71400.1);
  const DPoint3 boxHalf(1500, 400, 1500);
  double worstK = 0, worstTexels = 0, worstDepth = 0;
  int receivers = 0, onBoundary = 0, pageMisses = 0, texelMisses = 0, outsideWindow = 0;
  for (int level = 0; level < 4; ++level)
  {
    const double texel = TEXEL0 * (1 << level), pageWorld = PAGE_TEXELS * texel;
    const double extent = (WINDOW_PAGES / 2 - 1) * pageWorld;
    const float invTexel = float(1 / texel), invPageWorld = float(1 / pageWorld);
    for (int i = 0; i < 200; ++i)
    {
      const RtssLightXform x = rnd_xform(seed, camera0);
      const DPoint3 camera = camera0 + DPoint3(rnd(seed, -1000, 1000), rnd(seed, -300, 300), rnd(seed, -1000, 1000));
      double zMin = 1e30, zMax = -1e30;
      for (int corner = 0; corner < 8; ++corner)
      {
        const DPoint3 sign((corner & 1) ? 1 : -1, (corner & 2) ? 1 : -1, (corner & 4) ? 1 : -1);
        const double z = x.toLs(camera0 + DPoint3(sign.x * boxHalf.x, sign.y * boxHalf.y, sign.z * boxHalf.z)).z;
        zMin = eastl::min(zMin, z);
        zMax = eastl::max(zMax, z);
      }
      const IPoint2 origin = camera_window_origin(x, camera, pageWorld);
      Point4 rows[3];
      rtss_pack_xform(x, camera, origin, pageWorld, rows);
      const double k = sqrt(sqr(double(rows[0].w)) + sqr(double(rows[1].w)) + sqr(double(rows[2].w)));
      worstK = eastl::max(worstK, k / (WINDOW_PAGES * pageWorld + zMax - zMin));

      for (int j = 0; j < 100; ++j)
      {
        const DPoint3 receiver = camera + dpoint3(x.ex) * rnd(seed, -extent, extent) + dpoint3(x.ey) * rnd(seed, -extent, extent) +
                                 dpoint3(x.d) * rnd(seed, -extent, extent);
        const DPoint3 ls = x.toLs(receiver);
        const DPoint2 texelPos(ls.x / texel, ls.y / texel);
        const DPoint2 texelFrac(texelPos.x - floor(texelPos.x), texelPos.y - floor(texelPos.y));
        ++receivers;
        if (eastl::min({texelFrac.x, 1 - texelFrac.x, texelFrac.y, 1 - texelFrac.y}) < 1e-3)
        {
          ++onBoundary;
          continue;
        }
        const IPoint2 page = rtss_page(x, receiver, pageWorld);
        const IPoint2 slotTexel(int(floor(texelPos.x)) - page.x * PAGE_TEXELS + 1, int(floor(texelPos.y)) - page.y * PAGE_TEXELS + 1);
        outsideWindow +=
          page.x < origin.x || page.y < origin.y || page.x >= origin.x + WINDOW_PAGES || page.y >= origin.y + WINDOW_PAGES;

        const Point3 lsF = rtss_rows_to_ls(rows, point3(receiver - camera));
        const IPoint2 pageF(origin.x + int(floorf(lsF.x * invPageWorld)), origin.y + int(floorf(lsF.y * invPageWorld)));
        const IPoint2 slotTexelF(int(floorf(lsF.x * invTexel - float((pageF.x - origin.x) * PAGE_TEXELS) + 1)),
          int(floorf(lsF.y * invTexel - float((pageF.y - origin.y) * PAGE_TEXELS) + 1)));
        pageMisses += pageF != page;
        texelMisses += slotTexelF != slotTexel;
        worstDepth = eastl::max(worstDepth, fabs(lsF.z - (ls.z - x.z0)));
        worstTexels = eastl::max(worstTexels,
          eastl::max(fabs(lsF.x - (ls.x - origin.x * pageWorld)), fabs(lsF.y - (ls.y - origin.y * pageWorld))) / texel);
      }
    }
  }
  INFO("|k| / (window + box depth) " << worstK << ", xy error " << worstTexels << " texels, depth error " << worstDepth
                                     << " m, on a texel boundary " << onBoundary << " of " << receivers);
  CHECK(worstK < 1);
  CHECK(worstTexels < 1e-3);
  CHECK(pageMisses == 0);
  CHECK(texelMisses == 0);
  CHECK(outsideWindow == 0);
  CHECK(worstDepth < 1e-3);
  CHECK(onBoundary * 100 < receivers);
}

TEST_CASE("the shift of spec 6.2 matches a direct transform")
{
  int seed = 4;
  double worstAlong = 0, worstAlongExcess = 0, worstPerp = 0;
  bool finite = true;
  for (int i = 0; i < 10000; ++i)
  {
    const double extent = (WINDOW_PAGES / 2 - 1) * PAGE_TEXELS * TEXEL0 * (1 << _rnd_int(seed, 0, 3));
    const double pivotDepth = extent * rnd(seed, 1.5, 2.5);
    const double stepDeg = rnd(seed, 0.01, 30);
    const DPoint3 camera = rnd_far_point(seed);
    const RtssLightXform old = rnd_xform(seed, camera);
    const DPoint3 pivot = camera - dpoint3(old.d) * pivotDepth;
    const RtssLightXform nw = rtss_next_transform(old, sun_after_step(seed, old.d, stepDeg * DEG), pivot, camera);
    finite &= is_finite(nw);
    const DPoint3 receiver = camera + dpoint3(old.ex) * rnd(seed, -extent, extent) + dpoint3(old.ey) * rnd(seed, -extent, extent) +
                             dpoint3(old.d) * rnd(seed, -extent, extent);

    const ShiftReference ref = shift_reference(old, nw, pivot);
    double depth = 0;
    const DPoint2 expected = shift_by_formula(ref, old, receiver, depth);
    const DPoint3 direct = nw.toLs(receiver) - old.toLs(receiver);
    const DPoint2 error(direct.x - expected.x, direct.y - expected.y);
    const double alongError = fabs(error * ref.u), expectedLen = sqrt(expected * expected);
    if (stepDeg >= 0.5)
      worstAlong = eastl::max(worstAlong, alongError / expectedLen);
    worstAlongExcess = eastl::max(worstAlongExcess, alongError - 1e-4 * expectedLen);
    worstPerp = eastl::max(worstPerp, fabs(error.x * ref.u.y - error.y * ref.u.x) / depth);
  }
  INFO("along u " << worstAlong << " of the shift from 0.5 deg, " << worstAlongExcess << " m above 1e-4 of the shift, perpendicular "
                  << worstPerp << " m for each m of depth");
  CHECK(finite);
  CHECK(worstAlong < 1e-4);
  CHECK(worstAlongExcess < 5e-5);
  CHECK(worstPerp < 1e-5);
}

static double shift_dir_error(int &seed, double step_deg, int &undefined)
{
  const DPoint3 camera = rnd_far_point(seed);
  const RtssLightXform old = rnd_xform(seed, camera);
  const DPoint3 pivot = camera - dpoint3(old.d) * rnd(seed, 30, 400);
  const RtssLightXform nw = rtss_next_transform(old, sun_after_step(seed, old.d, step_deg * DEG), pivot, camera);
  Point2 u;
  if (!is_finite(nw))
    return HUGE_VAL;
  if (!rtss_shift_dir(old, nw, u))
  {
    ++undefined;
    return 0;
  }
  const DPoint2 refU = shift_reference(old, nw, pivot).u;
  return fabs(atan2(u.x * refU.y - u.y * refU.x, u.x * refU.x + u.y * refU.y));
}

TEST_CASE("shift direction matches the direct transform for steps of 0.01 to 30 deg and is undefined at 0 and 180 deg")
{
  int seed = 5;
  double worstSmall = 0, worstLarge = 0;
  int undefined = 0;
  for (int i = 0; i < 3000; ++i)
    worstSmall = eastl::max(worstSmall, shift_dir_error(seed, rnd(seed, 0.01, 0.1), undefined));
  for (int i = 1; i <= 300; ++i)
    worstLarge = eastl::max(worstLarge, shift_dir_error(seed, i * 0.1, undefined));
  INFO("worst angle " << worstSmall << " rad for 0.01 to 0.1 deg, " << worstLarge << " rad for 0.1 to 30 deg");
  CHECK(undefined == 0);
  CHECK(worstSmall < 1e-4);
  CHECK(worstLarge < 1e-4);

  const DPoint3 camera = rnd_far_point(seed);
  const RtssLightXform old = rnd_xform(seed, camera);
  Point2 u;
  for (const Point3 &sun : {-old.d, old.d})
    CHECK_FALSE(rtss_shift_dir(old, rtss_next_transform(old, sun, camera - dpoint3(old.d) * 100.0, camera), u));
}

TEST_CASE("steps of 179.5 and 180 deg give a finite orthonormal basis")
{
  int seed = 6;
  double worstOrtho = 0, worstDir = 0, worstPivot = 0;
  bool finite = true, rightHanded = true;
  for (int i = 0; i < 1000; ++i)
  {
    const DPoint3 camera = rnd_far_point(seed);
    const RtssLightXform old = rnd_xform(seed, camera);
    const DPoint3 pivot = camera - dpoint3(old.d) * rnd(seed, 30, 400);
    for (const Point3 &sun : {sun_after_step(seed, old.d, 179.5 * DEG), old.d})
    {
      const RtssLightXform nw = rtss_next_transform(old, sun, pivot, camera);
      finite &= is_finite(nw);
      rightHanded &= right_handed(nw);
      worstOrtho = eastl::max(worstOrtho, ortho_error(nw));
      worstDir = eastl::max(worstDir, (double)length(nw.d + normalize(sun)));
      worstPivot = eastl::max(worstPivot, length(nw.toLs(pivot) - old.toLs(pivot)));
    }
  }
  INFO("orthonormality error " << worstOrtho << ", direction error " << worstDir << ", pivot move " << worstPivot << " m");
  CHECK(finite);
  CHECK(rightHanded);
  CHECK(worstOrtho < 1e-6);
  CHECK(worstDir < 1e-6);
  CHECK(worstPivot < 1e-6);
}

TEST_CASE("100k steps of 0.1 deg stay orthonormal")
{
  const DPoint3 camera(FAR_DIST, 120, -3000);
  const DPoint3 sun0 = normalize(DPoint3(1, 0.6, 0.1)), tilt(0.3, 1, 0.2);
  const DPoint3 sunAxis = normalize(tilt - sun0 * (tilt * sun0));
  RtssLightXform x = rtss_first_transform(point3(sun0), camera);
  double worstOrtho = 0, worstDir = 0;
  for (int i = 1; i <= 100000; ++i)
  {
    const DPoint3 sun = rtss_rotate(sun0, sunAxis, i * 0.1 * DEG);
    x = rtss_next_transform(x, point3(sun), camera - dpoint3(x.d) * 65.1, camera);
    worstOrtho = eastl::max(worstOrtho, ortho_error(x));
    worstDir = eastl::max(worstDir, length(dpoint3(x.d) + sun));
  }
  INFO("orthonormality error " << worstOrtho << ", direction error " << worstDir);
  CHECK(worstOrtho < 1e-5);
  CHECK(worstDir < 1e-6);
  CHECK(is_finite(x));
  CHECK(right_handed(x));
}

TEST_CASE("orthonormalize keeps the light direction")
{
  int seed = 7;
  double worstOrtho = 0, worstDir = 0;
  bool rightHanded = true;
  for (int i = 0; i < 10000; ++i)
  {
    RtssLightXform x = rnd_xform(seed, rnd_far_point(seed));
    x.ex += point3(rnd_unit(seed)) * 1e-3f;
    x.ey += point3(rnd_unit(seed)) * 1e-3f;
    x.d = x.d * 1.001f + point3(rnd_unit(seed)) * 1e-3f;
    const Point3 dir = normalize(x.d);
    rtss_orthonormalize(x);
    worstOrtho = eastl::max(worstOrtho, ortho_error(x));
    worstDir = eastl::max(worstDir, (double)length(x.d - dir));
    rightHanded &= right_handed(x);
  }
  INFO("orthonormality error " << worstOrtho << ", direction error " << worstDir);
  CHECK(worstOrtho < 1e-6);
  CHECK(worstDir < 1e-6);
  CHECK(rightHanded);
}

TEST_CASE("rtss_rotate turns X about Z by 90 deg into Y and keeps the axis part")
{
  CHECK(length(rtss_rotate(DPoint3(1, 0, 0), DPoint3(0, 0, 1), 90 * DEG) - DPoint3(0, 1, 0)) < 1e-12);
  CHECK(length(rtss_rotate(DPoint3(1, 0, 2), DPoint3(0, 0, 1), 90 * DEG) - DPoint3(0, 1, 2)) < 1e-12);
}

TEST_CASE("window limits of spec 8.8")
{
  CHECK_THAT(rtss_window_limit_rad(4, 1.5f) / DEG, WithinAbs(26.17, 0.01));
  CHECK_THAT(rtss_window_limit_rad(8, 1.5f) / DEG, WithinAbs(7.88, 0.01));
  CHECK_THAT(rtss_window_limit_rad(16, 1.5f) / DEG, WithinAbs(3.31, 0.01));
  CHECK_THAT(rtss_window_limit_rad(32, 1.5f) / DEG, WithinAbs(1.54, 0.01));
  CHECK_THAT(rtss_window_limit_rad(16, 2.15f) / DEG, WithinAbs(2.62, 0.01));
  CHECK_THAT(rtss_window_limit_rad(16, 1.5f, 2.f) / DEG, WithinAbs(1.64, 0.01));

  int seed = 8;
  const double extent = (WINDOW_PAGES / 2 - 1) * PAGE_TEXELS * TEXEL0, pivotDepth = 1.5 * extent;
  const DPoint3 camera = rnd_far_point(seed);
  const RtssLightXform old = rnd_xform(seed, camera);
  const DPoint3 pivot = camera - dpoint3(old.d) * pivotDepth;
  const float limit = rtss_window_limit_rad(WINDOW_PAGES, 1.5f);
  const RtssLightXform nw = rtss_next_transform(old, sun_after_step(seed, old.d, limit), pivot, camera);
  Point2 u;
  REQUIRE(rtss_shift_dir(old, nw, u));
  const DPoint3 faceCorner = pivot + (dpoint3(old.ex) * u.x + dpoint3(old.ey) * u.y) * extent + dpoint3(old.d) * (pivotDepth + extent);
  const DPoint3 move = nw.toLs(faceCorner) - old.toLs(faceCorner);
  CHECK_THAT((move.x * u.x + move.y * u.y) / (PAGE_TEXELS * TEXEL0), WithinAbs(1, 1e-3));
}

TEST_CASE("anisotropic transforms give square ground texels and keep the pivot and the shift")
{
  auto sunAt = [](double elev_deg) { return Point3(float(cos(elev_deg * DEG)), float(sin(elev_deg * DEG)), 0.f); };
  CHECK(rtss_aniso_scale(sunAt(90), 8) == 1.f);
  CHECK(rtss_aniso_scale(sunAt(30), 8) == 2.f);
  CHECK(rtss_aniso_scale(sunAt(14), 8) == 4.f);
  CHECK(rtss_aniso_scale(sunAt(3), 8) == 8.f);
  CHECK(rtss_aniso_scale(sunAt(14), 1) == 1.f);

  const DPoint3 camera(100, 50, -30), up(0, 1, 0);
  const RtssLightXform x = rtss_first_transform(sunAt(14), camera, rtss_aniso_scale(sunAt(14), 8));
  const DPoint3 p = camera + DPoint3(13, -7, 21);
  CHECK(length(x.fromLs(x.toLs(p)) - p) < 1e-4);
  auto groundLen = [&](const RtssLightXform &t, const DPoint3 &v) {
    const DPoint3 d = dpoint3(t.d);
    return length(v - d * ((v * up) / (d * up)));
  };
  CHECK_THAT(groundLen(x, dpoint3(x.ex)), WithinAbs(1, 1e-6));
  CHECK_THAT(groundLen(x, dpoint3(x.ey) / x.sy), WithinAbs(1, 0.05));

  const DPoint3 d = dpoint3(x.d), pivot = camera - d * 300.0;
  const RtssLightXform same = rtss_next_transform(x, sunAt(14.5), pivot, camera, x.sy);
  const Point3 sun20(float(0.8 * cos(20 * DEG)), float(sin(20 * DEG)), float(0.6 * cos(20 * DEG)));
  const RtssLightXform scaled = rtss_next_transform(x, sun20, pivot, camera, x.sy * 0.5f);
  CHECK(length(same.toLs(pivot) - x.toLs(pivot)) < 1e-6);
  CHECK(length(scaled.toLs(pivot) - x.toLs(pivot)) < 1e-6);
  CHECK_THAT(groundLen(scaled, dpoint3(scaled.ey) / scaled.sy) * sin(20 * DEG) * 2, WithinAbs(1, 1e-3));

  Point2 u;
  REQUIRE(rtss_shift_dir(x, same, u));
  for (const DPoint3 &r : {pivot + d * 40.0, pivot + d * 120.0 + dpoint3(x.ex) * 30.0})
  {
    const DPoint3 move = same.toLs(r) - x.toLs(r);
    CHECK(fabs(move.x * u.y - move.y * u.x) / sqrt(move.x * move.x + move.y * move.y) < 1e-4);
  }
  REQUIRE(rtss_shift_dir(x, scaled, u));
  const DPoint3 onRay = pivot + d * 40.0, move = scaled.toLs(onRay) - x.toLs(onRay);
  CHECK(fabs(move.x * u.y - move.y * u.x) / sqrt(move.x * move.x + move.y * move.y) < 1e-4);
}
