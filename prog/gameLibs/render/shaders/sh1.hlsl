#ifndef SH1_INCLUDED
#define SH1_INCLUDED 1
half gi_luminance(half3 col)
{
  //return dot(col, half3(0.299, 0.587, 0.114));
  return dot(col, half(1./3));
}

struct SH1
{
  half4 R;
  half4 G;
  half4 B;
};

struct SH1Encoded
{
  half3 ambient;
  half4 directional;
};

SH1Encoded lerp(SH1Encoded a, SH1Encoded b, half v)
{
  a.ambient = lerp(a.ambient, b.ambient, v);
  a.directional = lerp(a.directional, b.directional, v);
  return a;
}

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

#ifndef INCREASE_SH1_PRECISION
#define INCREASE_SH1_PRECISION 1
#endif

void encode_sh1_luminance(SH1 sh1, out half3 ambient, out half4 directional)//ambient and directional can be negative
{
  half3 coefficient0 = half3(sh1.R.y, sh1.G.y, sh1.B.y);
  half3 coefficient1 = half3(sh1.R.z, sh1.G.z, sh1.B.z);
  half3 coefficient2 = half3(sh1.R.w, sh1.G.w, sh1.B.w);
  #if INCREASE_SH1_PRECISION
  ambient = half3(sh1.G.x, sh1.B.x, 0);//unused channel .z
  directional = half4(gi_luminance(coefficient0), gi_luminance(coefficient1), gi_luminance(coefficient2), sh1.R.x);
  #else
  ambient = half3(sh1.R.x, sh1.G.x, sh1.B.x);
  directional = half4(gi_luminance(coefficient0), gi_luminance(coefficient1), gi_luminance(coefficient2), 0);//unused channel .w
  #endif
}

SH1Encoded encode_sh1_luminance(SH1 sh1)
{
  SH1Encoded sh1E;
  encode_sh1_luminance(sh1, sh1E.ambient, sh1E.directional);
  return sh1E;
}

SH1 decode_sh1_luminance(half3 ambient, half4 directional)
{
  SH1 sh1;
  #if INCREASE_SH1_PRECISION
  ambient = half3(directional.w, ambient.xy);
  #endif
  sh1.R.x = ambient.r;
  sh1.G.x = ambient.g;
  sh1.B.x = ambient.b;
  half3 normalizedAmbient = half3(ambient.rgb / ( gi_luminance( ambient.rgb ) + 1e-5f )); // this calculation needs float precision

  sh1.R.yzw = directional.rgb * normalizedAmbient.r;
  sh1.G.yzw = directional.rgb * normalizedAmbient.g;
  sh1.B.yzw = directional.rgb * normalizedAmbient.b;
  return sh1;
}

SH1 decode_sh1_luminance(SH1Encoded sh1)
{
  return decode_sh1_luminance(sh1.ambient, sh1.directional);
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

half3 gi_sh1_decode_ambient(half3 ambient, half4 directional)
{
  #if INCREASE_SH1_PRECISION
  return half3(directional.w, ambient.xy);
  #endif
  return ambient;
}

#endif