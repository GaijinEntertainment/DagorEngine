#include <libTools/util/sHmathUtil.h>
#include <math/dag_SHlight.h>


void transform_sphharm(const Color3 sh[SPHHARM_NUM3], Color3 outsh[SPHHARM_NUM3], SH3Color3TransformCB &cb, int steps)
{
  real delta = PI / steps;
  real delta2 = delta * delta;
  real halfDelta = delta * 0.5f;

  int i, j;

  for (i = 0; i < SPHHARM_NUM3; ++i)
    outsh[i] = Color3(0, 0, 0);

  for (i = 0; i < steps; ++i)
  {
    real theta = i * delta + halfDelta;

    real sinTheta = sin(theta);
    real z = cos(theta);
    real area = delta2 * sinTheta;

    for (j = 0; j < steps * 2; ++j)
    {
      real phi = j * delta + halfDelta;

      real x = sinTheta * cos(phi);
      real y = sinTheta * sin(phi);

      Color3 func = sh[SPHHARM_00] * SPHHARM_COEF_0 +
                    (sh[SPHHARM_1m1] * y + sh[SPHHARM_10] * z + sh[SPHHARM_1p1] * x) * SPHHARM_COEF_1 +
                    (sh[SPHHARM_2m2] * x * y + sh[SPHHARM_2m1] * y * z + sh[SPHHARM_2p1] * x * z) * SPHHARM_COEF_21 +
                    sh[SPHHARM_20] * (3 * z * z - 1) * SPHHARM_COEF_20 + sh[SPHHARM_2p2] * (x * x - y * y) * SPHHARM_COEF_22;

      // debug("f=" FMT_P3 "",P3D(Point3(func,Point3::CTOR_FROM_PTR)));
      func = cb.transform(func);
      func *= area;

      outsh[SPHHARM_00] += func * SPHHARM_COEF_0;
      outsh[SPHHARM_1m1] += func * y * SPHHARM_COEF_1;
      outsh[SPHHARM_10] += func * z * SPHHARM_COEF_1;
      outsh[SPHHARM_1p1] += func * x * SPHHARM_COEF_1;
      outsh[SPHHARM_2m2] += func * x * y * SPHHARM_COEF_21;
      outsh[SPHHARM_2m1] += func * y * z * SPHHARM_COEF_21;
      outsh[SPHHARM_2p1] += func * x * z * SPHHARM_COEF_21;
      outsh[SPHHARM_20] += func * (3 * z * z - 1) * SPHHARM_COEF_20;
      outsh[SPHHARM_2p2] += func * (x * x - y * y) * SPHHARM_COEF_22;
    }
  }
}

/*
void add_sphere_light_sphharm(Color4 outputSH[SPHHARM_NUM3],
  const Point3 &pos, real r2, const Color3 &color)
{
  Color3 sphHarm_[SPHHARM_NUM3];
  for(int s=0; s<SPHHARM_NUM3; s++)
    sphHarm_[s] = Color3(outputSH[s].r, outputSH[s].g, outputSH[s].b);
  add_sphere_light_sphharm(sphHarm_, pos, r2, color);
  for(int s=0; s<SPHHARM_NUM3; s++)
    outputSH[s] = Color4(sphHarm_[s].r, sphHarm_[s].g, sphHarm_[s].b);
}
*/

Color4 recompute_color(const Point3 &n, const Color4 sphHarm[SPHHARM_NUM3])
{
  Color4 color;
  const real c1 = 0.429043f;
  const real c2 = 0.511664f;
  const real c3 = 0.743125f;
  const real c4 = 0.886227f;
  const real c5 = 0.247708f;

  color = (c1 * (n.x * n.x - n.y * n.y)) * sphHarm[SPHHARM_2p2] + (c3 * n.z * n.z - c5) * sphHarm[SPHHARM_20] +
          c4 * sphHarm[SPHHARM_00] + (2 * c1 * n.x * n.z) * sphHarm[SPHHARM_2p1] + (2 * c1 * n.y * n.z) * sphHarm[SPHHARM_2m1] +
          (2 * c1 * n.x * n.y) * sphHarm[SPHHARM_2m2] + (2 * c2 * n.x) * sphHarm[SPHHARM_1p1] + (2 * c2 * n.z) * sphHarm[SPHHARM_10] +
          (2 * c2 * n.y) * sphHarm[SPHHARM_1m1];
  color.clamp0();
  return color;
}
