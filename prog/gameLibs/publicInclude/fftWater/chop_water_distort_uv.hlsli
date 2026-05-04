#ifndef CHOP_WATER_DISTORT_UV_HLSL
#define CHOP_WATER_DISTORT_UV_HLSL

float2 rotate_uv_chop(float2 uv, float angle)
{
  float2 sin_cos;
  sincos(angle, sin_cos.x, sin_cos.y);
  return float2(uv.x * sin_cos.y - uv.y * sin_cos.x,
          uv.x * sin_cos.x + uv.y * sin_cos.y);
}

float FetchDistortPoly9(float t)
{
    float x = t - floor(t);

    float q =
        (((((((-11597.8526f * x + 40592.1614f) * x - 56615.6345f) * x + 40060.3740f)
          * x - 15218.0223f) * x + 3058.21625f) * x - 307.474562f) * x + 17.2568954f);

    return x * (1.0f - x) * q;
}

#ifndef __cplusplus
float4 FetchDistortPoly9_4(float4 t)
{
    float4 x = t - floor(t);

    float4 q =
        (((((((-11597.8526f * x + 40592.1614f) * x - 56615.6345f) * x + 40060.3740f)
          * x - 15218.0223f) * x + 3058.21625f) * x - 307.474562f) * x + 17.2568954f);

    return x * (1.0f - x) * q;
}
float4 DistortCascadeUV4(float4 domainUV_xyzw, float distort_strength)
{
  float4 scaled = domainUV_xyzw * 0.1f;
  float4 fetched = FetchDistortPoly9_4(float4(scaled.y, scaled.x, scaled.w, scaled.z));
  return domainUV_xyzw + fetched * distort_strength;
}
#endif

float2 EvalDistort(float2 uv)
{
  float dy = FetchDistortPoly9(uv.x);
  float dx = FetchDistortPoly9(uv.y);
  return float2(dx, dy);
}

// /////////////////////// Apply Distort ///////////////////////////////////
float2 RotateUV_Cascade1(float2 uv)
{
  return rotate_uv_chop(uv * 4.0f + float2(0.5f, 0.5f), 0.0f);
}
float2 RotateUV_Cascade2(float2 uv)
{
  return rotate_uv_chop(uv * 16.0f + float2(0.5f, 0.5f), 0.1f);
}
float2 RotateUV_Cascade3(float2 uv)
{
  return rotate_uv_chop(uv * 64.0f + float2(0.5f, 0.5f), 0.2f);
}
float2 RotateUV_Cascade4(float2 uv)
{
  return rotate_uv_chop(uv * 256.0f + float2(0.5f, 0.5f), 0.3f);
}

float2 DistortCascadeUV(float2 domainUV, float distort_strength)
{
  return domainUV + EvalDistort(domainUV * 0.1f) * distort_strength;
}
#ifndef __cplusplus
float4 FetchDistortPoly5_4(float4 t)
{
    float4 x = frac(t);

    // q5(x) = c0 + c1*x + ... + c5*x^5
    float4 q =
        (((((690.43377716f * x - 1724.87724290f) * x + 1495.88457808f) * x
           - 523.28735114f) * x + 59.85341234f) * x + 4.13828095f);

    return x * (1.0f - x) * q;
}
float2 AggressiveWarp(float2 uv, float2 uv2, float tiling, float strength)
{
    float4 fetched = FetchDistortPoly5_4(float4(uv.y, uv.x, uv2.y, uv2.x) * tiling * 0.07);
    return uv + fetched.xy * strength * 2.0 + fetched.zw * strength;
}
#endif
float2 CalcDistortedCascadeUV(int cascade_index, float2 uv, float distort_strength)
{
  float2 domainUV;
  if (cascade_index == 0)
    domainUV = uv;
  else if (cascade_index == 1)
    domainUV = RotateUV_Cascade1(uv);
  else if (cascade_index == 2)
    domainUV = RotateUV_Cascade2(uv);
  else if (cascade_index == 3)
    domainUV = RotateUV_Cascade3(uv);
  else
    domainUV = RotateUV_Cascade4(uv);

  return DistortCascadeUV(domainUV, distort_strength);
}
#endif // CHOP_WATER_DISTORT_UV_HLSL
