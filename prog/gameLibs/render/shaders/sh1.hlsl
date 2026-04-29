#ifndef SH1_INCLUDED
#define SH1_INCLUDED 1

struct SH1
{
  half4 R;
  half4 G;
  half4 B;
};

SH1 mul_sh1(half4 sh1_scalar, half3 color)
{
  SH1 ret;
  ret.R = sh1_scalar * color.r;
  ret.G = sh1_scalar * color.g;
  ret.B = sh1_scalar * color.b;
  return ret;
}
SH1 add_sh1(SH1 a, SH1 b)
{
  a.R += b.R;
  a.G += b.G;
  a.B += b.B;
  return a;
}

half4 sh_unit_basis1()
{
  return 0.282095f;
}

half4 sh_basis1(half3 dir)
{
  return half4(0.282095f, -0.488603f * dir.y, 0.488603f * dir.z, -0.488603f * dir.x);
}

half3 dot_sh1(SH1 sh1, half4 sh1_scalar)
{
  return half3(dot(sh1.R, sh1_scalar), dot(sh1.G, sh1_scalar), dot(sh1.B, sh1_scalar));
}

half4 diffuse_transfer_sh1_unit(half exponent = 1) // for non directional lighting
{
  return sh_unit_basis1() * (2 * half(PI) / (1 + 1 * exponent));
}
half2 sh1_l0_l1_coef(half exponent = 1) { return half2(2 * half(PI) / (1 + 1 * exponent), 2 * half(PI) / (2 + 1 * exponent)); }
half4 diffuse_transfer_sh1(half3 dir,half exponent = 1)
{
  return sh_basis1(dir)*sh1_l0_l1_coef(exponent).xyyy;
}

half4 negate_diffuse_transfer(half4 diff_transfer)
{
  return half4(diff_transfer.x, -diff_transfer.yzw);
}

//sh1_premul is SH1 *= sh_basis1(1)*sh1_l0_l1_coef().xyyy;
half3 sh1_premul_exp1_irradiance(SH1 sh1_premul, half3 dir)
{
  return max(0, dot_sh1(sh1_premul, half4(1, dir.yzx)));
}
half3 sh1_premul_exp1_ambient(SH1 sh1_premul)
{
  return half3(sh1_premul.R.x, sh1_premul.G.x, sh1_premul.B.x);
}

#endif