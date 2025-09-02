#ifndef Y_CO_CG_SPACE_HLSL_INCLUDED
#define Y_CO_CG_SPACE_HLSL_INCLUDED 1

// Converts from RGB space to YCoCg space
half3 PackToYCoCg(half3 c)
{
  half Co = c.r - c.b;
  half t = c.b + Co * 0.5h;
  half Cg = c.g - t;
  half Y = t + Cg * 0.5h;
  return half3(Y, Co, Cg);
}

void PackToYCoCg(float4 R, float4 G, float4 B, out float4 Y, out float4 Co, out float4 Cg)
{
  Co = R - B;
  float4 t = B + Co * 0.5;
  Cg = G - t;
  Y = t + Cg * 0.5;
}

half4 PackToYCoCgAlpha(half4 c)
{
  return half4(PackToYCoCg(c.rgb), c.a);
}

// Converts from YCoCg space to RGB space
half3 UnpackFromYCoCg(half3 c)
{
  half t = c.x - c.z * 0.5h;
  half G = c.z + t;
  half B = t - c.y * 0.5h;
  half R = c.y + B;
  return half3(R, G, B);
}

void UnpackFromYCoCg(float4 Y, float4 Co, float4 Cg, out float4 R, out float4 G, out float4 B)
{
  float4 t = Y - Cg * 0.5;
  G = Cg + t;
  B = t - Co * 0.5;
  R = Co + B;
}

float4 UnpackFromYCoCgAlpha(float4 c)
{
  return float4(UnpackFromYCoCg(c.rgb), c.a);
}

// YCoCg pack/unpack with all 3 components in [0,1] range

half3 PackToYCoCgUnorm(half3 c)
{
  half t = 0.25 * (c.x + c.z);
  c.y *= 0.5;
  half Y = c.y + t;
  half Co = 0.5 * (c.x - c.z) + 0.5;
  half Cg = c.y + 0.5 - t;
  return half3(Y, Co, Cg);
}

half3 UnpackFromYCoCgUnorm(half3 c)
{
  half t = c.x - c.z;
  return half3(t + c.y, c.x + c.z - 0.5, t - c.y + 1);
}

#endif