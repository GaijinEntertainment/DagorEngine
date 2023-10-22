#include <math/dag_SHmath.h>
#include <math/dag_SHlight.h>
#include <math/dag_TMatrix.h>
#include <debug/dag_debug.h>


const SH3Lighting SH3Lighting::ZERO(Color3(0, 0, 0));
const SHDirLighting SHDirLighting::ZERO;

Color3 SH3Lighting::getDiffuseLighting(const Point3 &n) const
{
  const real c1 = 0.429043f;
  const real c2 = 0.511664f;
  const real c3 = 0.743125f;
  const real c4 = 0.886227f;
  const real c5 = 0.247708f;

  return (c1 * (n.x * n.x - n.y * n.y)) * sh[SPHHARM_2p2] + (c3 * n.z * n.z - c5) * sh[SPHHARM_20] + c4 * sh[SPHHARM_00] +
         (2 * c1 * n.x * n.z) * sh[SPHHARM_2p1] + (2 * c1 * n.y * n.z) * sh[SPHHARM_2m1] + (2 * c1 * n.x * n.y) * sh[SPHHARM_2m2] +
         (2 * c2 * n.x) * sh[SPHHARM_1p1] + (2 * c2 * n.z) * sh[SPHHARM_10] + (2 * c2 * n.y) * sh[SPHHARM_1m1];
}


Color3 SH3Lighting::getSample(const Point3 &n) const
{
  return sh[SPHHARM_00] * SPHHARM_COEF_0 + (sh[SPHHARM_1m1] * n.y + sh[SPHHARM_10] * n.z + sh[SPHHARM_1p1] * n.x) * SPHHARM_COEF_1 +
         (sh[SPHHARM_2m2] * n.x * n.y + sh[SPHHARM_2m1] * n.y * n.z + sh[SPHHARM_2p1] * n.x * n.z) * SPHHARM_COEF_21 +

         sh[SPHHARM_20] * (3 * n.z * n.z - 1) * SPHHARM_COEF_20 + sh[SPHHARM_2p2] * (n.x * n.x - n.y * n.y) * SPHHARM_COEF_22;
}

void SH3StoredLighting::dump()
{
  for (int i = 0; i < SPHHARM_NUM3; ++i)
    debug("  % f  % f  % f", sh[i].r, sh[i].g, sh[i].b);
}


void rotate_sphharm(const Color3 worldSH[SPHHARM_NUM3], Color3 localSH[SPHHARM_NUM3], const TMatrix &tm)
{
  real x0 = tm[0][0], y0 = tm[0][1], z0 = tm[0][2];
  real x1 = tm[1][0], y1 = tm[1][1], z1 = tm[1][2];
  real x2 = tm[2][0], y2 = tm[2][1], z2 = tm[2][2];

  const real c1 = 0.546274f;
  const real c3 = 0.946176f;

  localSH[SPHHARM_00] = worldSH[SPHHARM_00];

  localSH[SPHHARM_1m1] = x1 * worldSH[SPHHARM_1p1] + y1 * worldSH[SPHHARM_1m1] + z1 * worldSH[SPHHARM_10];
  localSH[SPHHARM_10] = x2 * worldSH[SPHHARM_1p1] + y2 * worldSH[SPHHARM_1m1] + z2 * worldSH[SPHHARM_10];
  localSH[SPHHARM_1p1] = x0 * worldSH[SPHHARM_1p1] + y0 * worldSH[SPHHARM_1m1] + z0 * worldSH[SPHHARM_10];

  if (check_nan(worldSH[SPHHARM_NUM3 - 1].b))
  {
    // treat higher harmonics as 2 dirlts: Point3 dir1, dir2; Color3 c1, c2;
    Matrix3 itm = inverse(Matrix3((float *)tm.m));
    (Point3 &)localSH[4] = itm * (const Point3 &)worldSH[4];
    (Point3 &)localSH[5] = itm * (const Point3 &)worldSH[5];
    memcpy(&localSH[6], &worldSH[6], sizeof(Color3) * 2);
    localSH[SPHHARM_NUM3 - 1] = Color3(0, 0, realQNaN);
    return;
  }

  localSH[SPHHARM_2m2] = (x1 * y0 + y1 * x0) * worldSH[SPHHARM_2m2] + (z1 * y0 + y1 * z0) * worldSH[SPHHARM_2m1] +
                         z1 * z0 * (c3 / c1) * worldSH[SPHHARM_20] + (z1 * x0 + x1 * z0) * worldSH[SPHHARM_2p1] +
                         (x1 * x0 - y1 * y0) * worldSH[SPHHARM_2p2];
  localSH[SPHHARM_2m1] = (x2 * y1 + y2 * x1) * worldSH[SPHHARM_2m2] + (z2 * y1 + y2 * z1) * worldSH[SPHHARM_2m1] +
                         z2 * z1 * (c3 / c1) * worldSH[SPHHARM_20] + (z2 * x1 + x2 * z1) * worldSH[SPHHARM_2p1] +
                         (x2 * x1 - y2 * y1) * worldSH[SPHHARM_2p2];
  localSH[SPHHARM_20] = x2 * y2 * (2 * c1 / c3) * worldSH[SPHHARM_2m2] + y2 * z2 * (2 * c1 / c3) * worldSH[SPHHARM_2m1] +
                        (z2 * z2) * worldSH[SPHHARM_20] + x2 * z2 * (2 * c1 / c3) * worldSH[SPHHARM_2p1] +
                        (x2 * x2 - y2 * y2) * (c1 / c3) * worldSH[SPHHARM_2p2];
  localSH[SPHHARM_2p1] = (x2 * y0 + y2 * x0) * worldSH[SPHHARM_2m2] + (z2 * y0 + y2 * z0) * worldSH[SPHHARM_2m1] +
                         z2 * z0 * (c3 / c1) * worldSH[SPHHARM_20] + (z2 * x0 + x2 * z0) * worldSH[SPHHARM_2p1] +
                         (x2 * x0 - y2 * y0) * worldSH[SPHHARM_2p2];
  localSH[SPHHARM_2p2] = (2 * x0 * y0) * worldSH[SPHHARM_2m2] + (2 * y0 * z0) * worldSH[SPHHARM_2m1] +
                         z0 * z0 * (c3 / c1) * worldSH[SPHHARM_20] + (2 * x0 * z0) * worldSH[SPHHARM_2p1] +
                         (x0 * x0 - y0 * y0) * worldSH[SPHHARM_2p2];
}

