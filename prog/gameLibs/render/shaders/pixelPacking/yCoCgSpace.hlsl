#ifndef Y_CO_CG_SPACE_HLSL_INCLUDED
#define Y_CO_CG_SPACE_HLSL_INCLUDED 1

// Converts from RGB space to YCoCg space
float3 PackToYCoCg(float3 c)
{
  float Co = c.r - c.b;
  float t = c.b + Co * 0.5;
  float Cg = c.g - t;
  float Y = t + Cg * 0.5;
  return float3(Y, Co, Cg);
}

void PackToYCoCg(float4 R, float4 G, float4 B, out float4 Y, out float4 Co, out float4 Cg)
{
  Co = R - B;
  float4 t = B + Co * 0.5;
  Cg = G - t;
  Y = t + Cg * 0.5;
}

float4 PackToYCoCgAlpha(float4 c)
{
  return float4(PackToYCoCg(c.rgb), c.a);
}

// Converts from YCoCg space to RGB space
float3 UnpackFromYCoCg(float3 c)
{
  float t = c.x - c.z * 0.5;
  float G = c.z + t;
  float B = t - c.y * 0.5;
  float R = c.y + B;
  return float3(R, G, B);
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

#endif