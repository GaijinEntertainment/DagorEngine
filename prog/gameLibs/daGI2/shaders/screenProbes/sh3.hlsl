#ifndef SH3_INCLUDED
#define SH3_INCLUDED 1
struct SH3Scalar
{
  half4 v0;
  half4 v1;
  half v2;
};

struct SH3RGB
{
  SH3Scalar R, G, B;
};

SH3Scalar sh3_basis(half3 dir)
{
  SH3Scalar ret;
  ret.v0.x = 0.282095f;
  ret.v0.y = -0.488603f * dir.y;
  ret.v0.z = 0.488603f * dir.z;
  ret.v0.w = -0.488603f * dir.x;

  half3 dir2 = dir * dir;
  ret.v1.x = 1.092548f * dir.x * dir.y;
  ret.v1.y = -1.092548f * dir.y * dir.z;
  ret.v1.z = 0.315392f * (3.0f * dir2.z - 1.0f);
  ret.v1.w = -1.092548f * dir.x * dir.z;
  ret.v2 = 0.546274f * (dir2.x - dir2.y);

  return ret;
}

SH3Scalar diffuse_transfer_sh3(half3 dir,half expo)
{
  SH3Scalar ret = sh3_basis(dir);

  //in each band scale by cos(theta)^expo
  half L0 =	2 * PI / (1 + 1 * expo);
  half L1 = 2 * PI / (2 + 1 * expo);
  half L2 = expo * 2 * PI / (3 + 4 * expo + expo*expo);
  half L3 = (expo - 1) * 2 * PI / (8 + 6 * expo + expo*expo);

  ret.v0.x *= L0;
  ret.v0.yzw *= L1;
  ret.v1.xyzw *= L2;
  ret.v2 *= L2;
  return ret;
}

SH3RGB mul_sh3(SH3Scalar sh3, half3 color)
{
  SH3RGB ret;
  ret.R.v0 = sh3.v0 * color.r;
  ret.R.v1 = sh3.v1 * color.r;
  ret.R.v2 = sh3.v2 * color.r;
  ret.G.v0 = sh3.v0 * color.g;
  ret.G.v1 = sh3.v1 * color.g;
  ret.G.v2 = sh3.v2 * color.g;
  ret.B.v0 = sh3.v0 * color.b;
  ret.B.v1 = sh3.v1 * color.b;
  ret.B.v2 = sh3.v2 * color.b;
  return ret;
}

SH3RGB add_sh3(SH3RGB a, SH3RGB b)
{
  SH3RGB ret;
  ret.R.v0 = a.R.v0 + b.R.v0;
  ret.R.v1 = a.R.v1 + b.R.v1;
  ret.R.v2 = a.R.v2 + b.R.v2;

  ret.G.v0 = a.G.v0 + b.G.v0;
  ret.G.v1 = a.G.v1 + b.G.v1;
  ret.G.v2 = a.G.v2 + b.G.v2;

  ret.B.v0 = a.B.v0 + b.B.v0;
  ret.B.v1 = a.B.v1 + b.B.v1;
  ret.B.v2 = a.B.v2 + b.B.v2;
  return ret;
}

half dot_sh3(SH3Scalar a, SH3Scalar b)
{
  return dot(a.v0, b.v0) + dot(a.v1, b.v1) + a.v2 * b.v2;
}

half3 dot_sh3(SH3RGB A,SH3Scalar B)
{
  half3 ret = 0;
  ret.r = dot_sh3(A.R,B);
  ret.g = dot_sh3(A.G,B);
  ret.b = dot_sh3(A.B,B);
  return ret;
}

#endif