void add_hemisphere_sphharm(Color3 outputSH[SPHHARM_NUM3], const Point3 &dir, real cos_a, const Color3 &color)
{
  real cos2_a = cos_a * cos_a;

  real L0 = SPHHARM_COEF_0 * TWOPI * (1 - cos_a);
  real L1 = SPHHARM_COEF_1 * PI * (cos2_a - 1);

  real z0 = -dir.x;
  real z1 = -dir.y;
  real z2 = -dir.z;

  outputSH[SPHHARM_00] += L0 * color;

  outputSH[SPHHARM_1m1] += (z1 * L1) * color;
  outputSH[SPHHARM_10] += (z2 * L1) * color;
  outputSH[SPHHARM_1p1] += (z0 * L1) * color;

  if (check_nan(outputSH[SPHHARM_NUM3 - 1].b))
  {
    // treat higher harmonics as 2 dirlts: Point3 dir1, dir2; Color3 c1, c2;
    return;
  }

  real L2 = SPHHARM_COEF_20 * TWOPI * cos_a * (1 - cos2_a);
  const real c1 = 0.546274f;
  const real c3 = 0.946176f;

  outputSH[SPHHARM_2m2] += (z1 * z0 * (c3 / c1) * L2) * color;
  outputSH[SPHHARM_2m1] += (z2 * z1 * (c3 / c1) * L2) * color;
  outputSH[SPHHARM_20] += (z2 * z2 * L2) * color;
  outputSH[SPHHARM_2p1] += (z2 * z0 * (c3 / c1) * L2) * color;
  outputSH[SPHHARM_2p2] += (z0 * z0 * (c3 / c1) * L2) * color;
}

void add_sphere_light_sphharm(Color3 outputSH[SPHHARM_NUM3], const Point3 &pos, real r2, const Color3 &color)
{
  if (r2 <= 0)
    return;

  real d2 = lengthSq(pos);

  if (!float_nonzero(d2))
  {
    outputSH[SPHHARM_00] += (SPHHARM_COEF_0 * 4 * PI) * color;
    return;
  }

  real cosA;

  if (d2 >= r2)
    cosA = sqrtf(1 - r2 / d2);
  else
    cosA = -sqrtf(1 - d2 / r2);

  add_hemisphere_sphharm(outputSH, pos / sqrtf(d2), cosA, color);
}
